// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/RebinByPulseTimes.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/VectorHelper.h"
#include <algorithm>
#include <boost/make_shared.hpp>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(RebinByPulseTimes)

//----------------------------------------------------------------------------------------------
/// Algorithm's name for identification. @see Algorithm::name
const std::string RebinByPulseTimes::name() const {
  return "RebinByPulseTimes";
}

/// Algorithm's version for identification. @see Algorithm::version
int RebinByPulseTimes::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string RebinByPulseTimes::category() const {
  return "Transforms\\Rebin";
}

//----------------------------------------------------------------------------------------------

/**
 * Do histogramming of the data to create the output workspace.
 * @param inWS : input workspace
 * @param outputWS : output workspace
 * @param XValues_new : Pointer to new x values vector (cowp)
 * @param OutXValues_scaled : Vector of new x values
 * @param prog : Progress object
 */
void RebinByPulseTimes::doHistogramming(IEventWorkspace_sptr inWS,
                                        MatrixWorkspace_sptr outputWS,
                                        MantidVecPtr &XValues_new,
                                        MantidVec &OutXValues_scaled,
                                        Progress &prog) {

  // workspace independent determination of length
  const auto histnumber = static_cast<int>(inWS->getNumberHistograms());

  auto x = Kernel::make_cow<HistogramData::HistogramX>(OutXValues_scaled);

  PARALLEL_FOR_IF(Kernel::threadSafe(*inWS, *outputWS))
  for (int i = 0; i < histnumber; ++i) {
    PARALLEL_START_INTERUPT_REGION

    const auto &el = inWS->getSpectrum(i);
    MantidVec y_data, e_data;
    // The EventList takes care of histogramming.
    el.generateHistogramPulseTime(*XValues_new, y_data, e_data);

    // Set the X axis for each output histogram
    outputWS->setSharedX(i, x);

    // Copy the data over.
    outputWS->mutableY(i) = std::move(y_data);
    outputWS->mutableE(i) = std::move(e_data);

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
RebinByPulseTimes::getMaxX(Mantid::API::IEventWorkspace_sptr ws) const {
  return ws->getPulseTimeMax().totalNanoseconds();
}

/**
 * get Minimum x value across all spectrum
 * @param ws : workspace to inspect
 * @return min time since epoch in nanoseconds.
 */
uint64_t
RebinByPulseTimes::getMinX(Mantid::API::IEventWorkspace_sptr ws) const {
  return ws->getPulseTimeMin().totalNanoseconds();
}

} // namespace Algorithms
} // namespace Mantid
