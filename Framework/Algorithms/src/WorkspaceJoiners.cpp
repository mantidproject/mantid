//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/WorkspaceJoiners.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Constructor
 */
WorkspaceJoiners::WorkspaceJoiners() : Algorithm(), m_progress(NULL) {}

/** Destructor
 */
WorkspaceJoiners::~WorkspaceJoiners() { delete m_progress; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string WorkspaceJoiners::category() const {
  return "Transforms\\Merging";
}

/** Executes the algorithm for histogram workspace inputs
 *  @returns The result workspace
 */
MatrixWorkspace_sptr
WorkspaceJoiners::execWS2D(API::MatrixWorkspace_const_sptr ws1,
                           API::MatrixWorkspace_const_sptr ws2) {
  // Create the output workspace
  const size_t totalHists =
      ws1->getNumberHistograms() + ws2->getNumberHistograms();
  MatrixWorkspace_sptr output = WorkspaceFactory::Instance().create(
      "Workspace2D", totalHists, ws1->readX(0).size(), ws1->readY(0).size());
  // Copy over stuff from first input workspace. This will include the spectrum
  // masking
  WorkspaceFactory::Instance().initializeFromParent(ws1, output, true);

  // Create the X values inside a cow pointer - they will be shared in the
  // output workspace
  cow_ptr<MantidVec> XValues;
  XValues.access() = ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  // Loop over the input workspaces in turn copying the data into the output one
  const int64_t &nhist1 = ws1->getNumberHistograms();
  PARALLEL_FOR2(ws1, output)
  for (int64_t i = 0; i < nhist1; ++i) {
    PARALLEL_START_INTERUPT_REGION
    ISpectrum *outSpec = output->getSpectrum(i);
    const ISpectrum *inSpec = ws1->getSpectrum(i);

    // Copy X,Y,E
    outSpec->setX(XValues);
    outSpec->setData(inSpec->dataY(), inSpec->dataE());
    // Copy the spectrum number/detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Propagate binmasking, if needed
    if (ws1->hasMaskedBins(i)) {
      const MatrixWorkspace::MaskList &inputMasks = ws1->maskedBins(i);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it) {
        output->flagMasked(i, (*it).first, (*it).second);
      }
    }

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // For second loop we use the offset from the first
  const int64_t &nhist2 = ws2->getNumberHistograms();

  PARALLEL_FOR2(ws2, output)
  for (int64_t j = 0; j < nhist2; ++j) {
    PARALLEL_START_INTERUPT_REGION
    // The spectrum in the output workspace
    ISpectrum *outSpec = output->getSpectrum(nhist1 + j);
    // Spectrum in the second workspace
    const ISpectrum *inSpec = ws2->getSpectrum(j);

    // Copy X,Y,E
    outSpec->setX(XValues);
    outSpec->setData(inSpec->dataY(), inSpec->dataE());
    // Copy the spectrum number/detector IDs
    outSpec->copyInfoFrom(*inSpec);

    // Propagate masking, if needed
    if (ws2->hasMaskedBins(j)) {
      const MatrixWorkspace::MaskList &inputMasks = ws2->maskedBins(j);
      MatrixWorkspace::MaskList::const_iterator it;
      for (it = inputMasks.begin(); it != inputMasks.end(); ++it) {
        output->flagMasked(nhist1 + j, (*it).first, (*it).second);
      }
    }
    // Propagate spectrum masking
    Geometry::IDetector_const_sptr ws2Det;
    try {
      ws2Det = ws2->getDetector(j);
    } catch (Exception::NotFoundError &) {
    }
    if (ws2Det && ws2Det->isMasked()) {
      output->maskWorkspaceIndex(nhist1 + j);
    }

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  fixSpectrumNumbers(ws1, ws2, output);

  return output;
}

/** Executes the algorithm for event workspace inputs
 *  @returns The result workspace
 *  @throw std::invalid_argument If the input workspaces do not meet the
 * requirements of this algorithm
 */
MatrixWorkspace_sptr WorkspaceJoiners::execEvent() {
  // Create the output workspace
  const size_t totalHists =
      event_ws1->getNumberHistograms() + event_ws2->getNumberHistograms();
  // Have the minimum # of histograms in the output.
  EventWorkspace_sptr output = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 1,
                                          event_ws1->readX(0).size(),
                                          event_ws1->readY(0).size()));
  // Copy over geometry (but not data) from first input workspace
  WorkspaceFactory::Instance().initializeFromParent(event_ws1, output, true);

  // Create the X values inside a cow pointer - they will be shared in the
  // output workspace
  cow_ptr<MantidVec> XValues;
  XValues.access() = event_ws1->readX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  const int64_t &nhist1 = event_ws1->getNumberHistograms();
  for (int64_t i = 0; i < nhist1; ++i) {
    // Copy the events over
    output->getOrAddEventList(i) =
        event_ws1->getEventList(i); // Should fire the copy constructor
    ISpectrum *outSpec = output->getSpectrum(i);
    const ISpectrum *inSpec = event_ws1->getSpectrum(i);
    outSpec->copyInfoFrom(*inSpec);

    m_progress->report();
  }

  // For second loop we use the offset from the first
  const int64_t &nhist2 = event_ws2->getNumberHistograms();
  for (int64_t j = 0; j < nhist2; ++j) {
    // This is the workspace index at which we assign in the output
    int64_t output_wi = j + nhist1;
    // Copy the events over
    output->getOrAddEventList(output_wi) =
        event_ws2->getEventList(j); // Should fire the copy constructor
    ISpectrum *outSpec = output->getSpectrum(output_wi);
    const ISpectrum *inSpec = event_ws2->getSpectrum(j);
    outSpec->copyInfoFrom(*inSpec);

    // Propagate spectrum masking. First workspace will have been done by the
    // factory
    Geometry::IDetector_const_sptr ws2Det;
    try {
      ws2Det = event_ws2->getDetector(j);
    } catch (Exception::NotFoundError &) {
    }
    if (ws2Det && ws2Det->isMasked()) {
      output->maskWorkspaceIndex(output_wi);
    }

    m_progress->report();
  }

  // Set the same bins for all output pixels
  output->setAllX(XValues);

  fixSpectrumNumbers(event_ws1, event_ws2, output);

  return output;
}

/** Checks that the two input workspace have common binning & size, the same
 * instrument & unit.
 *  Also calls the checkForOverlap method.
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @throw std::invalid_argument If the workspaces are not compatible
 */
void WorkspaceJoiners::validateInputs(API::MatrixWorkspace_const_sptr ws1,
                                      API::MatrixWorkspace_const_sptr ws2) {
  // This is the full check for common binning
  if (!WorkspaceHelpers::commonBoundaries(ws1) ||
      !WorkspaceHelpers::commonBoundaries(ws2)) {
    g_log.error(
        "Both input workspaces must have common binning for all their spectra");
    throw std::invalid_argument(
        "Both input workspaces must have common binning for all their spectra");
  }

  if (ws1->getInstrument()->getName() != ws2->getInstrument()->getName()) {
    const std::string message("The input workspaces are not compatible because "
                              "they come from different instruments");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  Unit_const_sptr ws1_unit = ws1->getAxis(0)->unit();
  Unit_const_sptr ws2_unit = ws2->getAxis(0)->unit();
  const std::string ws1_unitID = (ws1_unit ? ws1_unit->unitID() : "");
  const std::string ws2_unitID = (ws2_unit ? ws2_unit->unitID() : "");

  if (ws1_unitID != ws2_unitID) {
    const std::string message("The input workspaces are not compatible because "
                              "they have different units on the X axis");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (ws1->isDistribution() != ws2->isDistribution()) {
    const std::string message(
        "The input workspaces have inconsistent distribution flags");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (!WorkspaceHelpers::matchingBins(ws1, ws2, true)) {
    const std::string message("The input workspaces are not compatible because "
                              "they have different binning");
    g_log.error(message);
    throw std::invalid_argument(message);
  }
}

/**
 * Determine the minimum and maximum spectra ids.
 *
 * @param ws the workspace to search
 * @param min The minimum id (output).
 * @param max The maximum id (output).
 */
void WorkspaceJoiners::getMinMax(MatrixWorkspace_const_sptr ws, specid_t &min,
                                 specid_t &max) {
  specid_t temp;
  size_t length = ws->getNumberHistograms();
  // initial values
  min = max = ws->getSpectrum(0)->getSpectrumNo();
  for (size_t i = 0; i < length; i++) {
    temp = ws->getSpectrum(i)->getSpectrumNo();
    // Adjust min/max
    if (temp < min)
      min = temp;
    if (temp > max)
      max = temp;
  }
}

} // namespace Algorithms
} // namespace Mantid
