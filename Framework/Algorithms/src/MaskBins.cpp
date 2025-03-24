// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAPI/Algorithm.hxx"
#include "MantidAPI/HistogramValidator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <limits>
#include <sstream>

using Mantid::HistogramData::BinEdges;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBins)

using namespace Kernel;
using namespace API;
using DataObjects::EventWorkspace;
using DataObjects::EventWorkspace_const_sptr;
using DataObjects::EventWorkspace_sptr;

void MaskBins::init() {
  declareWorkspaceInputProperties<MatrixWorkspace>("InputWorkspace",
                                                   "The name of the input workspace. Must contain histogram data.",
                                                   std::make_shared<HistogramValidator>());
  declareProperty(std::make_unique<WorkspaceProperty<>>("OutputWorkspace", "", Direction::Output),
                  "The name of the Workspace containing the masked bins.");

  // This validator effectively makes these properties mandatory
  // Would be nice to have an explicit validator for this, but
  // MandatoryValidator is already taken!
  auto required = std::make_shared<BoundedValidator<double>>();
  required->setUpper(std::numeric_limits<double>::max() * 0.99);
  declareProperty("XMin", std::numeric_limits<double>::max(), required, "The value to start masking from.");
  declareProperty("XMax", std::numeric_limits<double>::max(), required, "The value to end masking at.");

  this->declareProperty(std::make_unique<ArrayProperty<int64_t>>("SpectraList"),
                        "Deprecated, use InputWorkspaceIndexSet.");
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void MaskBins::exec() {
  // Check for valid X limits
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX) {
    std::stringstream msg;
    msg << "XMax (" << m_endX << ") must be greater than XMin (" << m_startX << ")";
    g_log.error(msg.str());
    throw std::invalid_argument(msg.str());
  }

  // Copy indices from legacy property
  std::vector<int64_t> spectraList = this->getProperty("SpectraList");
  if (!spectraList.empty()) {
    if (!isDefault("InputWorkspaceIndexSet"))
      throw std::runtime_error("Cannot provide both InputWorkspaceIndexSet and "
                               "SpectraList at the same time.");
    setProperty("InputWorkspaceIndexSet", spectraList);
    g_log.warning("The 'SpectraList' property is deprecated. Use "
                  "'InputWorkspaceIndexSet' instead.");
  }

  MatrixWorkspace_sptr inputWS;
  std::tie(inputWS, indexSet) = getWorkspaceAndIndices<MatrixWorkspace>("InputWorkspace");

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  if (std::dynamic_pointer_cast<const EventWorkspace>(inputWS)) {
    this->execEvent();
  } else {
    MantidVec::difference_type startBin(0), endBin(0);

    // If the binning is the same throughout, we only need to find the index
    // limits once
    const bool commonBins = inputWS->isCommonBins();
    if (commonBins) {
      auto X = inputWS->binEdges(0);
      this->findIndices(X, startBin, endBin);
    }

    Progress progress(this, 0.0, 1.0, indexSet.size());
    // Parallel running has problems with a race condition, leading to
    // occaisional test failures and crashes

    for (const auto wi : indexSet) {
      MantidVec::difference_type startBinLoop(startBin), endBinLoop(endBin);
      if (!commonBins)
        this->findIndices(outputWS->binEdges(wi), startBinLoop, endBinLoop);

      // Loop over masking each bin in the range
      for (auto j = static_cast<int>(startBinLoop); j < static_cast<int>(endBinLoop); ++j) {
        outputWS->maskBin(wi, j);
      }
      progress.report();
    }
  }
}

/** Execution code for EventWorkspaces
 */
void MaskBins::execEvent() {
  MatrixWorkspace_sptr outputMatrixWS = getProperty("OutputWorkspace");
  auto outputWS = std::dynamic_pointer_cast<EventWorkspace>(outputMatrixWS);

  Progress progress(this, 0.0, 1.0, outputWS->getNumberHistograms() * 2);

  {
    const auto timerStart = std::chrono::high_resolution_clock::now();
    outputWS->sortAll(Mantid::DataObjects::TOF_SORT, &progress);
    addTimer("sortEvents", timerStart, std::chrono::high_resolution_clock::now());
  }

  PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
  for (int i = 0; i < static_cast<int>(indexSet.size()); // NOLINT
       ++i) {
    PARALLEL_START_INTERRUPT_REGION
    outputWS->getSpectrum(indexSet[i]).maskTof(m_startX, m_endX);
    progress.report();
    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  outputWS->clearMRU();
}

/** Finds the indices of the bins at the limits of the range given.
 *  @param X ::        The X vector to search
 *  @param startBin :: Returns the bin index including the starting value
 *  @param endBin ::   Returns the bin index after the end value
 */
void MaskBins::findIndices(const BinEdges &X, MantidVec::difference_type &startBin,
                           MantidVec::difference_type &endBin) {
  startBin = std::distance(X.begin(), std::upper_bound(X.cbegin(), X.cend(), m_startX));
  if (startBin != 0)
    --startBin;
  auto last = std::lower_bound(X.cbegin(), X.cend(), m_endX);
  if (last == X.cend())
    --last;
  endBin = std::distance(X.cbegin(), last);
}

} // namespace Mantid::Algorithms
