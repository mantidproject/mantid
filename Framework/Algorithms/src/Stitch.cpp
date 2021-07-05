// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/Stitch.h"
#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

namespace {
static const std::string INPUT_WORKSPACE_PROPERTY = "InputWorkspaces";
static const std::string REFERENCE_WORKSPACE_PROPERTY = "ReferenceWorkspace";
static const std::string COMBINATION_BEHAVIOUR_PROPERTY = "CombinationBehaviour";
static const std::string SCALE_FACTOR_CALCULATION_PROPERTY = "ScaleFactorCalculation";
static const std::string MANUAL_SCALE_FACTORS_PROPERTY = "ManualScaleFactors";
static const std::string TIE_SCALE_FACTORS_PROPERTY = "TieScaleFactors";
static const std::string OUTPUT_WORKSPACE_PROPERTY = "OutputWorkspace";
static const std::string OUTPUT_SCALE_FACTORS_PROPERTY = "OutputScaleFactorsWorkspace";

/**
 * @brief Calculates the x-axis extent of a single spectrum workspace
 * Assumes that the histogram bin edges or point centres are in ascending order
 * Ragged workspaces are not needed to be supported, so taking from the first spectrum is sufficient
 * @param ws : the input workspace
 * @return a pair of min-x and max-x
 */
std::pair<double, double> getInterval(const MatrixWorkspace &ws) {
  return std::make_pair(ws.readX(0).front(), ws.readX(0).back());
}

/**
 * @brief Compares two workspaces in terms of their x-coverage
 * @param ws1 : input workspace 1
 * @param ws2 : input workspace 2
 * @return true if ws1 is less than ws2 in terms of its x-interval
 */
bool compareInterval(const MatrixWorkspace_sptr ws1, const MatrixWorkspace_sptr ws2) {
  const auto minmax1 = getInterval(*ws1);
  const auto minmax2 = getInterval(*ws2);
  if (minmax1.first < minmax2.first) {
    return true;
  } else if (minmax1.first > minmax2.first) {
    return false;
  } else {
    return minmax1.second < minmax2.second;
  }
}

/**
 * @brief Returns the overlap of two workspaces in x-axis
 * @param ws1 : input workspace 1
 * @param ws2 : input workspace 2
 * @return the x-axis covered by both
 * @throw runtime error if there is no overlap
 */
std::pair<double, double> getOverlap(const MatrixWorkspace_sptr ws1, const MatrixWorkspace_sptr ws2) {
  const auto minmax1 = getInterval(*ws1);
  const auto minmax2 = getInterval(*ws2);
  if (minmax1.second < minmax2.first || minmax2.second < minmax1.first) {
    std::stringstream ss;
    ss << "No overlap is found between the intervals: [" << minmax1.first << "," << minmax1.second << "] and ["
       << minmax2.first << ", " << minmax2.second << "]";
    throw std::runtime_error(ss.str());
  }
  return std::make_pair(std::max(minmax1.first, minmax2.first), std::min(minmax1.second, minmax2.second));
}

/**
 * @brief Calculates the median of a vector
 * @param vec : input vector
 * @return the median if not empty, 1 otherwise
 */
double median(const std::vector<double> &vec) {
  if (vec.empty())
    return 1;
  const size_t s = vec.size();
  std::vector<double> sorted;
  sorted.reserve(s);
  std::sort(sorted.begin(), sorted.end());
  if (s % 2 == 0) {
    return 0.5 * (sorted[s / 2] + sorted[s / 2 - 1]);
  } else {
    return sorted[s / 2];
  }
}

/**
 * @brief Creates a single bin workspace containing spectrum-wise medians of the input workspace
 * @param ws : the input workspace
 * @param global : if true, a global median will be calculated for all the spectra
 * @return a new workspace representing the medians, single count if global is requested
 */
MatrixWorkspace_sptr medianWorkspace(MatrixWorkspace_sptr ws, bool global = false) {
  if (global) {
    MatrixWorkspace_sptr out = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
    std::vector<double> allY;
    allY.reserve(ws->getNumberHistograms() * ws->blocksize());
    for (int i = 0; i < static_cast<int>(ws->getNumberHistograms()); ++i) {
      const size_t wsIndex = static_cast<size_t>(i);
      const auto spectrum = ws->mutableY(wsIndex).rawData();
      std::copy(spectrum.cbegin(), spectrum.cend(), allY.end());
    }
    auto &y = out->mutableY(0);
    y = std::vector<double>(1, median(std::move(allY)));
    return out;
  } else {
    MatrixWorkspace_sptr out = WorkspaceFactory::Instance().create("Workspace2D", ws->getNumberHistograms(), 1, 1);
    PARALLEL_FOR_IF(threadSafe(*ws, *out))
    for (int i = 0; i < static_cast<int>(ws->getNumberHistograms()); ++i) {
      const size_t wsIndex = static_cast<size_t>(i);
      auto &y = out->mutableY(wsIndex);
      y = std::vector<double>(1, median(ws->mutableY(wsIndex).rawData()));
    }
    return out;
  }
}

} // namespace

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(Stitch)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string Stitch::name() const { return "Stitch"; }

/// Algorithm's version for identification. @see Algorithm::version
int Stitch::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string Stitch::category() const { return "Transforms\\Merging"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string Stitch::summary() const { return "Stitches overlapping spectra from multiple workspaces."; }

/// Validate the input workspaces for compatibility
std::map<std::string, std::string> Stitch::validateInputs() {
  std::map<std::string, std::string> issues;
  const std::vector<std::string> inputs_given = getProperty(INPUT_WORKSPACE_PROPERTY);
  std::vector<std::string> workspaces;
  RunCombinationHelper combHelper;
  try {
    workspaces = combHelper.unWrapGroups(inputs_given);
  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] = std::string(e.what());
    return issues;
  }
  if (workspaces.size() < 2) {
    issues[INPUT_WORKSPACE_PROPERTY] = "Please provide at least 2 workspaces to stitch.";
    return issues;
  }
  if (getPropertyValue(SCALE_FACTOR_CALCULATION_PROPERTY) == "Manual") {
    const std::vector<double> factors = getProperty(MANUAL_SCALE_FACTORS_PROPERTY);
    if (factors.size() != workspaces.size()) {
      issues[MANUAL_SCALE_FACTORS_PROPERTY] = "If manual scale factors are requested, the number of scale factors must "
                                              "match the number of input workspaces.";
    }
  }
  try {
    const auto first = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaces.front());
    combHelper.setReferenceProperties(first);
  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] =
        std::string("Please provide MatrixWorkspaces as or groups of those as input: ") + e.what();
    return issues;
  }
  if (!isDefault(REFERENCE_WORKSPACE_PROPERTY)) {
    const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_PROPERTY);
    if (std::find_if(workspaces.cbegin(), workspaces.cend(),
                     [&referenceName](const auto wsName) { return wsName == referenceName; }) == workspaces.cend()) {
      issues[REFERENCE_WORKSPACE_PROPERTY] = "Reference workspace must be one of the input workspaces";
      return issues;
    }
  }
  for (const auto &wsName : workspaces) {
    const auto ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsName);
    // check if all the others are compatible with the first one
    const std::string compatible = combHelper.checkCompatibility(ws, true);
    if (!compatible.empty()) {
      issues[INPUT_WORKSPACE_PROPERTY] += "Workspace " + ws->getName() + " is not compatible: " + compatible + "\n";
    }
    // check that the workspaces are not ragged
    if (!ws->isCommonBins()) {
      issues[INPUT_WORKSPACE_PROPERTY] += "Workspace " + ws->getName() + " is ragged which is not supported.\n";
    }
  }
  return issues;
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void Stitch::init() {
  declareProperty(
      std::make_unique<ArrayProperty<std::string>>(INPUT_WORKSPACE_PROPERTY, std::make_unique<ADSValidator>()),
      "The names of the input workspaces or groups of those as a list. "
      "At least two compatible MatrixWorkspaces are required, having one spectrum each. ");
  declareProperty(REFERENCE_WORKSPACE_PROPERTY, "",
                  "The name of the workspace that will serve as the reference; "
                  "that is, the one that will not be scaled. If left blank, "
                  "stitching will be performed left to right in the order of x-axes ascending, "
                  "no matter the order of workspaces names in the input.");
  declareProperty(COMBINATION_BEHAVIOUR_PROPERTY, "Interleave",
                  std::make_unique<ListValidator<std::string>>(std::array<std::string, 1>{"Interleave"}));
  declareProperty(SCALE_FACTOR_CALCULATION_PROPERTY, "MedianOfRatios",
                  std::make_unique<ListValidator<std::string>>(std::array<std::string, 2>{"MedianOfRatios", "Manual"}));
  declareProperty(
      std::make_unique<ArrayProperty<double>>(MANUAL_SCALE_FACTORS_PROPERTY),
      "Manually specified scale factors, must follow the order of ascending x-axis in the input workspaces list.");
  setPropertySettings(MANUAL_SCALE_FACTORS_PROPERTY,
                      std::make_unique<EnabledWhenProperty>(SCALE_FACTOR_CALCULATION_PROPERTY, IS_EQUAL_TO, "Manual"));
  declareProperty(TIE_SCALE_FACTORS_PROPERTY, false,
                  "Whether or not to calculate a single scale factor per workspace for all the spectra.");
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(OUTPUT_WORKSPACE_PROPERTY, "", Direction::Output),
                  "The output workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(OUTPUT_SCALE_FACTORS_PROPERTY, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "The output workspace containing the applied scale factors.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Stitch::exec() {
  std::vector<MatrixWorkspace_sptr> workspaces;
  const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_PROPERTY);
  const auto combinationBehaviour = getPropertyValue(COMBINATION_BEHAVIOUR_PROPERTY);
  const auto scaleFactorCalculation = getPropertyValue(SCALE_FACTOR_CALCULATION_PROPERTY);
  const auto inputs = RunCombinationHelper::unWrapGroups(getProperty(INPUT_WORKSPACE_PROPERTY));
  std::transform(inputs.cbegin(), inputs.cend(), std::back_inserter(workspaces),
                 [](const auto ws) { return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws); });
  std::sort(workspaces.begin(), workspaces.end(), compareInterval);
  auto progress = std::make_unique<Progress>(this, 0.0, 1.0, workspaces.size());
  size_t referenceIndex = 0;
  if (!isDefault(REFERENCE_WORKSPACE_PROPERTY)) {
    const auto ref = std::find_if(workspaces.cbegin(), workspaces.cend(),
                                  [&referenceName](const auto ws) { return ws->getName() == referenceName; });
    referenceIndex = std::distance(workspaces.cbegin(), ref);
  }
  size_t leftIterator = referenceIndex, rightIterator = referenceIndex;
  std::vector<std::string> toStitch(workspaces.size(), "");
  toStitch[referenceIndex] = workspaces[referenceIndex]->getName();
  while (leftIterator > 0) {
    toStitch[leftIterator - 1] = scale(workspaces[leftIterator], workspaces[leftIterator - 1]);
    progress->report();
    --leftIterator;
  }
  while (rightIterator < workspaces.size() - 1) {
    toStitch[rightIterator + 1] = scale(workspaces[rightIterator], workspaces[rightIterator + 1]);
    progress->report();
    ++rightIterator;
  }
  setProperty(OUTPUT_WORKSPACE_PROPERTY, merge(toStitch, toStitch[referenceIndex]));
  progress->report();
}

MatrixWorkspace_sptr Stitch::merge(const std::vector<std::string> &inputs, const std::string &refName) {
  // interleave option is equivalent to concatenation followed by sort X axis
  auto joiner = createChildAlgorithm("ConjoinXRuns");
  joiner->setProperty("InputWorkspaces", inputs);
  joiner->setPropertyValue("OutputWorkspace", "__joined");
  joiner->execute();
  Workspace_sptr output = joiner->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr joined = std::dynamic_pointer_cast<MatrixWorkspace>(output);

  // once joined, clear all the scaled workspaces, except the reference as it is not clonned
  for (const auto &wsName : inputs) {
    if (wsName != refName)
      AnalysisDataService::Instance().remove(wsName);
  }

  auto sorter = createChildAlgorithm("SortXAxis");
  sorter->setProperty("InputWorkspace", joined);
  sorter->setPropertyValue("OutputWorkspace", "__sorted");
  sorter->execute();
  MatrixWorkspace_sptr sorted = sorter->getProperty("OutputWorkspace");
  return sorted;
}

std::string Stitch::scale(MatrixWorkspace_sptr wsToMatch, MatrixWorkspace_sptr wsToScale) {
  const auto overlap = getOverlap(wsToMatch, wsToScale);
  auto cropper = createChildAlgorithm("CropWorkspaceRagged");
  cropper->setProperty("XMin", std::vector<double>({overlap.first}));
  cropper->setProperty("XMax", std::vector<double>({overlap.second}));

  cropper->setProperty("InputWorkspace", wsToMatch);
  cropper->setPropertyValue("OutputWorkspace", "__to_match");
  cropper->execute();
  MatrixWorkspace_sptr croppedToMatch = cropper->getProperty("OutputWorkspace");
  cropper->setProperty("InputWorkspace", wsToScale);
  cropper->setPropertyValue("OutputWorkspace", "__to_scale");
  cropper->execute();
  MatrixWorkspace_sptr croppedToScale = cropper->getProperty("OutputWorkspace");

  MatrixWorkspace_sptr rebinnedToScale;
  if (wsToMatch->isHistogramData()) {
    auto rebinner = createChildAlgorithm("RebinToWorkspace");
    rebinner->setProperty("WorkspaceToMatch", croppedToMatch);
    rebinner->setProperty("WorkspaceToRebin", croppedToScale);
    rebinner->setPropertyValue("OutputWorkspace", "__rebinned");
    rebinner->execute();
    rebinnedToScale = rebinner->getProperty("OutputWorkspace");
  } else {
    auto interpolator = createChildAlgorithm("SplineInterpolation");
    interpolator->setProperty("WorkspaceToMatch", croppedToMatch);
    interpolator->setProperty("WorkspaceToInterpolate", croppedToScale);
    interpolator->setProperty("Linear2Points", true);
    interpolator->setPropertyValue("OutputWorkspace", "__interpolated");
    interpolator->execute();
    rebinnedToScale = interpolator->getProperty("OutputWorkspace");
  }

  auto divider = createChildAlgorithm("Divide");
  divider->setProperty("LHSWorkspace", rebinnedToScale);
  divider->setProperty("RHSWorkspace", croppedToMatch);
  divider->setPropertyValue("OutputWorkspace", "__ratio");
  divider->execute();
  MatrixWorkspace_sptr ratio = divider->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr median = medianWorkspace(ratio, getProperty(TIE_SCALE_FACTORS_PROPERTY));

  auto scaler = createChildAlgorithm("Divide");
  scaler->setAlwaysStoreInADS(true);
  scaler->setProperty("LHSWorkspace", wsToScale);
  scaler->setProperty("RHSWorkspace", median);
  const std::string scaled = "__scaled_" + wsToScale->getName();
  scaler->setPropertyValue("OutputWorkspace", scaled);
  scaler->execute();
  return scaled;
}

} // namespace Algorithms
} // namespace Mantid
