// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/TimeAtSampleStrategyIndirect.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
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
      m_beamDir(ws->getInstrument()->getReferenceFrame()->vecPointingAlongBeam()),
      m_paramMap(ws->constInstrumentParameters()) {}

/**
 * This will throw an exception if efixed is not found or negative
 */
double TimeAtSampleStrategyIndirect::getEfixed(const size_t &workspace_index) const {
  const IDetector *det = &m_spectrumInfo.detector(workspace_index);

  double efixed{0.};
  try {
    // Get the parameter map
    Parameter_sptr par = m_paramMap.getRecursive(det, "Efixed");
    if (par) {
      efixed = par->value<double>();
    }
  } catch (std::runtime_error &) {
    // Throws if a DetectorGroup, use single provided value
    std::stringstream errmsg;
    errmsg << "Inelastic instrument detector " << det->getID() << " of spectrum " << workspace_index
           << " does not have EFixed ";
    throw std::runtime_error(errmsg.str());
  }
  if (efixed <= 0.) {
    std::stringstream errmsg;
    errmsg << "Inelastic instrument detector " << det->getID() << " of spectrum " << workspace_index
           << " does not have EFixed ";
    throw std::runtime_error(errmsg.str());
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
