#include "MantidAlgorithms/TimeAtSampleStrategyDirect.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/V3D.h"
#include <cmath>

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TimeAtSampleStrategyDirect::TimeAtSampleStrategyDirect(
    MatrixWorkspace_const_sptr ws, double ei)
    : m_constShift(0) {

  // Get L1
  V3D samplepos = ws->getInstrument()->getSample()->getPos();
  V3D sourcepos = ws->getInstrument()->getSource()->getPos();
  double l1 = samplepos.distance(sourcepos);

  // Calculate constant (to all spectra) shift
  m_constShift = l1 / std::sqrt(ei * 2. * PhysicalConstants::meV /
                                PhysicalConstants::NeutronMass);
}

/**
 * @brief Calculate corrections to get a Time at Sample for a DG instrument.
 * @return Correction struct
 */
Correction Mantid::Algorithms::TimeAtSampleStrategyDirect::calculate(
    const size_t &) const {

  // Correction is L1 and Ei dependent only. Detector positions are not
  // required.
  return Correction(0, m_constShift);
}

} // namespace Algorithms
} // namespace Mantid
