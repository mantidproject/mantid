//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/MaskBins.h"
#include "MantidAPI/HistogramValidator.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"

#include <limits>
#include <sstream>

using Mantid::HistogramData::BinEdges;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MaskBins)

using namespace Kernel;
using namespace API;
using namespace Mantid;
using Mantid::DataObjects::EventList;
using Mantid::DataObjects::EventWorkspace;
using Mantid::DataObjects::EventWorkspace_sptr;
using Mantid::DataObjects::EventWorkspace_const_sptr;

MaskBins::MaskBins() : API::Algorithm(), m_startX(0.0), m_endX(0.0) {}

void MaskBins::init() {
  declareProperty(
      make_unique<WorkspaceProperty<>>(
          "InputWorkspace", "", Direction::Input,
          boost::make_shared<HistogramValidator>()),
      "The name of the input workspace. Must contain histogram data.");
  declareProperty(make_unique<WorkspaceProperty<>>("OutputWorkspace", "",
                                                   Direction::Output),
                  "The name of the Workspace containing the masked bins.");

  // This validator effectively makes these properties mandatory
  // Would be nice to have an explicit validator for this, but
  // MandatoryValidator is already taken!
  auto required = boost::make_shared<BoundedValidator<double>>();
  required->setUpper(std::numeric_limits<double>::max() * 0.99);
  declareProperty("XMin", std::numeric_limits<double>::max(), required,
                  "The value to start masking from.");
  declareProperty("XMax", std::numeric_limits<double>::max(), required,
                  "The value to end masking at.");

  // which pixels to load
  this->declareProperty(make_unique<ArrayProperty<int>>("SpectraList"),
                        "Optional: A list of individual which spectra to mask "
                        "(specified using the workspace index). If not set, "
                        "all spectra are masked. Can be entered as a "
                        "comma-seperated list of values, or a range (such as "
                        "'a-b' which will include spectra with workspace index "
                        "of a to b inclusively).");
}

/** Execution code.
 *  @throw std::invalid_argument If XMax is less than XMin
 */
void MaskBins::exec() {
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");

  // Check for valid X limits
  m_startX = getProperty("XMin");
  m_endX = getProperty("XMax");

  if (m_startX > m_endX) {
    std::stringstream msg;
    msg << "XMax (" << m_endX << ") must be greater than XMin (" << m_startX
        << ")";
    g_log.error(msg.str());
    throw std::invalid_argument(msg.str());
  }

  //---------------------------------------------------------------------------------
  // what spectra (workspace indices) to load. Optional.
  this->spectra_list = this->getProperty("SpectraList");
  if (!this->spectra_list.empty()) {
    const int numHist = static_cast<int>(inputWS->getNumberHistograms());
    //--- Validate spectra list ---
    for (auto wi : this->spectra_list) {
      if ((wi < 0) || (wi >= numHist)) {
        std::ostringstream oss;
        oss << "One of the workspace indices specified, " << wi
            << " is above the number of spectra in the workspace (" << numHist
            << ").";
        throw std::invalid_argument(oss.str());
      }
    }
  }

  // Only create the output workspace if it's different to the input one
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    outputWS = inputWS->clone();
    setProperty("OutputWorkspace", outputWS);
  }

  //---------------------------------------------------------------------------------
  // Now, determine if the input workspace is actually an EventWorkspace
  EventWorkspace_const_sptr eventW =
      boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);

  if (eventW != nullptr) {
    //------- EventWorkspace ---------------------------
    this->execEvent();
  } else {
    //------- MatrixWorkspace of another kind -------------
    MantidVec::difference_type startBin(0), endBin(0);

    // If the binning is the same throughout, we only need to find the index
    // limits once
    const bool commonBins = WorkspaceHelpers::commonBoundaries(*inputWS);
    if (commonBins) {
      auto X = inputWS->binEdges(0);
      this->findIndices(X, startBin, endBin);
    }

    const int numHists = static_cast<int>(inputWS->getNumberHistograms());
    Progress progress(this, 0.0, 1.0, numHists);
    // Parallel running has problems with a race condition, leading to
    // occaisional test failures and crashes

    bool useSpectraList = (!this->spectra_list.empty());

    // Alter the for loop ending based on what we are looping on
    int for_end = numHists;
    if (useSpectraList)
      for_end = static_cast<int>(this->spectra_list.size());

    for (int i = 0; i < for_end; ++i) {
      // Find the workspace index, either based on the spectra list or all
      // spectra
      int wi;
      if (useSpectraList)
        wi = this->spectra_list[i];
      else
        wi = i;

      MantidVec::difference_type startBinLoop(startBin), endBinLoop(endBin);
      if (!commonBins)
        this->findIndices(outputWS->binEdges(wi), startBinLoop, endBinLoop);

      // Loop over masking each bin in the range
      for (int j = static_cast<int>(startBinLoop);
           j < static_cast<int>(endBinLoop); ++j) {
        outputWS->maskBin(wi, j);
      }
      progress.report();

    } // ENDFOR(i)
  }   // ENDIFELSE(eventworkspace?)
}

/** Execution code for EventWorkspaces
 */
void MaskBins::execEvent() {
  MatrixWorkspace_sptr outputMatrixWS = getProperty("OutputWorkspace");
  auto outputWS = boost::dynamic_pointer_cast<EventWorkspace>(outputMatrixWS);

  // set up the progress bar
  const size_t numHists = outputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHists * 2);

  // sort the events
  outputWS->sortAll(Mantid::DataObjects::TOF_SORT, &progress);

  // Go through all histograms
  if (!this->spectra_list.empty()) {
    // Specific spectra were specified
    PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
    for (int i = 0; i < static_cast<int>(this->spectra_list.size()); // NOLINT
         ++i) {
      PARALLEL_START_INTERUPT_REGION
      outputWS->getSpectrum(this->spectra_list[i]).maskTof(m_startX, m_endX);
      progress.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  } else {
    // Do all spectra!
    PARALLEL_FOR_IF(Kernel::threadSafe(*outputWS))
    for (int64_t i = 0; i < int64_t(numHists); ++i) {
      PARALLEL_START_INTERUPT_REGION
      outputWS->getSpectrum(i).maskTof(m_startX, m_endX);
      progress.report();
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION
  }

  // Clear the MRU
  outputWS->clearMRU();
}

/** Finds the indices of the bins at the limits of the range given.
 *  @param X ::        The X vector to search
 *  @param startBin :: Returns the bin index including the starting value
 *  @param endBin ::   Returns the bin index after the end value
 */
void MaskBins::findIndices(const BinEdges &X,
                           MantidVec::difference_type &startBin,
                           MantidVec::difference_type &endBin) {
  startBin = std::distance(X.begin(),
                           std::upper_bound(X.cbegin(), X.cend(), m_startX));
  if (startBin != 0)
    --startBin;
  auto last = std::lower_bound(X.cbegin(), X.cend(), m_endX);
  if (last == X.cend())
    --last;
  endBin = std::distance(X.cbegin(), last);
}

} // namespace Algorithms
} // namespace Mantid
