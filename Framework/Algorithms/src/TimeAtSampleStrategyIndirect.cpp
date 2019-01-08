// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/TimeAtSampleStrategyIndirect.h"
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
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

/** Constructor
 */
TimeAtSampleStrategyIndirect::TimeAtSampleStrategyIndirect(
    MatrixWorkspace_const_sptr ws)
    : m_ws(ws), m_spectrumInfo(m_ws->spectrumInfo()) {}

Correction
TimeAtSampleStrategyIndirect::calculate(const size_t &workspace_index) const {

  // A constant among all spectra
  constexpr double TWO_MEV_OVER_MASS =
      2. * PhysicalConstants::meV / PhysicalConstants::NeutronMass;

  const IDetector *det = &m_spectrumInfo.detector(workspace_index);
  if (m_spectrumInfo.isMonitor(workspace_index)) {
    // use the same math as TimeAtSampleStrategyElastic
    const double L1s = m_spectrumInfo.l1();
    const auto &beamDir =
        m_ws->getInstrument()->getReferenceFrame()->vecPointingAlongBeam();
    const double L1m =
        beamDir.scalar_prod(m_spectrumInfo.sourcePosition() -
                            m_spectrumInfo.position(workspace_index));
    const double scale = std::abs(L1s / L1m);
    return Correction(0., scale);
  }

  // Get E_fix
  double efix{0.};
  try {
    // Get the parameter map
    const ParameterMap &pmap = m_ws->constInstrumentParameters();
    Parameter_sptr par = pmap.getRecursive(det, "Efixed");
    if (par) {
      efix = par->value<double>();
    }
  } catch (std::runtime_error &) {
    // Throws if a DetectorGroup, use single provided value
    std::stringstream errmsg;
    errmsg << "Inelastic instrument detector " << det->getID()
           << " of spectrum " << workspace_index << " does not have EFixed ";
    throw std::runtime_error(errmsg.str());
  }
  if (efix <= 0.) {
    std::stringstream errmsg;
    errmsg << "Inelastic instrument detector " << det->getID()
           << " of spectrum " << workspace_index << " does not have EFixed ";
    throw std::runtime_error(errmsg.str());
  }

  const double l2 = m_spectrumInfo.l2(workspace_index);
  const double shift = -1. * l2 / sqrt(efix * TWO_MEV_OVER_MASS);

  // 1.0 * tof + shift
  return Correction(shift, 1.0);
}

} // namespace Algorithms
} // namespace Mantid
