#include "MantidAlgorithms/WorkspaceJoiners.h"

#include "MantidAPI/Axis.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/IDetector.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Unit.h"

namespace Mantid {
namespace Algorithms {

using namespace Kernel;
using namespace API;
using namespace DataObjects;

/** Constructor
 */
WorkspaceJoiners::WorkspaceJoiners() : Algorithm(), m_progress(nullptr) {}

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
MatrixWorkspace_sptr WorkspaceJoiners::execWS2D(const MatrixWorkspace &ws1,
                                                const MatrixWorkspace &ws2) {
  // Create the output workspace
  const size_t totalHists =
      ws1.getNumberHistograms() + ws2.getNumberHistograms();
  MatrixWorkspace_sptr output = WorkspaceFactory::Instance().create(
      "Workspace2D", totalHists, ws1.x(0).size(), ws1.y(0).size());
  // Copy over stuff from first input workspace. This will include the spectrum
  // masking
  WorkspaceFactory::Instance().initializeFromParent(ws1, *output, true);

  // Create the X values inside a cow pointer - they will be shared in the
  // output workspace
  auto XValues = ws1.refX(0);

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  // Loop over the input workspaces in turn copying the data into the output one
  const int64_t &nhist1 = ws1.getNumberHistograms();
  PARALLEL_FOR_IF(Kernel::threadSafe(ws1, *output))
  for (int64_t i = 0; i < nhist1; ++i) {
    PARALLEL_START_INTERUPT_REGION
    auto &outSpec = output->getSpectrum(i);
    const auto &inSpec = ws1.getSpectrum(i);

    outSpec.setHistogram(inSpec.histogram());
    // Copy the spectrum number/detector IDs
    outSpec.copyInfoFrom(inSpec);

    // Propagate binmasking, if needed
    if (ws1.hasMaskedBins(i)) {
      for (const auto &inputMask : ws1.maskedBins(i)) {
        output->flagMasked(i, inputMask.first, inputMask.second);
      }
    }

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  // For second loop we use the offset from the first
  const int64_t &nhist2 = ws2.getNumberHistograms();
  const auto &spectrumInfo = ws2.spectrumInfo();
  auto &outSpectrumInfo = output->mutableSpectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(ws2, *output))
  for (int64_t j = 0; j < nhist2; ++j) {
    PARALLEL_START_INTERUPT_REGION
    // The spectrum in the output workspace
    auto &outSpec = output->getSpectrum(nhist1 + j);
    // Spectrum in the second workspace
    const auto &inSpec = ws2.getSpectrum(j);

    outSpec.setHistogram(inSpec.histogram());
    // Copy the spectrum number/detector IDs
    outSpec.copyInfoFrom(inSpec);

    // Propagate masking, if needed
    if (ws2.hasMaskedBins(j)) {
      for (const auto &inputMask : ws2.maskedBins(j)) {
        output->flagMasked(nhist1 + j, inputMask.first, inputMask.second);
      }
    }
    // Propagate spectrum masking
    if (spectrumInfo.hasDetectors(j) && spectrumInfo.isMasked(j)) {
      output->getSpectrum(nhist1 + j).clearData();
      PARALLEL_CRITICAL(setMasked) {
        outSpectrumInfo.setMasked(nhist1 + j, true);
      }
    }

    m_progress->report();
    PARALLEL_END_INTERUPT_REGION
  }
  PARALLEL_CHECK_INTERUPT_REGION

  fixSpectrumNumbers(ws1, ws2, *output);

  return output;
}

/** Executes the algorithm for event workspace inputs
 *  @returns The result workspace
 *  @throw std::invalid_argument If the input workspaces do not meet the
 * requirements of this algorithm
 */
DataObjects::EventWorkspace_sptr
WorkspaceJoiners::execEvent(const DataObjects::EventWorkspace &eventWs1,
                            const DataObjects::EventWorkspace &eventWs2) {
  // Create the output workspace
  const size_t totalHists =
      eventWs1.getNumberHistograms() + eventWs2.getNumberHistograms();
  auto output =
      create<EventWorkspace>(eventWs1, totalHists, eventWs1.binEdges(0));

  // Initialize the progress reporting object
  m_progress = new API::Progress(this, 0.0, 1.0, totalHists);

  const int64_t &nhist1 = eventWs1.getNumberHistograms();
  for (int64_t i = 0; i < nhist1; ++i) {
    output->getSpectrum(i) = eventWs1.getSpectrum(i);
    m_progress->report();
  }

  // For second loop we use the offset from the first
  const int64_t &nhist2 = eventWs2.getNumberHistograms();
  const auto &spectrumInfo = eventWs2.spectrumInfo();
  auto &outSpectrumInfo = output->mutableSpectrumInfo();
  for (int64_t j = 0; j < nhist2; ++j) {
    // This is the workspace index at which we assign in the output
    int64_t outputWi = j + nhist1;
    output->getSpectrum(outputWi) = eventWs2.getSpectrum(j);

    // Propagate spectrum masking. First workspace will have been done by the
    // factory
    if (spectrumInfo.hasDetectors(j) && spectrumInfo.isMasked(j)) {
      output->getSpectrum(outputWi).clearData();
      PARALLEL_CRITICAL(setMaskedEvent) {
        outSpectrumInfo.setMasked(outputWi, true);
      }
    }

    m_progress->report();
  }

  fixSpectrumNumbers(eventWs1, eventWs2, *output);

  return std::move(output);
}

/** Checks that the two input workspace have common size and the same
 * instrument & unit. There is an option to check whether their binning is
 * compatible
 *  Also calls the checkForOverlap method.
 *  @param ws1 :: The first input workspace
 *  @param ws2 :: The second input workspace
 *  @param checkBinning :: A flag for whether to check that the workspaces have
 * compatible binning (default true)
 *  @throw std::invalid_argument If the workspaces are not compatible
 */
void Mantid::Algorithms::WorkspaceJoiners::validateInputs(
    const API::MatrixWorkspace &ws1, const API::MatrixWorkspace &ws2,
    const bool checkBinning) {
  // Workspaces with point data are allowed to have different binning
  if (checkBinning) {
    // This is the full check for common binning
    if (!WorkspaceHelpers::commonBoundaries(ws1) ||
        !WorkspaceHelpers::commonBoundaries(ws2)) {
      g_log.error("Both input workspaces must have common binning for all "
                  "their spectra");
      throw std::invalid_argument("Both input workspaces must have common "
                                  "binning for all their spectra");
    }
  }

  if (ws1.getInstrument()->getName() != ws2.getInstrument()->getName()) {
    const std::string message("The input workspaces are not compatible because "
                              "they come from different instruments");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  Unit_const_sptr ws1_unit = ws1.getAxis(0)->unit();
  Unit_const_sptr ws2_unit = ws2.getAxis(0)->unit();
  const std::string ws1_unitID = (ws1_unit ? ws1_unit->unitID() : "");
  const std::string ws2_unitID = (ws2_unit ? ws2_unit->unitID() : "");

  if (ws1_unitID != ws2_unitID) {
    const std::string message("The input workspaces are not compatible because "
                              "they have different units on the X axis");
    g_log.error(message);
    throw std::invalid_argument(message);
  }

  if (ws1.isDistribution() != ws2.isDistribution()) {
    const std::string message(
        "The input workspaces have inconsistent distribution flags");
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
void WorkspaceJoiners::getMinMax(const MatrixWorkspace &ws, specnum_t &min,
                                 specnum_t &max) {
  specnum_t temp;
  size_t length = ws.getNumberHistograms();
  // initial values
  min = max = ws.getSpectrum(0).getSpectrumNo();
  for (size_t i = 0; i < length; i++) {
    temp = ws.getSpectrum(i).getSpectrumNo();
    // Adjust min/max
    if (temp < min)
      min = temp;
    if (temp > max)
      max = temp;
  }
}

} // namespace Algorithms
} // namespace Mantid
