#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EvaluateFunction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string EvaluateFunction::name() const { return "EvaluateFunction"; }

/// Algorithm's version for identification. @see Algorithm::version
int EvaluateFunction::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string EvaluateFunction::summary() const {
  return "Evaluate a function on a workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void EvaluateFunction::initConcrete() {
  declareProperty(make_unique<WorkspaceProperty<API::Workspace>>(
                      "OutputWorkspace", "", Direction::Output),
                  "An output workspace.");
}

/** Cross property validation
* @return A map containing property as keys and errors as values.
*/
std::map<std::string, std::string> EvaluateFunction::validateInputs() {
  API::Workspace_const_sptr ws = getProperty("InputWorkspace");
  auto wsMatrix = boost::dynamic_pointer_cast<const MatrixWorkspace>(ws);
  std::map<std::string, std::string> errors = Algorithm::validateInputs();

  if (wsMatrix != nullptr) {
    const double startX = getProperty("StartX");
    const double endX = getProperty("EndX");
    auto &xData = wsMatrix->x(0);

    const double workspaceStartX = xData[0];
    const double workspaceEndX = xData[xData.size() - 1];
    std::string errorMsg = "";
    bool doesNotCaptureWs =
        !(startX == EMPTY_DBL() && endX == EMPTY_DBL()) &&
        ((startX < workspaceStartX && endX < workspaceStartX) ||
         (startX > workspaceEndX && endX > workspaceEndX));

    // Build error message from out of range checks.
    if (doesNotCaptureWs) {
      errorMsg =
          "StartX and EndX do not capture a section of the workspace X range.";
    }

    // Check if there was an out of range error.
    if (!errorMsg.empty()) {
      errors["InputWorkspace"] = errorMsg;
    }
  }

  return errors;
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void EvaluateFunction::execConcrete() {

  // Function may need some preparation.
  m_function->setUpForFit();

  API::FunctionDomain_sptr domain;
  API::FunctionValues_sptr values;
  m_domainCreator->createDomain(domain, values);

  // Do something with the function which may depend on workspace.
  m_domainCreator->initFunction(m_function);

  // Apply any ties.
  m_function->applyTies();

  // Calculate function values.
  m_function->function(*domain, *values);

  // Gnegerate the output workspace
  auto outputWS = m_domainCreator->createOutputWorkspace("", m_function, domain,
                                                         values, "");

  // Store the result.
  setProperty("OutputWorkspace", outputWS);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
