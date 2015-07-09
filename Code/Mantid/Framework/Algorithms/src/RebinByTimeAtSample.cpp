#include "MantidAlgorithms/RebinByTimeAtSample.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include <boost/make_shared.hpp>
#include <algorithm>
#include <cmath>

namespace Mantid {
namespace Algorithms {
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RebinByTimeAtSample)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
RebinByTimeAtSample::RebinByTimeAtSample() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
RebinByTimeAtSample::~RebinByTimeAtSample() {}

//----------------------------------------------------------------------------------------------

/// Algorithm's version for identification. @see Algorithm::version
int RebinByTimeAtSample::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RebinByTimeAtSample::category() const {
  return "Transforms\\Rebin;Events\\EventFiltering";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string RebinByTimeAtSample::summary() const {
  return "Rebins with an x-axis of relative time at sample for comparing event "
         "arrival time at the sample environment.";
}

const std::string RebinByTimeAtSample::name() const {
  return "RebinByTimeAtSample";
}

/**
 * For detectors (not monitor detectors). neutrons interact with the sample first. so the ratio we want to
 * calculate is L1 / (L1 + L2) in order to calculate a TatSample for a neturon based on it's recorded TOF at the detector.
 *
 * For monitors. The L2 scattering distance is of no consequence. The ratio we want to calculate is L1m / L1s where L1m
 * is the L1 for the monitor, and L1s is the L1 for the sample.
 *
 *
 * @param detector : Detector
 * @param source : Source
 * @param L1s : L1 distance Source - Sample
 * @return Calculated ratio
 */
double RebinByTimeAtSample::calculateTOFRatio(const Geometry::IDetector& detector, const Geometry::IComponent& source, const Geometry::IComponent& sample,
                         const double& L1s, const V3D& beamDir){
    if(detector.isMonitor()){
       double L1m = beamDir.scalar_prod(source.getPos() - detector.getPos());
       return std::abs(L1s / L1m);
    } else {
       const double L2 = sample.getPos().distance(detector.getPos());
       return L1s / (L1s + L2);
    }
}

/**
 * Do histogramming of the data to create the output workspace.
 * @param inWS : input workspace
 * @param outputWS : output workspace
 * @param XValues_new : Pointer to new x values vector (cowp)
 * @param OutXValues_scaled : Vector of new x values
 * @param prog : Progress object
 */
void RebinByTimeAtSample::doHistogramming(IEventWorkspace_sptr inWS,
                                          MatrixWorkspace_sptr outputWS,
                                          MantidVecPtr &XValues_new,
                                          MantidVec &OutXValues_scaled,
                                          Progress &prog) {
  const int histnumber = static_cast<int>(inWS->getNumberHistograms());

  const double tofOffset = 0;
  auto instrument = inWS->getInstrument();
  auto source = instrument->getSource();
  auto sample = instrument->getSample();
  const double L1s = source->getDistance(*sample);
  auto refFrame = instrument->getReferenceFrame();
  const V3D& beamDir = refFrame->vecPointingAlongBeam();

  // Go through all the histograms and set the data
  PARALLEL_FOR2(inWS, outputWS)
  for (int i = 0; i < histnumber; ++i) {
    PARALLEL_START_INTERUPT_REGION

    const double tofFactor = calculateTOFRatio(*inWS->getDetector(i), *source, *sample, L1s, beamDir);

    const IEventList *el = inWS->getEventListPtr(i);
    MantidVec y_data, e_data;
    // The EventList takes care of histogramming.
    el->generateHistogramTimeAtSample(*XValues_new, y_data, e_data, tofFactor,
                                      tofOffset);

    // Set the X axis for each output histogram
    outputWS->setX(i, OutXValues_scaled);

    // Copy the data over.
    outputWS->dataY(i).assign(y_data.begin(), y_data.end());
    outputWS->dataE(i).assign(e_data.begin(), e_data.end());

    // Report progress
    prog.report(name());
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION
}

/**
 * get Maximum x value across all spectrum
 * @param ws : workspace to inspect
 * @return max time since epoch in nanoseconds.
 */
uint64_t
RebinByTimeAtSample::getMaxX(Mantid::API::IEventWorkspace_sptr ws) const {
  return ws->getTimeAtSampleMax().totalNanoseconds();
}

/**
 * get Minimum x value across all spectrum
 * @param ws : workspace to inspect
 * @return min time since epoch in nanoseconds.
 */
uint64_t
RebinByTimeAtSample::getMinX(Mantid::API::IEventWorkspace_sptr ws) const {
  return ws->getTimeAtSampleMin().totalNanoseconds();
}

} // namespace Algorithms
} // namespace Mantid
