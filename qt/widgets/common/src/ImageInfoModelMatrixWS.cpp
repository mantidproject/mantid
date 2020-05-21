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

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace {
Logger g_log("ImageInfoModelMatrixWS");
}

namespace MantidQt {
namespace MantidWidgets {

/**
 * Constructor
 */
ImageInfoModelMatrixWS::ImageInfoModelMatrixWS(const MatrixWorkspace_sptr &ws)
    : m_workspace(ws) {

  m_workspace->getXMinMax(m_xMin, m_xMax);
  m_spectrumInfo = &m_workspace->spectrumInfo();
  m_instrument = m_workspace->getInstrument();
  if (m_instrument) {
    m_source = m_instrument->getSource();
    if (!m_source) {
      g_log.debug("No SOURCE on instrument in MatrixWorkspace");
    }

    m_sample = m_instrument->getSample();
    if (!m_sample) {
      g_log.debug("No SAMPLE on instrument in MatrixWorkspace");
    }
  } else {
    g_log.debug("No INSTRUMENT on MatrixWorkspace");
  }
}

// Creates a list containing pairs of strings with information about the
// coordinates in the workspace.
std::vector<std::string>
ImageInfoModelMatrixWS::getInfoList(const double x, const double specNum,
                                    const double signal, bool includeValues) {
  std::vector<std::string> list;
  const int spectrumNumber = static_cast<int>(specNum);
  size_t wsIndex;
  try {
    wsIndex = m_workspace->getIndexFromSpectrumNumber(spectrumNumber);
  } catch (std::runtime_error) {
    if (includeValues)
      return list;
    wsIndex = 0;
  }

  if (includeValues && (x >= m_xMax || x <= m_xMin))
    return list;

  addNameAndValue("Signal", signal, 4, list, includeValues);

  const auto &spec = m_workspace->getSpectrum(wsIndex);
  addNameAndValue("Spec Num", spectrumNumber, 0, list, includeValues);

  std::string x_label = "";
  const auto &old_unit = m_workspace->getAxis(0)->unit();
  if (old_unit) {
    x_label = old_unit->caption();
    addNameAndValue(x_label, x, 3, list, includeValues);
  }

  const auto &ids = spec.getDetectorIDs();
  if (!ids.empty()) {
    const auto id = *(ids.begin());
    addNameAndValue("Det ID", id, 0, list, includeValues);
  }

  /* Cannot get any more information if we do not have a instrument, source and
   * sample */
  if (!(m_instrument && m_source && m_sample)) {
    return list;
  }

  if (!old_unit) {
    g_log.debug("No UNITS on MatrixWorkspace X-axis");
    return list;
  }

  if (!m_spectrumInfo->hasDetectors(wsIndex)) {
    g_log.debug() << "No DETECTOR for wsIndex " << wsIndex
                  << " in MatrixWorkspace\n";
    return list;
  }

  const double l1 = m_spectrumInfo->l1();
  const double l2 = m_spectrumInfo->l2(wsIndex);
  double two_theta(0.0);
  double azi(0.0);
  if (!m_spectrumInfo->isMonitor(wsIndex)) {
    two_theta = m_spectrumInfo->twoTheta(wsIndex);
    azi = m_spectrumInfo->detector(wsIndex).getPhi();
  }
  addNameAndValue("L2", l2, 4, list, includeValues);
  addNameAndValue("TwoTheta", two_theta * Mantid::Geometry::rad2deg, 2, list,
                  includeValues);
  addNameAndValue("Azimuthal", azi * Mantid::Geometry::rad2deg, 2, list,
                  includeValues);

  int emode(0);
  double efixed(0.0), delta(0.0);

  // try getting direct geometry information from the run object
  if (efixed == 0) {
    const Mantid::API::Run &run = m_workspace->run();
    if (run.hasProperty("Ei")) {
      efixed = run.getPropertyValueAsType<double>("Ei");
      emode = 1; // only correct if direct geometry
    } else if (run.hasProperty("EnergyRequested")) {
      efixed = run.getPropertyValueAsType<double>("EnergyRequested");
      emode = 1;
    } else if (run.hasProperty("EnergyEstimate")) {
      efixed = run.getPropertyValueAsType<double>("EnergyEstimate");
      emode = 1;
    }
  }

  // If it did not get emode & efixed, try getting indirect geometry information
  // from the detector object
  if (efixed == 0) {
    const auto &det = m_spectrumInfo->detector(wsIndex);
    if (!(m_spectrumInfo->isMonitor(wsIndex) && det.hasParameter("Efixed"))) {
      try {
        const ParameterMap &pmap = m_workspace->constInstrumentParameters();
        const Parameter_sptr par = pmap.getRecursive(&det, "Efixed");
        if (par) {
          efixed = par->value<double>();
          emode = 2;
        }
      } catch (std::runtime_error &) {
        g_log.debug() << "Failed to get Efixed from detector ID: "
                      << det.getID() << " in MatrixWSDataSource\n";
        efixed = 0;
      }
    }
  }

  if (efixed == 0)
    emode = 0;

  const double tof =
      old_unit->convertSingleToTOF(x, l1, l2, two_theta, emode, efixed, delta);
  if (x_label != "Time-of-flight") {
    addNameAndValue("Time-of-flight", tof, 1, list, includeValues);
  }

  if (x_label != "Wavelength") {
    const auto wl_unit = UnitFactory::Instance().create("Wavelength");
    const double wavelength = wl_unit->convertSingleFromTOF(
        tof, l1, l2, two_theta, emode, efixed, delta);
    addNameAndValue("Wavelength", wavelength, 4, list, includeValues);
  }

  if (x_label != "Energy") {
    const auto e_unit = UnitFactory::Instance().create("Energy");
    const double energy = e_unit->convertSingleFromTOF(tof, l1, l2, two_theta,
                                                       emode, efixed, delta);
    addNameAndValue("Energy", energy, 4, list, includeValues);
  }

  if (x_label != "d-Spacing" && ((two_theta != 0.0 && emode == 0) || !includeValues)) {
    const auto d_unit = UnitFactory::Instance().create("dSpacing");
    const double d_spacing = d_unit->convertSingleFromTOF(
        tof, l1, l2, two_theta, emode, efixed, delta);
    addNameAndValue("d-Spacing", d_spacing, 4, list, includeValues);
  }

  if (x_label != "q" && (two_theta != 0.0 || !includeValues)) {
    const auto q_unit = UnitFactory::Instance().create("MomentumTransfer");
    const double mag_q = q_unit->convertSingleFromTOF(tof, l1, l2, two_theta,
                                                      emode, efixed, delta);
    addNameAndValue("|Q|", mag_q, 4, list, includeValues);
  }

  if (x_label != "DeltaE" && (two_theta != 0.0 && emode != 0)) {
    const auto deltaE_unit = UnitFactory::Instance().create("DeltaE");
    const double delta_E = deltaE_unit->convertSingleFromTOF(
        tof, l1, l2, two_theta, emode, efixed, delta);
    addNameAndValue("DeltaE", delta_E, 4, list, includeValues);
  }
  return list;
}

} // namespace MantidWidgets
} // namespace MantidQt
