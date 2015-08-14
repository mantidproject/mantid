#include "MantidAlgorithms/ProcessIndirectFitParameters.h"

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid {
namespace Algorithms {

using namespace API;
using namespace Kernel;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ProcessIndirectFitParameters)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProcessIndirectFitParameters::ProcessIndirectFitParameters() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ProcessIndirectFitParameters::~ProcessIndirectFitParameters() {}

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

  declareProperty(new WorkspaceProperty<API::ITableWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "The table workspace to convert to a MatrixWorkspace.");

  declareProperty(
      "X Column", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "The column in the table to use for the x values.", Direction::Input);

  declareProperty("Parameter Names", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "List of the parameter names to add to the workspace.",
                  Direction::Input);

  declareProperty("OutputWorkspace Name", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name to call the output workspace.", Direction::Input);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ProcessIndirectFitParameters::exec() {
  // Get Properties
  ITableWorkspace_sptr inputWs = getProperty("InputWorkspace");
  std::string xColumn = getProperty("X Column");
  std::string parameterNamesProp = getProperty("Parameter Names");
  auto parameterNames = listToVector(parameterNamesProp);
  std::string outputWsName = getProperty("OutputWorkspace Name");

  // Search for any parameters in the table with the given parameter names,
  // ignoring their function index and output them to a workspace
  auto workspaceNames = std::vector<std::vector<std::string>>();
  const size_t totalNames = parameterNames.size();
  for (size_t i = 0; i < totalNames; i++) {
    auto const allColumnNames = inputWs->getColumnNames();
    auto columns = searchForFitParams(parameterNames.at(i), allColumnNames);
    auto errColumns =
        searchForFitParams((parameterNames.at(i) + "_Err"), allColumnNames);

    auto paramWorkspaces = std::vector<std::string>();
    size_t min = columns.size();
    if (errColumns.size() < min) {
      min = errColumns.size();
    }
    auto convertToMatrix =
        createChildAlgorithm("ConvertTableToMatrixWorkspace", -1, -1, true);
    convertToMatrix->setProperty("InputWorkspace", inputWs);
    convertToMatrix->setProperty("ColumnX", xColumn);
    for (size_t j = 0; j < min; j++) {
      convertToMatrix->setProperty("ColumnY", columns.at(j));
      convertToMatrix->setProperty("ColumnE", errColumns.at(j));
      convertToMatrix->setProperty("OutputWorkspace", columns.at(j));
      convertToMatrix->executeAsChildAlg();
      paramWorkspaces.push_back(columns.at(j));
    }
    workspaceNames.push_back(paramWorkspaces);
  }

  // Transpose list of workspaces, ignoring unequal length of lists
  // this handles the case where a parameter occurs only once in the whole
  // workspace
  workspaceNames = reorderWorkspaceParams(workspaceNames); 


  // Join all the parameters for each peak into a single workspace per peak
  auto tempWorkspaces = std::vector<std::string>();
  auto conjoin = createChildAlgorithm("ConjoinWorkspaces", -1, -1, true);
  conjoin->setProperty("CheckOverlapping", false);

  const size_t wsMax = workspaceNames.size();
  for (size_t j = 0; j < wsMax; j++) {
    auto tempPeakWs = workspaceNames.at(j).at(0);
    const size_t paramMax = workspaceNames.at(j).size();
    for (size_t k = 1; k < paramMax; k++) {
      auto paramWs = workspaceNames.at(j).at(k);
      conjoin->setProperty("InputWorkspace1", tempPeakWs);
      conjoin->setProperty("InputWorkspace2", paramWs);
      conjoin->executeAsChildAlg();
      tempPeakWs = conjoin->getProperty("InputWorkspace1");
    }
    tempWorkspaces.push_back(tempPeakWs);
  }

  // Join all peaks into a single workspace
  auto tempWorkspace = tempWorkspaces.at(0);
  for (auto it = tempWorkspaces.begin(); it != tempWorkspaces.end(); ++it) {
    conjoin->setProperty("InputWorkspace1", tempWorkspace);
    conjoin->setProperty("InputWorkspace2", *it);
    conjoin->executeAsChildAlg();
    tempWorkspace = conjoin->getProperty("InputWorkspace1");
  }

  auto renamer = createChildAlgorithm("RenameWorkspace", -1, -1, true);
  renamer->setProperty("InputWorkspace", tempWorkspace);
  renamer->setProperty("OutputWorkspace", outputWsName);
  renamer->executeAsChildAlg();
  MatrixWorkspace_sptr outputWs = renamer->getProperty("OutputWorkspace");

  // Replace axis on workspaces with text axis
  auto axis = TextAxis(outputWs->getNumberHistograms());
  TextAxis *axisPtr = &(axis);
  for (size_t j = 0; j < workspaceNames.size(); j++) {
    auto peakWs = workspaceNames.at(j);
    for (size_t k = 0; k < peakWs.size(); k++) {
      axisPtr->setLabel(k, peakWs.at(k));
    }
  }
  outputWs->replaceAxis(1, axisPtr);


}

std::vector<std::string>
ProcessIndirectFitParameters::listToVector(std::string &commaList) {
  auto listVector = std::vector<std::string>();
  auto pos = commaList.find(",");
  while (pos != std::string::npos) {
    std::string nextItem = commaList.substr(0, pos);
    listVector.push_back(nextItem);
    commaList = commaList.substr(pos + 1, commaList.size());
    pos = commaList.find(",");
  }
  listVector.push_back(commaList);
  return listVector;
}

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

std::vector<std::vector<std::string>> ProcessIndirectFitParameters::reorderWorkspaceParams(const std::vector<std::vector<std::string>> &original){
	size_t maximumLength = original.at(0).size();
	for(int i = 1; i < original.size(); i++){
		if(original.at(i).size() > maximumLength){
			maximumLength = original.at(i).size();
		}
	}

	auto reorderedVector = std::vector<std::vector<std::string>>();

	for(int i = 0; i < maximumLength; i++){
		std::vector<std::string> temp;
		for(int j = 0; j < original.size(); j++){
			if(original.at(j).size() > i){
			temp.push_back(original.at(j).at(i));
			}
		}
		reorderedVector.push_back(temp);
	}

	return reorderedVector;

}

} // namespace Algorithms
} // namespace Mantid