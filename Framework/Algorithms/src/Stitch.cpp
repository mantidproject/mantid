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
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace {
static const std::string INPUT_WORKSPACE_PROPERTY = "InputWorkspaces";
static const std::string REFERENCE_WORKSPACE_NAME = "ReferenceWorkspace";
static const std::string COMBINATION_BEHAVIOUR = "CombinationBehaviour";
static const std::string SCALE_FACTOR_CALCULATION = "ScaleFactorCalculation";
static const std::string SCALE_FACTOR_BEHAVIOUR = "ScaleFactorBehaviour";
static const std::string OUTPUT_WORKSPACE_PROPERTY = "OutputWorkspace";

/**
 * @brief getMinMaxX Calculates the x-axis extent of a [ragged] workspace
 * Assumes that the histogram bin edges are in ascending order
 * @param ws : the input workspace
 * @return a pair of min-x and max-x
 */
std::pair<double, double> getMinMaxX(const MatrixWorkspace &ws) {
  double xmin = ws.readX(0).front();
  double xmax = ws.readX(0).back();
  for (size_t i = 1; i < ws.getNumberHistograms(); ++i) {
    const double startx = ws.readX(i).front();
    if (startx < xmin) {
      xmin = startx;
    }
    const double endx = ws.readX(i).back();
    if (endx > xmax) {
      xmax = endx;
    }
  }
  return std::make_pair(xmin, xmax);
}

/**
 * @brief compareInterval Compares two workspaces in terms of their x-coverage
 * @param ws1 : input workspace 1
 * @param ws2 : input workspace 2
 * @return true if ws1 is less than ws2 in terms of its x-interval
 */
bool compareInterval(const MatrixWorkspace_sptr ws1, const MatrixWorkspace_sptr ws2) {
  const auto minmax1 = getMinMaxX(*ws1);
  const auto minmax2 = getMinMaxX(*ws2);
  if (minmax1.first < minmax2.first) {
    return true;
  } else if (minmax1.first > minmax2.first) {
    return false;
  } else {
    return minmax1.second < minmax2.second;
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
  try {
    combHelper.setReferenceProperties(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaces.front()));
  } catch (const std::exception &e) {
    issues[INPUT_WORKSPACE_PROPERTY] =
        std::string("Please provide MatrixWorkspaces as or groups of those as input: ") + e.what();
    return issues;
  }
  if (!isDefault(REFERENCE_WORKSPACE_NAME)) {
    const auto referenceName = getProperty(REFERENCE_WORKSPACE_NAME);
    if (std::find(workspaces.cbegin(), workspaces.cend(), referenceName) == workspaces.cend()) {
      issues[REFERENCE_WORKSPACE_NAME] = "Reference workspace must be one of the input workspaces";
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
      "At least two MatrixWorkspaces are required, having the same instrument, same number of spectra and units.");
  declareProperty(REFERENCE_WORKSPACE_NAME, "",
                  "The name of the workspace that will serve as the reference; "
                  "that is, the one that will not be scaled. If left blank, "
                  "stitching will be performed left to right.");
  declareProperty(COMBINATION_BEHAVIOUR, "Interleave",
                  std::make_unique<ListValidator<std::string>>("Interleave", "Merge"));
  declareProperty(SCALE_FACTOR_BEHAVIOUR, "PerSpectrum",
                  std::make_unique<ListValidator<std::string>>("PerSpectrum", "PerWorkspace"));
  declareProperty(SCALE_FACTOR_CALCULATION, "MedianOfRatios",
                  std::make_unique<ListValidator<std::string>>("MedianOfRatios", "MeanOfRatios", "RatioOfIntegrals"));
  declareProperty(std::make_unique<WorkspaceProperty<API::Workspace>>(OUTPUT_WORKSPACE_PROPERTY, "", Direction::Output),
                  "The output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void Stitch::exec() {
  std::vector<MatrixWorkspace_sptr> workspaces;
  const auto referenceName = getPropertyValue(REFERENCE_WORKSPACE_NAME);
  const auto combinationBehaviour = getPropertyValue(COMBINATION_BEHAVIOUR);
  const auto scaleFactorBehaviour = getPropertyValue(SCALE_FACTOR_BEHAVIOUR);
  const auto scaleFactorCalculation = getPropertyValue(SCALE_FACTOR_CALCULATION);
  const auto inputs = RunCombinationHelper::unWrapGroups(getProperty(INPUT_WORKSPACE_PROPERTY));
  std::transform(inputs.cbegin(), inputs.cend(), std::back_inserter(workspaces),
                 [](const auto ws) { return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(ws); });
  std::sort(workspaces.begin(), workspaces.end(), compareInterval);
}

} // namespace Algorithms
} // namespace Mantid
