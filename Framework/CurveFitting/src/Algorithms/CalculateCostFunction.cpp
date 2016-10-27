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
  return "Calculate cost function for a function and a data set in a workspace.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void CalculateCostFunction::initConcrete() {
  Kernel::IValidator_sptr costFuncValidator =
      boost::make_shared<Kernel::ListValidator<std::string>>(getCostFunctionNames());
  declareProperty(
      "CostFunction", "Least squares", costFuncValidator,
      "The cost function to be used for the fit, default is Least squares",
      Kernel::Direction::InOut);
  declareProperty("Value", 0.0, "Output value of the cost function.",
                  Direction::Output);
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void CalculateCostFunction::execConcrete() {

  if (!m_costFunction) {

    // Function may need some preparation.
    m_function->setUpForFit();

    API::FunctionDomain_sptr domain;
    API::FunctionValues_sptr values;
    m_domainCreator->createDomain(domain, values);

    // Do something with the function which may depend on workspace.
    m_domainCreator->initFunction(m_function);

    // get the cost function which must be a CostFuncFitting
    m_costFunction = boost::dynamic_pointer_cast<CostFunctions::CostFuncFitting>(
        API::CostFunctionFactory::Instance().create(
            getPropertyValue("CostFunction")));

    m_costFunction->setFittingFunction(m_function, domain, values);

  }

  double value = m_costFunction->val();
  g_log.notice() << "Cost function " << value << "\n";

  // Store the result.
  setProperty("Value", value);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
