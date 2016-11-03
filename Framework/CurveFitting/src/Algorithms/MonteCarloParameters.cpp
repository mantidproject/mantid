#include "MantidCurveFitting/Algorithms/MonteCarloParameters.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidKernel/MersenneTwister.h"

#include <numeric>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MonteCarloParameters)

namespace {

/// Get parameters of a cost function as a vector of doubles.
std::vector<double> getParameters(const CostFunctions::CostFuncFitting &costFunc) {
  std::vector<double> out;
  auto n = costFunc.nParams();
  if (n > 0) {
    out.reserve(n);
    for (size_t i = 0; i < n; ++i) {
      out.push_back(costFunc.getParameter(i));
    }
  }
  return out;
}

/// Set parameters of a cost function from a vector.
void setParameters(CostFunctions::CostFuncFitting &costFunc, const std::vector<double> &params) {
  if (params.size() != costFunc.nParams()) {
    throw std::logic_error("Number of parameters of a cost function doesn't "
                           "match the size of an input vector.");
  }
  for (size_t i = 0; i < params.size(); ++i) {
    costFunc.setParameter(i, params[i]);
  }
  costFunc.applyTies();
}

/// Return a sum of constraints penalty values.
double
getConstraints(const std::vector<std::unique_ptr<IConstraint>> &constraints) {
  try {
    return std::accumulate(constraints.begin(), constraints.end(), 0.0,
                           [](double s, const std::unique_ptr<IConstraint> &c) {
                             return s + c->check();
                           });
  } catch (...) {
    return 0.0;
  }
}

} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string MonteCarloParameters::name() const {
  return "MonteCarloParameters";
}

/// Algorithm's version for identification. @see Algorithm::version
int MonteCarloParameters::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string MonteCarloParameters::summary() const {
  return "Estimate parameters of a fitting function using a Monte Carlo "
         "algorithm.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void MonteCarloParameters::initConcrete() {
  declareCostFunctionProperty();
  declareProperty("NIterations", 100, "Number of iterations to run.");
  declareProperty("Constraints","","Additional constraints on tied parameters.");
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void MonteCarloParameters::execConcrete() {
  auto costFunction = getCostFunctionProperty();
  auto nParams = costFunction->nParams();
  auto func = costFunction->getFittingFunction();

  // Use additional constraints on parameters tied in some way
  // to the varied parameters to exculde unwanted results.
  std::vector<std::unique_ptr<IConstraint>> constraints;
  std::string constraintStr = getProperty("Constraints");
  if (!constraintStr.empty()) {
    Expression expr;
    expr.parse(constraintStr);
    expr.toList();
    for (auto &term : expr.terms()) {
      IConstraint *c =
          ConstraintFactory::Instance().createInitialized(func.get(), term);
      constraints.push_back(std::unique_ptr<IConstraint>(c));
    }
  }

  Kernel::MersenneTwister randGenerator;
  // Ranges to use with random number generators: one for each free parameter.
  std::vector<std::pair<double, double>> ranges;
  ranges.reserve(nParams);
  for (size_t i = 0; i < func->nParams(); ++i) {
    if (func->isFixed(i)) {
      continue;
    }
    auto constraint = func->getConstraint(i);
    if (constraint == nullptr) {
      func->fix(i);
      continue;
    }
    auto boundary = dynamic_cast<Constraints::BoundaryConstraint *>(constraint);
    if (boundary == nullptr) {
      throw std::runtime_error("Parameter " + func->parameterName(i) +
                               " must have a boundary constraint. ");
    }
    if (!boundary->hasLower()) {
      throw std::runtime_error("Constraint of " + func->parameterName(i) +
                               " must have a lower bound.");
    }
    if (!boundary->hasUpper()) {
      throw std::runtime_error("Constraint of " + func->parameterName(i) +
                               " must have an upper bound.");
    }
    // Use the lower and upper bounds of the constraint to set the range
    // of a generator with uniform distribution.
    ranges.push_back(std::make_pair(boundary->lower(), boundary->upper()));
  }
  // Number of parameters could have changed
  costFunction->reset();
  nParams = costFunction->nParams();

  std::vector<double> bestParams = getParameters(*costFunction);
  double bestValue = costFunction->val() + getConstraints(constraints);

  // Implicit cast from int to size_t
  size_t nIter = static_cast<int>(getProperty("NIterations"));
  for(size_t it = 0; it < nIter; ++it) {
    for(size_t i = 0; i < nParams; ++i) {
      const auto &range = ranges[i];
      costFunction->setParameter(i, randGenerator.nextValue(range.first, range.second));
    }
    costFunction->applyTies();
    if (getConstraints(constraints) > 0.0) {
      continue;
    }
    auto value = costFunction->val();
    if (value < bestValue) {
      bestValue = value;
      bestParams = getParameters(*costFunction);
    }
  }
  setParameters(*costFunction, bestParams);
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
