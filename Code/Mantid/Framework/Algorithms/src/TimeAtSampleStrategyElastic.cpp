#include "MantidAlgorithms/TimeAtSampleStrategyElastic.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/V3D.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;

namespace {
/**
 * For detectors (not monitor detectors). neutrons interact with the sample
 *first. so the ratio we want to
 * calculate is L1 / (L1 + L2) in order to calculate a TatSample for a neturon
 *based on it's recorded TOF at the detector.
 *
 * For monitors. The L2 scattering distance is of no consequence. The ratio we
 *want to calculate is L1m / L1s where L1m
 * is the L1 for the monitor, and L1s is the L1 for the sample.
 *
 *
 * @param detector : Detector
 * @param source : Source
 * @param L1s : L1 distance Source - Sample
 * @return Calculated ratio
 */
double calculateTOFRatio(const IDetector &detector, const IComponent &source,
                         const IComponent &sample, const double &L1s,
                         const V3D &beamDir) {
  if (detector.isMonitor()) {
    double L1m = beamDir.scalar_prod(source.getPos() - detector.getPos());
    return std::abs(L1s / L1m);
  } else {
    const double L2 = sample.getPos().distance(detector.getPos());
    return L1s / (L1s + L2);
  }
}
}

namespace Mantid {
namespace Algorithms {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
TimeAtSampleStrategyElastic::TimeAtSampleStrategyElastic(
    Mantid::API::MatrixWorkspace_const_sptr ws)
    : m_ws(ws) {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
TimeAtSampleStrategyElastic::~TimeAtSampleStrategyElastic() {}

/**
 * @brief Calculate correction
 * @param workspace_index
 * @return Correction
 */
Correction
TimeAtSampleStrategyElastic::calculate(const size_t &workspace_index) const {
  auto instrument = m_ws->getInstrument();
  auto source = instrument->getSource();
  auto sample = instrument->getSample();
  const double L1s = source->getDistance(*sample);
  auto refFrame = instrument->getReferenceFrame();
  const V3D &beamDir = refFrame->vecPointingAlongBeam();
  const double factor = calculateTOFRatio(*m_ws->getDetector(workspace_index),
                                          *source, *sample, L1s, beamDir);
  return Correction(0, factor);
}

} // namespace Algorithms
} // namespace Mantid
