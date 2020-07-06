// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidQtWidgets/Common/ImageInfoModelMatrixWS.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Crystal/AngleUnits.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/UnitConversion.h"
#include "MantidKernel/UnitFactory.h"
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {
/// Flag value to specify a value should show as unspecified
constexpr auto UNSET_VALUE = std::numeric_limits<double>::max();

Logger g_log("ImageInfoModelMatrixWS");

} // namespace

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 * @param workspace A MatrixWorkspace to provide information for the model
 */
ImageInfoModelMatrixWS::ImageInfoModelMatrixWS(
    Mantid::API::MatrixWorkspace_sptr workspace)
    : m_workspace(std::move(workspace)), m_spectrumInfo(nullptr),
      m_xMin(UNSET_VALUE), m_xMax(UNSET_VALUE) {
  cacheWorkspaceInfo();
}

/// @copydoc MantidQt::MantidWidgets::ImageInfoModel::info
ImageInfoModel::ImageInfo
ImageInfoModelMatrixWS::info(const double x, const double y,
                             const double signal) const {
  ImageInfo info(createItemNames());
  if (x == UNSET_VALUE || y == UNSET_VALUE || signal == UNSET_VALUE)
    return info;

  info.setValue(0, defaultFormat(x));
  const auto yAxis = m_workspace->getAxis(1);
  const auto wsIndex = yAxis->indexOfValue(y);
  const auto &spectrum = m_workspace->getSpectrum(wsIndex);
  if (yAxis->isSpectra()) {
    info.setValue(1, defaultFormat(spectrum.getSpectrumNo()));
  } else {
    info.setValue(1, defaultFormat(y));
  }
  info.setValue(2, defaultFormat(signal));

  if (!(m_instrument && m_source && m_sample))
    return info;

  // Everything else requires an instrument
  if (m_spectrumInfo->hasDetectors(wsIndex))
    info.setValue(3, defaultFormat(*spectrum.getDetectorIDs().begin()));

  info.setValue(4, defaultFormat(m_spectrumInfo->l2(wsIndex)));
  info.setValue(5, defaultFormat(m_spectrumInfo->signedTwoTheta(wsIndex)));
  info.setValue(6, defaultFormat(m_spectrumInfo->azimuthal(wsIndex)));
  setUnitsInfo(&info, 7, wsIndex, x);

  return info;
}

/**
 * Add the unit-related quantities to the info list
 * @param info Existing ImageInfo object
 * @param infoIndex The index to start at when adding units information
 * @param wsIndex The wsIndex whose spectrum is under the cursor
 */
void ImageInfoModelMatrixWS::setUnitsInfo(ImageInfoModel::ImageInfo *info,
                                          int infoIndex, const size_t wsIndex,
                                          const double x) const {
  const auto l1 = m_spectrumInfo->l1();
  const auto l2 = m_spectrumInfo->l2(wsIndex);
  const auto twoTheta = m_spectrumInfo->twoTheta(wsIndex);
  const auto [emode, efixed] = efixedAt(wsIndex);

  double tof(0.0);
  if (m_xunit == "TOF") {
    // set as first element in list already
    tof = x;
  } else {
    try {
      tof = UnitConversion::run(m_xunit.toStdString(), "TOF", x, l1, l2,
                                twoTheta, emode, efixed);
      info->setValue(infoIndex, defaultFormat(tof));
      ++infoIndex;
    } catch (std::invalid_argument &exc) {
      // without TOF we can't get to the other units
      g_log.debug() << "Error calculating TOF from " << m_xunit.toStdString()
                    << ": " << exc.what() << "\n";
      return;
    }
  }

  const auto &unitFactory = UnitFactory::Instance();
  const auto tofUnit = unitFactory.create("TOF");
  for (const auto &unitName : {"Wavelength", "Energy", "dSpacing"}) {
    if (unitName == m_xunit)
      continue;
    const auto unit = unitFactory.create(unitName);
    const auto unitValue =
        unit->convertSingleFromTOF(tof, l1, l2, twoTheta, 0, 0.0, 0.0);
    info->setValue(infoIndex, defaultFormat(unitValue));
    ++infoIndex;
  }
  if (efixed > 0.0) {
    for (const auto &unitName : {"MomentumTransfer", "DeltaE"}) {
      if (unitName == m_xunit)
        continue;
      const auto unit = unitFactory.create(unitName);
      const auto unitValue =
          unit->convertSingleFromTOF(tof, l1, l2, twoTheta, emode, efixed, 0.0);
      info->setValue(infoIndex, defaultFormat(unitValue));
      ++infoIndex;
    }
  }
}

/**
 * Cache metadata for the workspace for faster lookup
 */
void ImageInfoModelMatrixWS::cacheWorkspaceInfo() {
  g_log.debug("Updating cached workspace info");
  m_workspace->getXMinMax(m_xMin, m_xMax);
  m_spectrumInfo = &m_workspace->spectrumInfo();
  m_instrument = m_workspace->getInstrument();
  if (m_instrument) {
    m_source = m_instrument->getSource();
    if (!m_source) {
      g_log.debug("No source on instrument in MatrixWorkspace");
    }

    m_sample = m_instrument->getSample();
    if (!m_sample) {
      g_log.debug("No sample on instrument in MatrixWorkspace");
    }
  } else {
    g_log.debug("No instrument on MatrixWorkspace");
  }
  if (auto xunit = m_workspace->getAxis(0)->unit())
    m_xunit = QString::fromStdString(xunit->unitID());
  else
    m_xunit = "x";
  if (auto yaxis = m_workspace->getAxis(1)) {
    if (yaxis->isSpectra())
      m_yunit = "Spectrum";
    else if (auto yunit = yaxis->unit())
      m_yunit = QString::fromStdString(yunit->unitID());
  } else
    m_yunit = "y";
}

/**
 * Create a sequence of names for each item this model will produce
 */
ImageInfoModel::ImageInfo::StringItems
ImageInfoModelMatrixWS::createItemNames() const {
  ImageInfoModel::ImageInfo::StringItems names;
  names.reserve(itemCount());
  auto appendName = [&names](const auto name) { names.emplace_back(name); };
  auto appendNameIfXUnitNot = [this, &names, &appendName](const auto name) {
    if (name != this->m_xunit)
      appendName(name);
  };

  appendName(m_xunit);
  appendName(m_yunit);
  appendName("Signal");
  appendName("Det ID");
  appendName("L2(m)");
  appendName("TwoTheta(Deg)");
  appendName("Azimuthal(Deg)");
  appendNameIfXUnitNot("TOF");
  appendNameIfXUnitNot("Wavelength");
  appendNameIfXUnitNot("Energy");
  appendNameIfXUnitNot("dSpacing");
  appendNameIfXUnitNot("q");
  appendNameIfXUnitNot("DeltaE");

  return names;
}

/**
 * Return tuple(emode, efixed) for the pixel at the given workspace index
 * @param wsIndex Indeox of spectrum to query
 */
std::tuple<Mantid::Kernel::DeltaEMode::Type, double>
ImageInfoModelMatrixWS::efixedAt(const size_t wsIndex) const {
  DeltaEMode::Type emode = m_workspace->getEMode();
  double efixed(0.0);
  if (m_spectrumInfo->isMonitor(wsIndex))
    return std::make_tuple(emode, efixed);

  if (emode == DeltaEMode::Direct) {
    const auto &run = m_workspace->run();
    for (const auto &logName : {"Ei", "EnergyRequested", "EnergyEstimate"}) {
      if (run.hasProperty(logName)) {
        efixed = run.getPropertyValueAsType<double>(logName);
        break;
      }
    }
  } else if (emode == DeltaEMode::Indirect) {
    const auto &detector = m_spectrumInfo->detector(wsIndex);
    const ParameterMap &pmap = m_workspace->constInstrumentParameters();
    try {
      for (const auto &paramName : {"Efixed", "Efixed-val"}) {
        auto parameter = pmap.getRecursive(&detector, paramName);
        if (parameter) {
          efixed = parameter->value<double>();
          break;
        }
        // check the instrument as if the detector is a group the above
        // recursion doesn't work
        parameter = pmap.getRecursive(m_instrument.get(), paramName);
        if (parameter) {
          efixed = parameter->value<double>();
          break;
        }
      }
    } catch (std::runtime_error &exc) {
      g_log.debug() << "Failed to get efixed from spectrum at index " << wsIndex
                    << ": " << exc.what() << "\n";
      efixed = 0.0;
    }
  }

  // if it's not possible to find an efixed we are forced to treat is as elastic
  if (efixed == 0.0)
    emode = DeltaEMode::Elastic;

  return std::make_tuple(emode, efixed);
}

} // namespace MantidWidgets
} // namespace MantidQt
