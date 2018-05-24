#include "MantidWorkflowAlgorithms/ProcessIndirectFitParameters.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/UnitFactory.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ProcessIndirectFitParameters)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string ProcessIndirectFitParameters::name() const {
  return "ProcessIndirectFitParameters";
}

/// Algorithm's version for identification. @see Algorithm::version
int ProcessIndirectFitParameters::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string ProcessIndirectFitParameters::category() const {
  return "Workflow\\MIDAS";
}

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string ProcessIndirectFitParameters::summary() const {
  return "Convert a parameter table output by PlotPeakByLogValue to a "
         "MatrixWorkspace.";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ProcessIndirectFitParameters::init() {

  std::vector<std::string> unitOptions = UnitFactory::Instance().getKeys();
  unitOptions.emplace_back("");

  declareProperty(make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "InputWorkspace", "", Direction::Input),
                  "The table workspace to convert to a MatrixWorkspace.");

  declareProperty(
      "ColumnX", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "The column in the table to use for the x values.", Direction::Input);

  declareProperty(
      make_unique<ArrayProperty<std::string>>(
          "ParameterNames",
          boost::make_shared<MandatoryValidator<std::vector<std::string>>>()),
      "List of the parameter names to add to the workspace.");

  declareProperty("XAxisUnit", "",
                  boost::make_shared<StringListValidator>(unitOptions),
                  "The unit to assign to the X Axis");

  declareProperty(make_unique<WorkspaceProperty<MatrixWorkspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to give the output workspace");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ProcessIndirectFitParameters::exec() {
  // Get Properties
  ITableWorkspace_sptr inputWs = getProperty("InputWorkspace");
  std::string xColumn = getProperty("ColumnX");
  std::vector<std::string> parameterNames = getProperty("ParameterNames");
  std::string xUnit = getProperty("XAxisUnit");
  MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");

  // Search for any parameters in the table with the given parameter names,
  // ignoring their function index and output them to a workspace
  auto workspaces = std::vector<std::vector<MatrixWorkspace_sptr>>();
  const size_t totalNames = parameterNames.size();
  Progress tblSearchProg = Progress(this, 0.0, 0.5, totalNames * 2);
  for (size_t i = 0; i < totalNames; i++) {
    tblSearchProg.report("Splitting table into relevant columns");
    auto const allColumnNames = inputWs->getColumnNames();
    auto columns = searchForFitParams(parameterNames.at(i), allColumnNames);
    auto errColumns =
        searchForFitParams((parameterNames.at(i) + "_Err"), allColumnNames);

    auto paramWorkspaces = std::vector<MatrixWorkspace_sptr>();
    size_t min = columns.size();
    if (errColumns.size() < min) {
      min = errColumns.size();
    }
    auto convertToMatrix =
        createChildAlgorithm("ConvertTableToMatrixWorkspace", -1, -1, true);
    tblSearchProg.report("Converting Column to Matrix");
    for (size_t j = 0; j < min; j++) {
      convertToMatrix->setProperty("InputWorkspace", inputWs);
      convertToMatrix->setProperty("ColumnX", xColumn);
      convertToMatrix->setProperty("ColumnY", columns.at(j));
      convertToMatrix->setProperty("ColumnE", errColumns.at(j));
      convertToMatrix->setProperty("OutputWorkspace", columns.at(j));
      convertToMatrix->executeAsChildAlg();
      paramWorkspaces.push_back(
          convertToMatrix->getProperty("OutputWorkspace"));
    }
    workspaces.push_back(std::move(paramWorkspaces));
  }

  Progress workflowProg = Progress(this, 0.5, 1.0, 10);
  // Transpose list of workspaces, ignoring unequal length of lists
  // this handles the case where a parameter occurs only once in the whole
  // workspace
  workflowProg.report("Reordering workspace vector");
  workspaces = reorder2DVector(workspaces);

  // Get output workspace columns
  auto outputSpectraNames = std::vector<std::string>();
  for (const auto &paramWorkspaces : workspaces) {
    for (const auto &workspace : paramWorkspaces) {
      outputSpectraNames.push_back(workspace->YUnitLabel());
    }
  }

  // Join all the parameters for each peak into a single workspace per peak
  auto tempWorkspaces = std::vector<MatrixWorkspace_sptr>();
  auto conjoin = createChildAlgorithm("ConjoinWorkspaces", -1, -1, true);
  conjoin->setProperty("CheckOverlapping", false);
  const size_t wsMax = workspaces.size();
  for (size_t j = 0; j < wsMax; j++) {
    auto tempPeakWs = workspaces.at(j).at(0);
    const size_t paramMax = workspaces.at(j).size();
    workflowProg.report("Conjoining matrix workspaces");
    for (size_t k = 1; k < paramMax; k++) {
      auto paramWs = workspaces.at(j).at(k);
      conjoin->setProperty("InputWorkspace1", tempPeakWs);
      conjoin->setProperty("InputWorkspace2", paramWs);
      conjoin->executeAsChildAlg();
      tempPeakWs = conjoin->getProperty("InputWorkspace1");
    }
    tempWorkspaces.push_back(tempPeakWs);
  }

  // Join all peaks into a single workspace
  outputWs = tempWorkspaces.at(0);
  for (auto it = tempWorkspaces.begin() + 1; it != tempWorkspaces.end(); ++it) {
    workflowProg.report("Joining peak workspaces");
    conjoin->setProperty("InputWorkspace1", outputWs);
    conjoin->setProperty("InputWorkspace2", *it);
    conjoin->executeAsChildAlg();
    outputWs = conjoin->getProperty("InputWorkspace1");
  }

  // Replace axis on workspaces with text axis
  workflowProg.report("Converting text axis");
  const auto numberOfHist = outputWs->getNumberHistograms();
  auto axis = new TextAxis(numberOfHist);

  for (size_t k = 0; k < numberOfHist; k++) {
    axis->setLabel(k, outputSpectraNames[k]);
  }
  outputWs->replaceAxis(1, axis);

  workflowProg.report("Setting unit");
  // Set units for the xAxis
  if (!xUnit.empty()) {
    outputWs->getAxis(0)->setUnit(xUnit);
  }

  setProperty("OutputWorkspace", outputWs);
}

/**
 * Searchs for a particular word within all of the fit params in the columns of
 * a table workspace (note: This method only matches strings that are at the end
 * of a column name this is to ensure that "Amplitude" will match
 * "f0.f0.f1.Amplitude" but not f0.f0.f1.Amplitude_Err")
 * @param suffix - The string to search for
 * @param columns - A string vector of all the column names in a table workspace
 * @return - The full column names in which the string is present
 */
std::vector<std::string> ProcessIndirectFitParameters::searchForFitParams(
    const std::string &suffix, const std::vector<std::string> &columns) {
  auto fitParams = std::vector<std::string>();
  const size_t totalColumns = columns.size();
  for (size_t i = 0; i < totalColumns; i++) {
    auto pos = columns.at(i).rfind(suffix);
    if (pos != std::string::npos) {
      auto endCheck = pos + suffix.size();
      if (endCheck == columns.at(i).size()) {
        fitParams.push_back(columns.at(i));
      }
    }
  }
  return fitParams;
}

/**
 * Changes the ordering of a 2D vector of strings such that
 * [[1a,2a,3a], [1b,2b,3b], [1c,2c,3c]]
 * becomes
 * [[1a,1b,1c], [2a,2b,2c], [3a,3b,3c]]
 * @param original - The original vector to be transformed
 * @return - The vector after it has been transformed
 */
std::vector<std::vector<MatrixWorkspace_sptr>>
ProcessIndirectFitParameters::reorder2DVector(
    std::vector<std::vector<MatrixWorkspace_sptr>> &original) {
  size_t maximumLength = original.at(0).size();
  for (size_t i = 1; i < original.size(); i++) {
    if (original.at(i).size() > maximumLength) {
      maximumLength = original.at(i).size();
    }
  }

  auto reorderedVector = std::vector<std::vector<MatrixWorkspace_sptr>>();
  for (size_t i = 0; i < maximumLength; i++) {
    std::vector<MatrixWorkspace_sptr> temp;
    for (const auto &j : original) {
      if (j.size() > i) {
        temp.push_back(j.at(i));
      }
    }
    reorderedVector.push_back(temp);
  }

  return reorderedVector;
}

} // namespace Algorithms
} // namespace Mantid
