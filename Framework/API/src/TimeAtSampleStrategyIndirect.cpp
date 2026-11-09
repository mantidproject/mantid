// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/TimeAtSampleStrategyIndirect.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/V3D.h"
#include <cmath>
#include <memory>
#include <sstream>
#include <utility>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;

namespace Mantid::API {

/** Constructor
 */
TimeAtSampleStrategyIndirect::TimeAtSampleStrategyIndirect(MatrixWorkspace_const_sptr ws)
    : m_spectrumInfo(ws->spectrumInfo()), m_L1s(m_spectrumInfo.l1()),
      m_beamDir(ws->getReferenceFrame()->vecPointingAlongBeam()), m_componentInfo(ws->componentInfo()),
      m_detectorInfo(ws->detectorInfo()) {}

/**
 * This will throw an exception if efixed is not found or negative
 */
double TimeAtSampleStrategyIndirect::getEfixed(const size_t &workspace_index) const {
  if (!m_spectrumInfo.hasDetectors(workspace_index)) {
    throw Kernel::Exception::NotFoundError("TimeAtSampleStrategyIndirect: no detectors for spectrum",
                                           std::to_string(workspace_index));
  }
  // The first detector, for a spectrum of several
  const auto detIndex = m_spectrumInfo.spectrumDefinition(workspace_index)[0].first;
  const auto noEfixed = [&]() {
    std::stringstream errmsg;
    errmsg << "Inelastic instrument detector " << m_detectorInfo.detid(detIndex) << " of spectrum " << workspace_index
           << " does not have EFixed ";
    return std::runtime_error(errmsg.str());
  };

  double efixed{0.};
  // A spectrum of several detectors has no parameters of its own
  if (m_spectrumInfo.hasUniqueDetector(workspace_index)) {
    try {
      const auto values = m_componentInfo.getNumberParameter(detIndex, "Efixed");
      if (!values.empty()) {
        efixed = values.front();
      }
    } catch (std::runtime_error &) {
      throw noEfixed();
    }
  }
  if (efixed <= 0.) {
    throw noEfixed();
  }

  return efixed;
}

Correction TimeAtSampleStrategyIndirect::calculate(const size_t &workspace_index) const {

  // A constant among all spectra
  constexpr double TWO_MEV_OVER_MASS = 2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;

  if (m_spectrumInfo.isMonitor(workspace_index)) {
    // use the same math as TimeAtSampleStrategyElastic
    const double L1m =
        m_beamDir.scalar_prod(m_spectrumInfo.sourcePosition() - m_spectrumInfo.position(workspace_index));
    const double scale = std::abs(m_L1s / L1m);
    return Correction(scale, 0.);
  } else {
    const double efix = this->getEfixed(workspace_index);
    const double l2 = m_spectrumInfo.l2(workspace_index);
    const double shift = -1. * l2 / sqrt(efix * TWO_MEV_OVER_MASS);

    // 1.0 * tof + shift
    constexpr double NO_MULTIPLY_TOF{1.};
    return Correction(NO_MULTIPLY_TOF, shift);
  }
}

} // namespace Mantid::API
