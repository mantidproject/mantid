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
#include "MantidKernel/UnitFactory.h"
#include "MantidQtWidgets/Common/QStringUtils.h"
#include <limits>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {

/**
 * Return an array of unit values to display with the point information
 */
static const std::array<std::tuple<Unit_sptr, bool>, 6> &displayUnits() {
  static const std::array<std::tuple<Unit_sptr, bool>, 6> units = {
      std::make_tuple(UnitFactory::Instance().create("TOF"), false),
      std::make_tuple(UnitFactory::Instance().create("Wavelength"), false),
      std::make_tuple(UnitFactory::Instance().create("Energy"), false),
      std::make_tuple(UnitFactory::Instance().create("dSpacing"), false),
      std::make_tuple(UnitFactory::Instance().create("MomentumTransfer"), false),
      std::make_tuple(UnitFactory::Instance().create("DeltaE"), true)};
  return units;
}

Logger g_log("ImageInfoModelMatrixWS");
} // namespace

namespace MantidQt::MantidWidgets {

/**
 * Constructor
 * @param workspace A MatrixWorkspace to provide information for the model
 */
ImageInfoModelMatrixWS::ImageInfoModelMatrixWS(Mantid::API::MatrixWorkspace_sptr workspace)
    : m_workspace(std::move(workspace)), m_spectrumInfo(nullptr) {
  cacheWorkspaceInfo();
}

/// @copydoc MantidQt::MantidWidgets::ImageInfoModel::info
ImageInfoModel::ImageInfo ImageInfoModelMatrixWS::info(const double x, const double y, const double signal) const {
  ImageInfo info(m_names);
  if (x == UnsetValue || y == UnsetValue || signal == UnsetValue)
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
  if (m_spectrumInfo->hasDetectors(wsIndex)) {
    info.setValue(3, defaultFormat(*spectrum.getDetectorIDs().begin()));
    info.setValue(4, defaultFormat(m_spectrumInfo->l2(wsIndex)));
    try {
      info.setValue(5, defaultFormat(m_spectrumInfo->signedTwoTheta(wsIndex) * rad2deg));
      info.setValue(6, defaultFormat(m_spectrumInfo->azimuthal(wsIndex) * rad2deg));
      setUnitsInfo(&info, 7, wsIndex, x);
    } catch (const std::exception &exc) {
      g_log.debug() << "Unable to fill in instrument angle-related value: " << exc.what() << "\n";
    }
  }

  return info;
}

/**
 * Add the unit-related quantities to the info list
 * @param info Existing ImageInfo object
 * @param infoIndex The index to start at when adding units information
 * @param wsIndex The wsIndex whose spectrum is under the cursor
 * @param x The value from the cursor on the X axis
 */
void ImageInfoModelMatrixWS::setUnitsInfo(ImageInfoModel::ImageInfo *info, int infoIndex, const size_t wsIndex,
                                          const double x) const {
  const auto l1 = m_spectrumInfo->l1();
  auto emode = m_workspace->getEMode();
  UnitParametersMap pmap{};
  m_spectrumInfo->getDetectorValues(Units::Empty(), Units::Empty(), emode, false, wsIndex, pmap);
  double efixed = 0.;
  if (pmap.find(UnitParams::efixed) != pmap.end()) {
    efixed = pmap[UnitParams::efixed];
  }
  // if it's not possible to find an efixed we are forced to treat is as elastic
  if (efixed == 0.0)
    emode = DeltaEMode::Elastic;
  double tof(0.0);
  const auto &unitFactory = UnitFactory::Instance();
  const auto tofUnit = unitFactory.create("TOF");
  if (m_xIsTOF) {
    // set as first element in list already
    tof = x;
  } else {
    try {
      tof = m_xunit->convertSingleToTOF(x, l1, emode, pmap);
    } catch (std::exception &exc) {
      // without TOF we can't get to the other units
      if (g_log.is(Logger::Priority::PRIO_DEBUG))
        g_log.debug() << "Error calculating TOF from " << m_xunit->unitID() << ": " << exc.what() << "\n";
      return;
    }
  }
  for (auto [unit, requiresEFixed] : displayUnits()) {
    if (unit->unitID() == m_xunit->unitID())
      continue;
    if (!requiresEFixed || efixed > 0.0) {
      try {
        // the final parameter is unused and a relic
        const auto unitValue = unit->convertSingleFromTOF(tof, l1, emode, pmap);
        info->setValue(infoIndex, defaultFormat(unitValue));
      } catch (std::exception &exc) {
        if (g_log.is(Logger::Priority::PRIO_DEBUG))
          g_log.debug() << "Error calculating " << unit->unitID() << " from " << m_xunit->unitID() << ": " << exc.what()
                        << "\n";
      }
      ++infoIndex;
    }
  }
}

/**
 * Cache metadata for the workspace for faster lookup
 */
void ImageInfoModelMatrixWS::cacheWorkspaceInfo() {
  g_log.debug("Updating cached workspace info");
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
  m_xunit = m_workspace->getAxis(0)->unit();
  m_xIsTOF = (m_xunit->unitID() == "TOF");
  createItemNames();
}

/**
 * Create a sequence of names for each item this model will produce
 * and store it internally
 */
void ImageInfoModelMatrixWS::createItemNames() {
  auto appendName = [this](const auto &name) { m_names.emplace_back(name); };
  auto shortUnitName = [](const Unit &unit) {
    const auto caption = unit.caption();
    if (caption.rfind("-flight") != std::string::npos)
      return QString("TOF");
    else if (caption == "q")
      return QString("|Q|");
    else
      return QString::fromStdString(caption);
  };
  auto appendUnit = [&appendName, &shortUnitName](const auto &unit) {
    QString name = shortUnitName(unit);
    const auto unitLabel = unit.label().utf8();
    if (!unitLabel.empty()) {
      name += "(" + MantidQt::API::toQStringInternal(unitLabel) + ")";
    }
    appendName(name);
  };

  // General information first
  if (m_xunit && !m_xunit->caption().empty())
    appendUnit(*m_xunit);
  else
    appendName("x");
  if (auto yaxis = m_workspace->getAxis(1)) {
    if (yaxis->isSpectra())
      appendName("Spectrum");
    else if (auto yunit = yaxis->unit()) {
      if (!yunit->caption().empty())
        appendUnit(*yunit);
      else
        appendName("y");
    }
  } else
    appendName("y");
  appendName("Signal");
  appendName("Det ID");
  appendName("L2(m)");
  appendName("TwoTheta(Deg)");
  appendName("Azimuthal(Deg)");

  // Add conversions to common units
  auto appendUnitIfNotX = [this, &appendUnit](const auto &unit) {
    if (unit.unitID() == m_xunit->unitID())
      return;
    appendUnit(unit);
  };
  for (const auto &unitInfo : displayUnits()) {
    appendUnitIfNotX(*std::get<0>(unitInfo));
  }
}

} // namespace MantidQt::MantidWidgets
