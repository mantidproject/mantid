// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/SpectraUnitConverter.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/MultiThreaded.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>

namespace Mantid::API {
using Kernel::UnitParams;

namespace {
/// The quick conversion between the units, if there is one, as ConvertUnits would use
template <typename QuickConversion>
std::optional<QuickConversion> findQuickConversion(const Kernel::Unit &from, const Kernel::Unit &to) {
  double factor{0.0}, power{0.0};
  if (!from.quickConversion(to, factor, power))
    return std::nullopt;
  return QuickConversion{factor, power};
}

std::unique_ptr<Kernel::Unit> cloneUnit(const Kernel::Unit &unit) {
  return std::unique_ptr<Kernel::Unit>(unit.clone());
}

/// The workspace to make a converter for, which must not be null
const MatrixWorkspace &workspaceToConvert(const std::shared_ptr<const MatrixWorkspace> &workspace) {
  if (!workspace)
    throw std::invalid_argument("A unit converter needs a workspace to convert the units of");
  return *workspace;
}
} // namespace

/** @param workspace :: The workspace whose units are to be converted
 * @return Whether the instrument's show-signed-theta parameter is "Always"
 */
bool unitConversionUsesSignedTwoTheta(const MatrixWorkspace &workspace) {
  const auto parameters = workspace.getInstrument()->getStringParameter("show-signed-theta");
  return std::ranges::find(parameters, "Always") != parameters.end();
}

/** @param workspace :: The workspace whose units are to be converted
 * @param emode :: The energy mode
 * @param efixed :: The fixed energy given for the conversion, if any
 * @return efixed if given, else for direct geometry the incident energy in the workspace's logs, if it has one
 */
std::optional<double> unitConversionFixedEnergy(const MatrixWorkspace &workspace, const Kernel::DeltaEMode::Type emode,
                                                const std::optional<double> efixed) {
  if (efixed || emode != Kernel::DeltaEMode::Direct)
    return efixed;
  try {
    return workspace.getEFixedGivenEMode(nullptr, emode);
  } catch (const std::runtime_error &) {
    return std::nullopt;
  }
}

/** Look up the geometry each spectrum of the workspace needs to be converted
 * @param workspace :: The workspace whose spectra are to be converted, kept alive while this exists
 * @param unit :: The unit to convert between the workspace's X unit and, as ConvertUnits would convert to
 * @param emode :: The energy mode
 * @param efixed :: The fixed energy, if not taken from the instrument
 * @throw Kernel::Exception::NotFoundError :: If unit is not a unit
 * @throw std::runtime_error :: If the instrument has no source or sample
 * @throw std::invalid_argument :: If workspace is null, or the units cannot be converted in the energy mode, e.g. to
 * energy transfer when elastic, or a spectrum throws any other error ConvertUnits would fail on
 */
SpectraUnitConverter::SpectraUnitConverter(std::shared_ptr<const MatrixWorkspace> workspace, const std::string &unit,
                                           const Kernel::DeltaEMode::Type emode, const std::optional<double> efixed)
    : m_workspace(std::move(workspace)), m_numberOfSpectra(workspaceToConvert(m_workspace).getNumberHistograms()),
      m_workspaceUnit(m_workspace->getAxis(0)->unit()), m_unit(Kernel::UnitFactory::Instance().create(unit)),
      m_workspaceUnitID(m_workspaceUnit->unitID()), m_unitID(m_unit->unitID()), m_emode(emode),
      m_quickFromWorkspaceUnit(findQuickConversion<QuickConversion>(*m_workspaceUnit, *m_unit)),
      m_quickToWorkspaceUnit(findQuickConversion<QuickConversion>(*m_unit, *m_workspaceUnit)) {
  if (emode == Kernel::DeltaEMode::Undefined)
    throw std::invalid_argument("Units cannot be converted with an undefined energy mode");
  // As in ConvertUnits, a quick conversion needs no geometry, so works even without an instrument
  if (m_quickFromWorkspaceUnit && m_quickToWorkspaceUnit)
    return;

  const auto &workspaceRef = *m_workspace;
  const auto &spectrumInfo = workspaceRef.spectrumInfo();
  m_l1 = spectrumInfo.l1();
  const bool signedTheta = unitConversionUsesSignedTwoTheta(workspaceRef);
  const auto fixedEnergy = unitConversionFixedEnergy(workspaceRef, emode, efixed);
  m_parameters.resize(m_numberOfSpectra);

  // An error ConvertUnits would fail on rather than mask the spectrum, kept to be thrown outside the parallel loop
  std::exception_ptr failure;
  PARALLEL_FOR_IF(Kernel::threadSafe(workspaceRef))
  for (int64_t i = 0; i < static_cast<int64_t>(m_numberOfSpectra); ++i) {
    Kernel::UnitParametersMap params{{UnitParams::delta, 0.0}};
    if (fixedEnergy)
      params[UnitParams::efixed] = *fixedEnergy;
    try {
      spectrumInfo.getDetectorValues(*m_workspaceUnit, *m_unit, emode, signedTheta, i, params);
      // Initialising the units checks they have the parameters they need. Converting the ends of the spectrum checks
      // what only converting values does, e.g. that d-spacing is not asked of a spectrum on the beam axis.
      const auto from = cloneUnit(*m_workspaceUnit);
      const auto to = cloneUnit(*m_unit);
      from->initialize(m_l1, emode, params);
      to->initialize(m_l1, emode, params);
      if (const auto &xData = workspaceRef.x(static_cast<size_t>(i)); !xData.empty()) {
        to->singleFromTOF(from->singleToTOF(xData.front()));
        to->singleFromTOF(from->singleToTOF(xData.back()));
      }
    } catch (const std::runtime_error &) {
      // ConvertUnits would mask the spectrum, e.g. as it has no detectors, so it cannot be converted
      continue;
    } catch (...) {
      PARALLEL_CRITICAL(SpectraUnitConverter_failure) {
        if (!failure)
          failure = std::current_exception();
      }
      continue;
    }
    Parameters &stored = m_parameters[i].emplace();
    stored.fill(std::numeric_limits<double>::quiet_NaN());
    for (const auto &[param, value] : params)
      stored[static_cast<size_t>(param)] = value;
  }
  if (failure)
    std::rethrow_exception(failure);
}

/// Whether this was made for the workspace, so has the geometry of its spectra
bool SpectraUnitConverter::isFor(const MatrixWorkspace &workspace) const {
  return &workspace == m_workspace.get() && workspace.getNumberHistograms() == m_numberOfSpectra &&
         workspace.getAxis(0)->unit()->unitID() == m_workspaceUnitID;
}

/// Whether values of the spectrum can be converted, which ConvertUnits would otherwise mask the spectrum for
bool SpectraUnitConverter::canConvert(const size_t index) const {
  return m_quickFromWorkspaceUnit || m_parameters[index].has_value();
}

/// Convert values of a spectrum in place from the workspace's X unit, setting them all to NaN if they cannot be
void SpectraUnitConverter::fromWorkspaceUnit(const size_t index, std::span<double> values) const {
  convert(index, values, *m_workspaceUnit, *m_unit, m_quickFromWorkspaceUnit);
}

/// Convert values of a spectrum in place to the workspace's X unit, setting them all to NaN if they cannot be
void SpectraUnitConverter::toWorkspaceUnit(const size_t index, std::span<double> values) const {
  convert(index, values, *m_unit, *m_workspaceUnit, m_quickToWorkspaceUnit);
}

void SpectraUnitConverter::convert(const size_t index, std::span<double> values, const Kernel::Unit &from,
                                   const Kernel::Unit &to,
                                   const std::optional<QuickConversion> &quickConversion) const {
  if (quickConversion) {
    std::transform(values.begin(), values.end(), values.begin(), [&quickConversion](const double value) {
      return quickConversion->factor * std::pow(value, quickConversion->power);
    });
    return;
  }
  const auto &stored = m_parameters[index];
  if (!stored) {
    std::ranges::fill(values, std::numeric_limits<double>::quiet_NaN());
    return;
  }
  Kernel::UnitParametersMap params;
  for (size_t param = 0; param < stored->size(); ++param) {
    if (!std::isnan((*stored)[param]))
      params.emplace(static_cast<UnitParams>(param), (*stored)[param]);
  }
  // Units hold the parameters of the spectrum they were initialised for, so they cannot be shared between threads.
  // They are initialised once for all the values, rather than once for each.
  const auto localFrom = cloneUnit(from);
  const auto localTo = cloneUnit(to);
  try {
    localFrom->toTOF(values.begin(), values.end(), m_l1, m_emode, params);
    localTo->fromTOF(values.begin(), values.end(), m_l1, m_emode, params);
  } catch (const std::exception &) {
    // As ConvertUnits masks a spectrum it cannot convert. Nothing must escape, as this is called in parallel loops
    std::ranges::fill(values, std::numeric_limits<double>::quiet_NaN());
  }
}

} // namespace Mantid::API
