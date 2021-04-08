// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/CalculateEfficiency2.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/BoundedValidator.h"
#include <cmath>
#include <limits>
#include <vector>

namespace Mantid {
namespace Algorithms {

// Register the class into the algorithm factory
DECLARE_ALGORITHM(CalculateEfficiency2)

using namespace Kernel;
using namespace API;
using namespace Geometry;
using namespace DataObjects;

/// A private namespace for property names.
namespace PropertyNames {
const static std::string INPUT_WORKSPACE{"InputWorkspace"};
const static std::string OUTPUT_WORKSPACE{"OutputWorkspace"};
const static std::string MIN_THRESHOLD{"MinThreshold"};
const static std::string MAX_THRESHOLD{"MaxThreshold"};
const static std::string MERGE_GROUP{"MergeGroup"};
} // namespace PropertyNames

namespace { // anonymous
static void applyBadPixelThreshold(MatrixWorkspace &outputWS, double minThreshold, double maxThreshold) {

  // Number of spectra
  const size_t numberOfSpectra = outputWS.getNumberHistograms();
  const auto &spectrumInfo = outputWS.spectrumInfo();

  for (size_t i = 0; i < numberOfSpectra; i++) {
    auto &YOut = outputWS.mutableY(i);
    auto &EOut = outputWS.mutableE(i);

    // Skip if we have a monitor or if the detector is masked.
    if (spectrumInfo.isMonitor(i)) {
      YOut.front() = 1.0;
      EOut.front() = 0.0;
      continue;
    } else if (spectrumInfo.isMasked(i)) {
      continue;
    }

    // if the pixel is outside the thresholds let make it EMPTY_DBL
    // In the documentation is "-inf"
    const auto y = YOut.front();
    if (y < minThreshold || y > maxThreshold) {
      YOut.front() = EMPTY_DBL();
      EOut.front() = EMPTY_DBL();
    }
  }
}
} // anonymous namespace

/** Initialization method.
 *
 */
void CalculateEfficiency2::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(PropertyNames::INPUT_WORKSPACE, "", Direction::Input),
                  "The workspace containing the flood data");
  declareProperty(std::make_unique<WorkspaceProperty<>>(PropertyNames::OUTPUT_WORKSPACE, "", Direction::Output,
                                                        PropertyMode::Optional),
                  "The name of the workspace to be created as the output of the algorithm");

  auto positiveDouble = std::make_shared<BoundedValidator<double>>();
  positiveDouble->setLower(0);
  declareProperty(PropertyNames::MIN_THRESHOLD, 0.0, positiveDouble, "Minimum threshold for a pixel to be considered");
  declareProperty(PropertyNames::MAX_THRESHOLD, 2.0, positiveDouble->clone(),
                  "Maximum threshold for a pixel to be considered");

  declareProperty(PropertyNames::MERGE_GROUP, false,
                  "Whether to merge entries when WorkspaceGroup is specified as input.");
}

std::map<std::string, std::string> CalculateEfficiency2::validateInputs() {
  std::map<std::string, std::string> result;

  // Files from time-of-flight instruments must be integrated in Lambda before
  // using this algorithm

  auto oneBinMsg = "Input workspace must have only one bin. Consider "
                   "integrating the input over all the bins.";
  Workspace_const_sptr ws1 = getProperty(PropertyNames::INPUT_WORKSPACE);
  MatrixWorkspace_const_sptr inputWS = std::dynamic_pointer_cast<const MatrixWorkspace>(ws1);
  if (inputWS == nullptr) {
    WorkspaceGroup_const_sptr inputGroup = std::dynamic_pointer_cast<const WorkspaceGroup>(ws1);
    if (inputGroup != nullptr) {
      for (auto entryNo = 0; entryNo < inputGroup->getNumberOfEntries(); ++entryNo) {
        auto const entry = std::static_pointer_cast<const MatrixWorkspace>(inputGroup->getItem(entryNo));
        if (entry->blocksize() > 1) {
          result[PropertyNames::INPUT_WORKSPACE] = oneBinMsg;
          break;
        }
      }
    } else {
      result[PropertyNames::INPUT_WORKSPACE] = "The input property must be either MatrixWorkspace or a "
                                               "WorkspaceGroup containing MatrixWorkspaces";
    }
  } else if (inputWS->blocksize() > 1) {
    result[PropertyNames::INPUT_WORKSPACE] = oneBinMsg;
  }

  if (getPropertyValue(PropertyNames::OUTPUT_WORKSPACE) == "") {
    result[PropertyNames::OUTPUT_WORKSPACE] = "The output workspace name must be specified.";
  }

  // get the thresholds once to error check and use in the main function
  m_minThreshold = getProperty("MinThreshold");
  m_maxThreshold = getProperty("MaxThreshold");
  if (m_minThreshold >= m_maxThreshold) {
    const std::string msg{"MinThreshold must be less than MaxThreshold"};
    result[PropertyNames::MIN_THRESHOLD] = msg;
    result[PropertyNames::MAX_THRESHOLD] = msg;
  }

  return result;
}

/** Executes the algorithm
 *
 */
void CalculateEfficiency2::exec() {
  Workspace_sptr ws1 = getProperty(PropertyNames::INPUT_WORKSPACE);
  auto inputWS = std::static_pointer_cast<MatrixWorkspace>(ws1);
  auto outputWS = calculateEfficiency(inputWS);
  setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWS);
  progress(1.0, "Done!");
}

API::MatrixWorkspace_sptr CalculateEfficiency2::calculateEfficiency(MatrixWorkspace_sptr inputWorkspace,
                                                                    double startProgress, double stepProgress) {

  // create the output workspace from the input, while NOT preserving events
  auto childAlg = createChildAlgorithm("RebinToWorkspace", 0.0, 0.1);
  childAlg->setProperty("WorkspaceToRebin", inputWorkspace);
  childAlg->setProperty("WorkspaceToMatch", inputWorkspace);
  childAlg->setPropertyValue("OutputWorkspace", getPropertyValue(PropertyNames::OUTPUT_WORKSPACE));
  childAlg->setProperty("PreserveEvents", false);
  childAlg->executeAsChildAlg();
  MatrixWorkspace_sptr outputWS = childAlg->getProperty("OutputWorkspace");

  // Loop over spectra and sum all the counts to get normalization
  // Skip monitors and masked detectors
  // returns tuple with (sum, err, npixels)
  progress(startProgress + 0.1 * stepProgress, "Computing the counts.");
  auto counts = sumUnmaskedAndDeadPixels(*outputWS);
  if (counts.nPixels == 0) {
    throw std::runtime_error("No pixels being used for calculation");
  }

  progress(startProgress + 0.3 * stepProgress, "Normalising the detectors.");
  averageAndNormalizePixels(*outputWS, counts);

  progress(startProgress + 0.5 * stepProgress, "Applying bad pixel threshold.");
  applyBadPixelThreshold(*outputWS, m_minThreshold, m_maxThreshold);

  // do it again only using the pixels that are within the threshold
  progress(startProgress + 0.7 * stepProgress, "Computing the counts.");
  counts = sumUnmaskedAndDeadPixels(*outputWS);
  if (counts.nPixels == 0) {
    throw std::runtime_error("All pixels are outside of the threshold values");
  }

  progress(startProgress + 0.9 * stepProgress, "Normalising the detectors.");
  averageAndNormalizePixels(*outputWS, counts);

  return outputWS;
}

/**
 * Explicitly calls validateInputs and throws runtime error in case
 * of issues in the input properties.
 *
 */
void CalculateEfficiency2::validateGroupInput() {
  auto results = validateInputs();
  for (const auto &result : results) {
    throw std::runtime_error("Issue in " + result.first + " property: " + result.second);
  }
}

/**
 * Process groups and merge entries if MergeGroup property is set.
 *
 * @return A boolean true if execution was sucessful, false otherwise
 */
bool CalculateEfficiency2::processGroups() {

  // if run as a script, validateInputs will not be triggered
  // for the processGroups, so properties will be validated manually
  validateGroupInput();

  Workspace_sptr ws1 = getProperty(PropertyNames::INPUT_WORKSPACE);
  WorkspaceGroup_sptr inputWS = std::static_pointer_cast<WorkspaceGroup>(ws1);

  const bool mergeGroups = getProperty(PropertyNames::MERGE_GROUP);
  if (mergeGroups) {
    auto mergedWS = mergeGroup(*inputWS);
    auto outputWS = calculateEfficiency(mergedWS);
    setProperty(PropertyNames::OUTPUT_WORKSPACE, outputWS);
  } else {
    auto outputGroup = std::make_shared<WorkspaceGroup>();
    auto const nEntries = inputWS->getNumberOfEntries();
    auto const stepProgress = 1.0 / nEntries;
    for (auto entryNo = 0; entryNo < nEntries; ++entryNo) {
      auto entryWS = std::static_pointer_cast<API::MatrixWorkspace>(inputWS->getItem(entryNo));
      auto const startProgress = static_cast<double>(entryNo) / nEntries;
      auto outputWS = calculateEfficiency(entryWS, startProgress, stepProgress);
      outputGroup->addWorkspace(outputWS);
    }
    const std::string groupName = getPropertyValue(PropertyNames::OUTPUT_WORKSPACE);
    AnalysisDataService::Instance().addOrReplace(groupName, outputGroup);
    setProperty(PropertyNames::OUTPUT_WORKSPACE, outputGroup);
  }
  progress(1.0, "Done!");
  return true;
}

/**
 *  Sum up all the unmasked detector pixels.
 *
 * @param workspace: workspace where all the wavelength bins have been grouped
 */
SummedResults CalculateEfficiency2::sumUnmaskedAndDeadPixels(const MatrixWorkspace &workspace) {
  // Number of spectra
  const size_t numberOfSpectra = workspace.getNumberHistograms();
  SummedResults results;

  const auto &spectrumInfo = workspace.spectrumInfo();
  for (size_t i = 0; i < numberOfSpectra; i++) {

    // Retrieve the spectrum into a vector
    auto &YValues = workspace.y(i);
    auto &YErrors = workspace.e(i);

    // Skip if we have a monitor, if the detector is masked or if the pixel is
    // dead
    if (spectrumInfo.isMonitor(i) || spectrumInfo.isMasked(i) || isEmpty(YValues.front()))
      continue;

    results.sum += YValues.front();
    results.error += YErrors.front() * YErrors.front();
    results.nPixels++;
  }
  results.error = std::sqrt(results.error);

  g_log.debug() << "Total of unmasked/dead pixels = " << results.nPixels << " from a total of " << numberOfSpectra
                << "\n";

  return results;
}

void CalculateEfficiency2::averageAndNormalizePixels(MatrixWorkspace &workspace, const SummedResults &counts) {

  // Number of spectra
  const size_t numberOfSpectra = workspace.getNumberHistograms();
  const auto &spectrumInfo = workspace.spectrumInfo();
  // Calculate the averages
  const double averageY = counts.sum / static_cast<double>(counts.nPixels);
  const double averageE = counts.error / static_cast<double>(counts.nPixels);

  for (size_t i = 0; i < numberOfSpectra; i++) {

    auto &y = workspace.mutableY(i);
    auto &e = workspace.mutableE(i);
    const auto yOriginal = y.front();

    // Skip if we have a monitor, the detector is masked, or it has already been
    // marked as outside of the threashold by being set to EMPTY_DBL
    if (spectrumInfo.isMasked(i) || spectrumInfo.isMonitor(i) || isEmpty(yOriginal))
      continue;

    const auto eOriginal = e.front();

    // Normalize counts
    y.front() = yOriginal / averageY;
    const double signalToNoiseOrig = eOriginal / yOriginal;
    const double signalToNoiseAvg = averageE / averageY;
    e.front() = y.front() * std::sqrt((signalToNoiseOrig * signalToNoiseOrig) + (signalToNoiseAvg * signalToNoiseAvg));
  }

  g_log.debug() << "Averages :: counts = " << averageY << "; error = " << averageE << "\n";
}

/**
 *  Merges the input workspace group and tries to remove masked spectra
 *  and replace the masked value with an average of counts from non-masked
 *  entries.
 *
 *  @param input: workspace group to be merged
 *
 *  @returns merged workspace group
 */
API::MatrixWorkspace_sptr CalculateEfficiency2::mergeGroup(API::WorkspaceGroup &input) {
  auto nEntries = input.getNumberOfEntries();
  auto mergeRuns = createChildAlgorithm("MergeRuns");
  mergeRuns->setProperty("InputWorkspaces", input.getName());
  mergeRuns->executeAsChildAlg();
  Workspace_sptr mergedWs = mergeRuns->getProperty("OutputWorkspace");

  auto createSingleAlg = createChildAlgorithm("CreateSingleValuedWorkspace");
  createSingleAlg->setProperty("DataValue", static_cast<double>(nEntries));
  createSingleAlg->executeAsChildAlg();
  MatrixWorkspace_sptr normWs = createSingleAlg->getProperty("OutputWorkspace");

  auto divideAlg = createChildAlgorithm("Divide");
  divideAlg->setProperty("LHSWorkspace", mergedWs);
  divideAlg->setProperty("RHSWorkspace", normWs);
  divideAlg->executeAsChildAlg();
  MatrixWorkspace_sptr mergedNormalisedWs = divideAlg->getProperty("OutputWorkspace");

  auto spectrumInfo = mergedNormalisedWs->spectrumInfo();
  PARALLEL_FOR_IF(Kernel::threadSafe(*mergedNormalisedWs))
  for (auto spectrumNo = 0; spectrumNo < static_cast<int>(mergedNormalisedWs->getNumberHistograms()); ++spectrumNo) {
    if (spectrumInfo.isMasked(spectrumNo)) {
      auto &detDataY = mergedNormalisedWs->mutableY(spectrumNo);
      auto &detDataErr = mergedNormalisedWs->mutableE(spectrumNo);
      auto dataY = 0.0;
      auto dataE = 0.0;
      auto nonMaskedEntries = 0;
      for (auto entryNo = 0; entryNo < nEntries; ++entryNo) {
        MatrixWorkspace_sptr entry = std::static_pointer_cast<API::MatrixWorkspace>(input.getItem(entryNo));
        auto spectrumInfoEntry = entry->spectrumInfo();
        if (!spectrumInfoEntry.isMasked(spectrumNo)) {
          dataY += entry->readY(spectrumNo)[0];
          dataE += pow(entry->readE(spectrumNo)[0], 2); // propagate errors
          nonMaskedEntries++;
        }
      }
      if (nonMaskedEntries != 0) {

        spectrumInfo.setMasked(spectrumNo, false);
        detDataY.front() = dataY / nonMaskedEntries;
        detDataErr.front() = sqrt(dataE) / nonMaskedEntries;
      }
    }
  }
  return mergedNormalisedWs;
}

} // namespace Algorithms
} // namespace Mantid
