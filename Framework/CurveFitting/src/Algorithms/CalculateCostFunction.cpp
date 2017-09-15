#include "MantidCurveFitting/Algorithms/CalculateCostFunction.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(CalculateCostFunction)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string CalculateCostFunction::name() const {
  return "CalculateCostFunction";
}

/// Algorithm's version for identification. @see Algorithm::version
int CalculateCostFunction::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string CalculateCostFunction::summary() const {
  return "Calculate cost function for a function and a data set in a "
         "workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void CalculateCostFunction::initConcrete() {
  declareCostFunctionProperty();
  declareProperty("Value", 0.0, "Output value of the cost function.",
                  Direction::Output);
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void CalculateCostFunction::execConcrete() {

  if (!m_costFunction) {
    m_costFunction = getCostFunctionInitialized();
  }

  // Get the result.
  double value = m_costFunction->val();
  // Store the result.
  setProperty("Value", value);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
