#include "MantidAlgorithms/RebinByTimeAtSample.h"

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidKernel/Unit.h"
#include <boost/make_shared.hpp>
#include <algorithm>

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
  const double L1 = source->getDistance(*sample);

  // Go through all the histograms and set the data
  PARALLEL_FOR2(inWS, outputWS)
  for (int i = 0; i < histnumber; ++i) {
    PARALLEL_START_INTERUPT_REGION

    const double L2 = inWS->getDetector(i)->getDistance(*sample);
    const double tofFactor = L1 / (L1 + L2);

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
