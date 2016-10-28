#include "MantidCurveFitting/Algorithms/MonteCarloParameters.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidKernel/ListValidator.h"

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MonteCarloParameters)

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MonteCarloParameters::name() const {
  return "MonteCarloParameters";
}

/// Algorithm's version for identification. @see Algorithm::version
int MonteCarloParameters::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MonteCarloParameters::summary() const {
  return "Estimate parameters of a fitting function using a Monte Carlo algorithm.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void MonteCarloParameters::initConcrete() {
  declareCostFunctionProperty();
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void MonteCarloParameters::execConcrete() {
  auto costFunction = getCostFunctionProperty();
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
