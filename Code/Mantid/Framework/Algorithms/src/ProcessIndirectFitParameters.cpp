#include "MantidAlgorithms/ProcessIndirectFitParameters.h"

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
const std::string ProcessIndirectFitParameters::name() const { return "ProcessIndirectFitParameters"; }

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
  declareProperty(
      new WorkspaceProperty<>("InputWorkspace", "", Direction::Input),
      "The table workspace to convert to a MatrixWorkspace.");

  declareProperty(
      "X Column", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "The column in the table to use for the x values.", Direction::Input);

  declareProperty("Parameter Names", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "List of the parameter names to add to the workspace.",
                  Direction::Input);
  /*May change to a string*/
  declareProperty(
      new WorkspaceProperty<>("OutputWorkspace", "", Direction::Output),
      "The name to call the output workspace.");
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void ProcessIndirectFitParameters::exec() {
  ITableWorkspace_sptr inputWs = getProperty("InputWorkspace");
  std::string xColumn = getProperty("X Column");
  std::string parameterNamesProp = getProperty("Parameter Names");
  auto parameterNames = listToVector(parameterNamesProp);
  MatrixWorkspace_sptr outputWs = getProperty("OutputWorkspace");
}

std::vector<std::string>
ProcessIndirectFitParameters::listToVector(std::string &commaList) {
  auto listVector = std::vector<std::string>();
  auto pos = commaList.find(",");
  while (pos != std::string::npos) {
	  std::string nextItem = commaList.substr(0, pos);
	  listVector.push_back(nextItem);
	  commaList = commaList.substr(pos, commaList.size());
	  pos = commaList.find(",");
  }
  return listVector;
}

} // namespace Algorithms
} // namespace Mantid