// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include <utility>

#include "MantidAPI/ADSValidator.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/Stitch.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/EnabledWhenProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

namespace {
static const std::string INPUT_WORKSPACE_PROPERTY = "InputWorkspaces";
static const std::string REFERENCE_WORKSPACE_PROPERTY = "ReferenceWorkspace";
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
bool compareInterval(const MatrixWorkspace_sptr &ws1, const MatrixWorkspace_sptr &ws2) {
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
std::pair<double, double> getOverlap(const MatrixWorkspace_sptr &ws1, const MatrixWorkspace_sptr &ws2) {
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
  std::vector<double> sorted = vec;
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
 * @return a new workspace representing the medians, single count if global is requested
 */
MatrixWorkspace_sptr medianWorkspaceLocal(const MatrixWorkspace_sptr &ws) {
  const size_t nSpectra = ws->getNumberHistograms();
  MatrixWorkspace_sptr out = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, 1, 1);
  PARALLEL_FOR_IF(threadSafe(*ws, *out))
  for (int i = 0; i < static_cast<int>(nSpectra); ++i) {
    const size_t wsIndex = static_cast<size_t>(i);
    auto &y = out->mutableY(wsIndex);
    y = std::vector<double>(1, median(ws->readY(wsIndex)));
  }
  return out;
}

/**
 * @brief Creates a single bin and single spectrum workspace containing the global median of the input workspace
 * @param ws : the input workspace
 * @return a new workspace representing the medians, single count if global is requested
 */
MatrixWorkspace_sptr medianWorkspaceGlobal(const MatrixWorkspace_sptr &ws) {
  MatrixWorkspace_sptr out = WorkspaceFactory::Instance().create("Workspace2D", 1, 1, 1);
  std::vector<double> allY;
  allY.reserve(ws->getNumberHistograms() * ws->blocksize());
  for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
    const auto spectrum = ws->mutableY(i).rawData();
    std::copy(spectrum.cbegin(), spectrum.cend(), std::back_inserter(allY));
  }
  auto &y = out->mutableY(0);
  y = std::vector<double>(1, median(allY));
  return out;
}

/**
 * @brief Creates a 2D workspace to host the calculated scale factors, initialized to 1s
 * @param nSpectra : number of spectra
 * @param nPoints : number of points, that is input workspaces
 * @return a pointer to the workspace
 */
MatrixWorkspace_sptr initScaleFactorsWorkspace(const size_t nSpectra, const size_t nPoints) {
  MatrixWorkspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", nSpectra, nPoints, nPoints);
  PARALLEL_FOR_IF(threadSafe(*ws))
  for (int i = 0; i < static_cast<int>(nSpectra); ++i) {
    ws->mutableY(static_cast<size_t>(i)) = std::vector<double>(nPoints, 1.);
  }
  return ws;
}

} // namespace

namespace Mantid::Algorithms {

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
const std::string Stitch::summary() const { return "Stitches overlapping spectra from multiple 2D workspaces."; }

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
    if (ws->isHistogramData()) {
      issues[INPUT_WORKSPACE_PROPERTY] +=
          "Workspace " + ws->getName() + " contains histogram data, only point data are supported.\n";
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
  declareProperty(SCALE_FACTOR_CALCULATION_PROPERTY, "MedianOfRatios",
                  std::make_unique<ListValidator<std::string>>(std::array<std::string, 2>{"MedianOfRatios", "Manual"}));
  declareProperty(std::make_unique<ArrayProperty<double>>(MANUAL_SCALE_FACTORS_PROPERTY),
                  "Manually specified scale factors, must follow the same order of the workspaces in the list.");
  setPropertySettings(MANUAL_SCALE_FACTORS_PROPERTY,
                      std::make_unique<EnabledWhenProperty>(SCALE_FACTOR_CALCULATION_PROPERTY, IS_EQUAL_TO, "Manual"));
  declareProperty(TIE_SCALE_FACTORS_PROPERTY, false,
                  "Whether or not to calculate a single scale factor per workspace for all the spectra.");
  declareProperty(
      std::make_unique<WorkspaceProperty<MatrixWorkspace>>(OUTPUT_WORKSPACE_PROPERTY, "", Direction::Output),
      "The output workspace.");
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(OUTPUT_SCALE_FACTORS_PROPERTY, "",
                                                                       Direction::Output, PropertyMode::Optional),
                  "The output workspace containing the applied scale factors.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Stitch::exec() {
  const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_PROPERTY);
  const auto scaleFactorCalculation = getPropertyValue(SCALE_FACTOR_CALCULATION_PROPERTY);
  const auto inputs = RunCombinationHelper::unWrapGroups(getProperty(INPUT_WORKSPACE_PROPERTY));
  MatrixWorkspace_sptr scaleFactorsWorkspace;
  cloneWorkspaces(inputs);
  std::vector<std::string> clones;
  std::transform(inputs.cbegin(), inputs.cend(), std::back_inserter(clones),
                 [](const auto ws) { return "__cloned_" + ws; });
  if (scaleFactorCalculation == "Manual") {
    scaleFactorsWorkspace = initScaleFactorsWorkspace(1, clones.size());
    scaleManual(clones, getProperty(MANUAL_SCALE_FACTORS_PROPERTY), scaleFactorsWorkspace);
  } else {
    scaleWithMedianRatios(clones, referenceName, scaleFactorsWorkspace);
  }
  setProperty(OUTPUT_WORKSPACE_PROPERTY, merge(clones));
  for (const auto &ws : clones) {
    AnalysisDataService::Instance().remove(ws);
  }
  if (!isDefault(OUTPUT_SCALE_FACTORS_PROPERTY)) {
    setProperty(OUTPUT_SCALE_FACTORS_PROPERTY, scaleFactorsWorkspace);
  }
}

/**
 * @brief Scales workspaces by medians of point-wise ratios in the overlap regions
 * @param clones : input workspace names (already cloned)
 * @param referenceName : the name of the reference workspace
 * @param scaleFactorsWorkspace : a reference to assign the scale factors output workspace
 */
void Stitch::scaleWithMedianRatios(const std::vector<std::string> &clones, const std::string &referenceName,
                                   MatrixWorkspace_sptr &scaleFactorsWorkspace) {
  std::vector<MatrixWorkspace_sptr> workspaces;
  std::transform(clones.cbegin(), clones.cend(), std::back_inserter(workspaces),
                 [](const auto ws) { return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws); });
  const size_t nSpectrumInScaleFactors =
      isDefault(TIE_SCALE_FACTORS_PROPERTY) ? workspaces[0]->getNumberHistograms() : 1;
  scaleFactorsWorkspace = initScaleFactorsWorkspace(nSpectrumInScaleFactors, workspaces.size());
  // sort internally by the x-extent interval ascending, but the scale factors will be stored in the original order
  std::sort(workspaces.begin(), workspaces.end(), compareInterval);
  auto progress = std::make_unique<Progress>(this, 0.0, 1.0, workspaces.size());
  const size_t referenceIndex = getReferenceIndex(workspaces, referenceName);
  size_t leftIterator = referenceIndex, rightIterator = referenceIndex;
  // we start from the reference index and iterate to the left, then to the right
  // note that these loops are delibarately serial, as the subsequent scale factors need to be computed wrt already
  // scaled previous workspaces
  while (leftIterator > 0) {
    scale(workspaces[leftIterator], workspaces[leftIterator - 1], scaleFactorsWorkspace, clones);
    progress->report();
    --leftIterator;
  }
  while (rightIterator < workspaces.size() - 1) {
    scale(workspaces[rightIterator], workspaces[rightIterator + 1], scaleFactorsWorkspace, clones);
    progress->report();
    ++rightIterator;
  }
}

/**
 * @brief Returns the index of the reference workspace in the sorted workspace list
 * @param workspaces : input workspace pointers, sorted by x-interval ascending
 * @param referenceName : the name of the reference workspace
 * @return the index of the reference workspace in the sorted workspace list
 */
size_t Stitch::getReferenceIndex(const std::vector<MatrixWorkspace_sptr> &workspaces,
                                 const std::string &referenceName) {
  size_t referenceIndex = 0;
  if (!isDefault(REFERENCE_WORKSPACE_PROPERTY)) {
    const auto ref = std::find_if(workspaces.cbegin(), workspaces.cend(), [&referenceName](const auto ws) {
      return ws->getName() == "__cloned_" + referenceName;
    });
    referenceIndex = std::distance(workspaces.cbegin(), ref);
  }
  return referenceIndex;
}

/**
 * @brief Clones all the input workspaces so that they can be scaled in-place without altering the inputs
 * Clones will be prefixed and stored on ADS, and will be eventually deleted
 * @param inputs: the list of workspace names to clone
 */
void Stitch::cloneWorkspaces(const std::vector<std::string> &inputs) {
  auto cloner = createChildAlgorithm("CloneWorkspace");
  cloner->setAlwaysStoreInADS(true);
  for (const auto &ws : inputs) {
    cloner->setProperty("InputWorkspace", ws);
    cloner->setProperty("OutputWorkspace", "__cloned_" + ws);
    cloner->execute();
  }
}

/**
 * @brief Combines the scaled workspaces together by interleaving their data
 * This is equivalent to concatenation followed by sort
 * @param inputs: the list of the scaled input workspace names
 * @return the combined workspace
 */
MatrixWorkspace_sptr Stitch::merge(const std::vector<std::string> &inputs) {
  // interleave option is equivalent to concatenation followed by sort X axis
  auto joiner = createChildAlgorithm("ConjoinXRuns");
  joiner->setProperty("InputWorkspaces", inputs);
  joiner->execute();
  Workspace_sptr output = joiner->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr joined = std::dynamic_pointer_cast<MatrixWorkspace>(output);

  auto sorter = createChildAlgorithm("SortXAxis");
  sorter->setProperty("InputWorkspace", joined);
  sorter->execute();
  MatrixWorkspace_sptr sorted = sorter->getProperty("OutputWorkspace");
  return sorted;
}

/**
 * @brief Scales one workspace to match the scale of the other
 * The scale factors are calculated as medians of point-wise ratios in the overlap region
 * The scale factors can be different per spectrum, or global for the workspace, if tied scale factors is requested
 * @param wsToMatch: the workspace to match up the scale
 * @param wsToScale: the workspace to be scaled
 * @param scaleFactorsWorkspace: the workspace where calculated scale factors will be stored
 * @param inputs: the name of the input workspace list (needed only to store the scale factor at right index)
 */
void Stitch::scale(const MatrixWorkspace_sptr &wsToMatch, const MatrixWorkspace_sptr &wsToScale,
                   Mantid::API::MatrixWorkspace_sptr scaleFactorsWorkspace, const std::vector<std::string> &inputs) {
  const auto overlap = getOverlap(wsToMatch, wsToScale);
  auto cropper = createChildAlgorithm("CropWorkspaceRagged");
  cropper->setProperty("XMin", std::vector<double>({overlap.first}));
  cropper->setProperty("XMax", std::vector<double>({overlap.second}));

  cropper->setProperty("InputWorkspace", wsToMatch);
  cropper->execute();
  MatrixWorkspace_sptr croppedToMatch = cropper->getProperty("OutputWorkspace");
  cropper->setProperty("InputWorkspace", wsToScale);
  cropper->execute();
  MatrixWorkspace_sptr croppedToScale = cropper->getProperty("OutputWorkspace");

  MatrixWorkspace_sptr rebinnedToScale;
  if (croppedToMatch->blocksize() > 1) {
    auto interpolator = createChildAlgorithm("SplineInterpolation");
    interpolator->setProperty("WorkspaceToMatch", croppedToMatch);
    interpolator->setProperty("WorkspaceToInterpolate", croppedToScale);
    interpolator->setProperty("Linear2Points", true);
    interpolator->execute();
    rebinnedToScale = interpolator->getProperty("OutputWorkspace");
  } else {
    if (croppedToMatch->readX(0) != croppedToScale->readX(0)) {
      throw std::runtime_error(
          "Unable to make the ratio; only one overlapping point is found and it is at different x");
    } else {
      rebinnedToScale = croppedToScale;
    }
  }

  auto divider = createChildAlgorithm("Divide");
  divider->setProperty("LHSWorkspace", rebinnedToScale);
  divider->setProperty("RHSWorkspace", croppedToMatch);
  divider->execute();
  MatrixWorkspace_sptr ratio = divider->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr median =
      getProperty("TieScaleFactors") ? medianWorkspaceGlobal(ratio) : medianWorkspaceLocal(ratio);

  auto scaler = createChildAlgorithm("Divide");
  scaler->setAlwaysStoreInADS(true);
  scaler->setProperty("LHSWorkspace", wsToScale);
  scaler->setProperty("RHSWorkspace", median);
  scaler->setPropertyValue("OutputWorkspace", wsToScale->getName());
  scaler->execute();

  recordScaleFactor(scaleFactorsWorkspace, median, wsToScale, inputs);
}

/**
 * @brief Stores the multiplicative scale factors into a workspace
 * Note that the scale factors are stored in the original order of the input workspaces
 * @param scaleFactorWorkspace: the output workspace where they'll be stored
 * @param medianWorkspace: the workspace containing the scale factors (i.e. medians)
 * @param scaledWorkspace: the workspace that was scaled
 * @param inputs: the list of the workspace names used to store the factors at the right index
 */
void Stitch::recordScaleFactor(const Mantid::API::MatrixWorkspace_sptr &scaleFactorWorkspace,
                               const Mantid::API::MatrixWorkspace_sptr &medianWorkspace,
                               const Mantid::API::MatrixWorkspace_sptr &scaledWorkspace,
                               const std::vector<std::string> &inputs) {
  const auto it = std::find(inputs.cbegin(), inputs.cend(), scaledWorkspace->getName());
  const size_t index = std::distance(inputs.cbegin(), it);
  PARALLEL_FOR_IF(threadSafe(*scaleFactorWorkspace))
  for (int i = 0; i < static_cast<int>(scaleFactorWorkspace->getNumberHistograms()); ++i) {
    scaleFactorWorkspace->mutableY(i)[index] = 1. / medianWorkspace->readY(i)[0];
  }
}

/**
 * @brief Performs scaling with manual scale factors, which are treated as global, i.e. applied to all spectra
 * Manual scale factors must be given in the original order of the workspaces, no matter their order in terms of
 * x-extent
 * @param inputs: the list of the input workspace names
 * @param scaleFactors: a vector with the manual scale factors, must have the same size as inputs
 * @param scaleFactorsWorkspace: the workspace to store the scale factors to
 */
void Stitch::scaleManual(const std::vector<std::string> &inputs, const std::vector<double> &scaleFactors,
                         const MatrixWorkspace_sptr &scaleFactorsWorkspace) {
  auto &outputFactors = scaleFactorsWorkspace->mutableY(0);
  auto progress = std::make_unique<Progress>(this, 0.0, 1.0, inputs.size());
  PARALLEL_FOR_IF(threadSafe(*scaleFactorsWorkspace))
  for (int i = 0; i < static_cast<int>(inputs.size()); ++i) {
    outputFactors[i] = scaleFactors[i];
    auto scaler = createChildAlgorithm("Scale");
    scaler->setAlwaysStoreInADS(true);
    scaler->setPropertyValue("InputWorkspace", inputs[i]);
    scaler->setProperty("Factor", scaleFactors[i]);
    scaler->setPropertyValue("OutputWorkspace", inputs[i]);
    scaler->execute();
    progress->report();
  }
}

} // namespace Mantid::Algorithms
