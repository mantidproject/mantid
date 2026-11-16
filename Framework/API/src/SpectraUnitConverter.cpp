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
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

namespace Mantid::API {
using Kernel::UnitParams;

/** Look up the geometry each spectrum of the workspace needs to be converted
 * @param workspace :: The workspace whose spectra are to be converted
 * @param unit :: The unit to convert between the workspace's X unit and, as ConvertUnits would convert to
 * @param emode :: The energy mode
 * @param efixed :: The fixed energy, if not taken from the instrument
 * @throw Kernel::Exception::NotFoundError :: If unit is not a unit
 * @throw std::runtime_error :: If the instrument has no source or sample
 */
SpectraUnitConverter::SpectraUnitConverter(const MatrixWorkspace &workspace, const std::string &unit,
                                           const Kernel::DeltaEMode::Type emode, const std::optional<double> efixed)
    : m_workspaceUnit(workspace.getAxis(0)->unit()), m_unit(Kernel::UnitFactory::Instance().create(unit)),
      m_workspaceUnitID(m_workspaceUnit->unitID()), m_unitID(m_unit->unitID()), m_l1(workspace.spectrumInfo().l1()),
      m_emode(emode), m_parameters(workspace.getNumberHistograms()) {
  const auto &spectrumInfo = workspace.spectrumInfo();
  const auto signedThetaParameters = workspace.getInstrument()->getStringParameter("show-signed-theta");
  const bool signedTheta = std::ranges::find(signedThetaParameters, "Always") != signedThetaParameters.end();

  PARALLEL_FOR_NO_WSP_CHECK()
  for (int64_t i = 0; i < static_cast<int64_t>(m_parameters.size()); ++i) {
    Kernel::UnitParametersMap params{{UnitParams::delta, 0.0}};
    if (efixed)
      params[UnitParams::efixed] = *efixed;
    try {
      spectrumInfo.getDetectorValues(*m_workspaceUnit, *m_unit, emode, signedTheta, i, params);
    } catch (const std::runtime_error &) {
      continue;
    }
    Parameters &stored = m_parameters[i].emplace();
    stored.fill(std::numeric_limits<double>::quiet_NaN());
    for (const auto &[param, value] : params)
      stored[static_cast<size_t>(param)] = value;
  }
}

/// Convert values of a spectrum in place from the workspace's X unit, setting them to NaN if they cannot be
void SpectraUnitConverter::fromWorkspaceUnit(const size_t index, std::span<double> values) const {
  convert(index, values, *m_workspaceUnit, *m_unit);
}

/// Convert values of a spectrum in place to the workspace's X unit, setting them to NaN if they cannot be
void SpectraUnitConverter::toWorkspaceUnit(const size_t index, std::span<double> values) const {
  convert(index, values, *m_unit, *m_workspaceUnit);
}

void SpectraUnitConverter::convert(const size_t index, std::span<double> values, const Kernel::Unit &from,
                                   const Kernel::Unit &to) const {
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
  // Units hold the parameters of the spectrum they were initialised for, so they cannot be shared between threads
  const auto localFrom = std::unique_ptr<Kernel::Unit>(from.clone());
  const auto localTo = std::unique_ptr<Kernel::Unit>(to.clone());
  try {
    for (auto &value : values)
      value = Kernel::UnitConversion::run(*localFrom, *localTo, value, m_l1, m_emode, params);
  } catch (const std::runtime_error &) {
    std::ranges::fill(values, std::numeric_limits<double>::quiet_NaN());
  }
}

} // namespace Mantid::API
