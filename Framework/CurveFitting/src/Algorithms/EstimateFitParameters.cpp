// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Algorithms/EstimateFitParameters.h"

#include "MantidAPI/ConstraintFactory.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"
#include "MantidCurveFitting/Functions/ChebfunBase.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/normal_distribution.h"

#include <map>
#include <numeric>
#include <random>

namespace Mantid {
namespace CurveFitting {
namespace Algorithms {

using namespace Kernel;
using namespace API;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(EstimateFitParameters)

namespace {

const size_t MAX_APPROX_SIZE = 129;
const double MIN_APPROX_TOLERANCE = 1e-4;

/// Return a sum of constraints penalty values.
double
getConstraints(const std::vector<std::unique_ptr<IConstraint>> &constraints) {
  return std::accumulate(constraints.begin(), constraints.end(), 0.0,
                         [](double s, const std::unique_ptr<IConstraint> &c) {
                           return s + c->check();
                         });
}

/// Defines a 1D slice of a cost function along direction of
/// one of its parameters.
struct Slice {
  double operator()(double p) {
    m_costFunction.setParameter(m_paramIndex, p);
    m_costFunction.applyTies();
    return m_costFunction.val();
  }
  /// The cost function
  CostFunctions::CostFuncFitting &m_costFunction;
  /// Index of the running parameter
  size_t m_paramIndex;
};

/// Try to estimate if any of the parameters can cause problems during fitting.
/// If such parameters are found - fix them.
/// @param costFunction :: The cost function.
/// @param ranges :: The ranges of values within which the parameters may
/// change.
void fixBadParameters(CostFunctions::CostFuncFitting &costFunction,
                      const std::vector<std::pair<double, double>> &ranges) {
  std::vector<size_t> indicesOfFixed;
  std::vector<double> P, A, D;
  auto &fun = *costFunction.getFittingFunction();
  for (size_t i = 0, j = 0; i < fun.nParams(); ++i) {
    if (!fun.isActive(i)) {
      continue;
    }
    auto lBound = ranges[j].first;
    auto rBound = ranges[j].second;
    auto storedParam = fun.getParameter(i);
    Slice slice{costFunction, j};
    auto base = Functions::ChebfunBase::bestFitAnyTolerance(
        lBound, rBound, slice, P, A, 1.0, MIN_APPROX_TOLERANCE,
        MAX_APPROX_SIZE);
    bool fix = false;
    if (!base) {
      fun.setParameter(i, storedParam);
      base = boost::make_shared<Functions::ChebfunBase>(
          MAX_APPROX_SIZE, lBound, rBound, MIN_APPROX_TOLERANCE);
      P = base->fit(slice);
      A = base->calcA(P);
    }
    base->derivative(A, D);
    auto roots = base->roots(D);
    if (!roots.empty()) {
      if (roots.size() * 2 >= base->size()) {
        // If the approximating polynomial oscillates too much
        // the parameter is likely to be bad.
        fix = true;
      }
    }
    fun.setParameter(i, storedParam);

    if (fix) {
      // Parameter is bad - fix it. Delay actual fixing until all bad ones
      // found.
      indicesOfFixed.push_back(i);
    }
    ++j;
  }
  for (auto i : indicesOfFixed) {
    fun.tie(fun.parameterName(i), std::to_string(fun.getParameter(i)));
  }
}

/// A class for sorting and storing sets of best function parameters.
class BestParameters {
  /// Maximum size of the store
  size_t m_size;
  /// Actual storage.
  std::map<double, GSLVector> m_params;

public:
  /// Constructor
  explicit BestParameters(size_t size) : m_size(size) {}
  /// Test a cost function value if corresponding parameters must be stored.
  bool isOneOfBest(double value) const {
    return m_params.size() < m_size || value < m_params.rbegin()->first;
  }
  /// Insert a set of parameters to the store.
  void insertParams(double value, const GSLVector &params) {
    if (m_params.size() == m_size) {
      auto it = m_params.find(m_params.rbegin()->first);
      m_params.erase(it);
    }
    m_params[value] = params;
  }
  /// Return all stored parameters, drop function values.
  std::vector<GSLVector> getParams() const {
    std::vector<GSLVector> res;
    res.reserve(m_params.size());
    for (auto &it : m_params) {
      res.emplace_back(it.second);
    }
    return res;
  }
};

/// Run the Monte Carlo version of the algorithm.
/// Generate random values of function parameters and return those that
/// give the smallest cost function.
/// @param costFunction :: The cost function.
/// @param ranges :: The ranges of values defining the uniform distributions for
/// the parameters.
/// @param constraints :: Additional constraints.
/// @param nSamples :: A number of samples to generate.
/// @param seed :: A seed for the random number generator.
std::vector<GSLVector>
runMonteCarlo(CostFunctions::CostFuncFitting &costFunction,
              const std::vector<std::pair<double, double>> &ranges,
              const std::vector<std::unique_ptr<IConstraint>> &constraints,
              const size_t nSamples, const size_t nOutput,
              const unsigned int seed) {
  std::mt19937 rng;
  if (seed != 0) {
    rng.seed(seed);
  }
  double value = costFunction.val() + getConstraints(constraints);
  auto nParams = costFunction.nParams();
  GSLVector params;
  costFunction.getParameters(params);
  BestParameters bestParams(nOutput);
  bestParams.insertParams(value, params);

  // Implicit cast from int to size_t
  for (size_t it = 0; it < nSamples; ++it) {
    for (size_t i = 0; i < nParams; ++i) {
      const auto &range = ranges[i];
      costFunction.setParameter(
          i, std::uniform_real_distribution<>(range.first, range.second)(rng));
    }
    costFunction.applyTies();
    if (getConstraints(constraints) > 0.0) {
      continue;
    }
    value = costFunction.val();
    if (costFunction.nParams() != nParams) {
      throw std::runtime_error("Cost function changed number of parameters " +
                               std::to_string(nParams) + " -> " +
                               std::to_string(costFunction.nParams()));
    }
    if (bestParams.isOneOfBest(value)) {
      costFunction.getParameters(params);
      bestParams.insertParams(value, params);
    }
  }
  auto outputParams = bestParams.getParams();
  costFunction.setParameters(outputParams.front());
  return outputParams;
}
/// Run the Cross Entropy version of the algorithm.
/// https://en.wikipedia.org/wiki/Cross-entropy_method
/// How it works:
///   1. Generate a few sets of function parameters such that their values are
///   drawn from
///   normal distributions with given means and sigmas.
///   2. Calculate the cost function for each set of parameters and select a
///   number of smallest values.
///   3. Find the sample means and sigmas of the parameters from the subset
///   selected in step 2.
///   4. Repeat steps 1 - 3 a few times.
///   5. Return parameters for the smallest cost function value found in the
///   last iteration.
/// @param costFunction :: The cost function.
/// @param ranges :: The ranges of values defining the initial distributions for
/// the parameters:
///     The middle of the range gives the mean and the spread from the mean is a
///     sigma.
/// @param constraints :: Additional constraints.
/// @param nSamples :: A number of parameter sets (samples) generated in step 1.
/// @param nSelection :: A number of sets selected in step 2.
/// @param nIterations :: A number of iterations of the algorithm.
/// @param seed :: A seed for the random number generator.
void runCrossEntropy(
    CostFunctions::CostFuncFitting &costFunction,
    const std::vector<std::pair<double, double>> &ranges,
    const std::vector<std::unique_ptr<IConstraint>> &constraints,
    size_t nSamples, size_t nSelection, size_t nIterations, unsigned int seed) {
  // Initialise the normal distribution parameters (mean and sigma for each
  // function parameter).
  std::vector<std::pair<double, double>> distributionParams;
  for (auto &range : ranges) {
    auto mean = (range.first + range.second) / 2;
    auto sigma = std::fabs(range.first - range.second) / 2;
    distributionParams.push_back(std::make_pair(mean, sigma));
  }

  auto nParams = costFunction.nParams();
  std::mt19937 rng;
  if (seed != 0) {
    rng.seed(seed);
  }
  // Sets of function parameters (GSLVector) and corresponding values of the
  // cost function (double)
  using ParameterSet = std::pair<double, GSLVector>;
  std::vector<ParameterSet> sampleSets(nSamples);
  // Function for comparing parameter sets.
  auto compareSets = [](const ParameterSet &p1, const ParameterSet &p2) {
    return p1.first < p2.first;
  };

  // Run nIterations of the algorithm
  for (size_t it = 0; it < nIterations; ++it) {
    for (size_t isam = 0; isam < nSamples; ++isam) {
      // Draw a set of function parameters from their distributions
      auto &paramSet = sampleSets[isam];
      if (paramSet.second.size() != nParams) {
        paramSet.second.resize(nParams);
      }
      for (size_t i = 0; i < nParams; ++i) {
        paramSet.second[i] = Kernel::normal_distribution<>(
            distributionParams[i].first, distributionParams[i].second)(rng);
      }
      // Calculate the cost function with those parameters
      costFunction.setParameters(paramSet.second);
      paramSet.first = costFunction.val() + getConstraints(constraints);
      if (costFunction.nParams() != nParams) {
        throw std::runtime_error("Cost function changed number of parameters " +
                                 std::to_string(nParams) + " -> " +
                                 std::to_string(costFunction.nParams()));
      }
    }
    // Partially sort the parameter sets in ascending order of the cost
    // function.
    // Find nSelection smallest values.
    std::partial_sort(sampleSets.begin(), sampleSets.begin() + nSelection,
                      sampleSets.end(), compareSets);
    // Estimate new distribution parameters from the sample of nSelection sets.
    GSLVector means(nParams);
    GSLVector variances(nParams);
    for (size_t isam = 0; isam < nSelection; ++isam) {
      auto &paramSet = sampleSets[isam];
      for (size_t i = 0; i < nParams; ++i) {
        auto p = paramSet.second[i];
        means[i] += p;
        variances[i] += p * p;
      }
    }
    means *= 1.0 / double(nSelection);
    variances *= 1.0 / double(nSelection);
    for (size_t i = 0; i < nParams; ++i) {
      auto mean = means[i];
      auto sigma = sqrt(variances[i] - mean * mean);
      distributionParams[i].first = mean;
      distributionParams[i].second = sigma;
    }
  }
  // Set parameters of the cost function to the best sample set.
  costFunction.setParameters(sampleSets.front().second);
}

} // namespace

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string EstimateFitParameters::name() const {
  return "EstimateFitParameters";
}

/// Algorithm's version for identification. @see Algorithm::version
int EstimateFitParameters::version() const { return 1; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string EstimateFitParameters::summary() const {
  return "Estimate parameters of a fitting function using a Monte Carlo "
         "algorithm.";
}

//----------------------------------------------------------------------------------------------
/// Initialize the algorithm's properties.
void EstimateFitParameters::initConcrete() {
  std::vector<std::string> types{"Monte Carlo", "Cross Entropy"};
  Kernel::StringListValidator TypesValidator(types);

  declareCostFunctionProperty();
  declareProperty("NSamples", 100, "Number of samples.");
  declareProperty("Constraints", "",
                  "Additional constraints on tied parameters.");
  declareProperty("Type", "Monte Carlo",
                  R"(Type of the algorithm: "Monte Carlo" or "Cross Entropy")");
  declareProperty("NOutputs", 10,
                  "Number of parameter sets to output to "
                  "OutputWorkspace. Unused if OutputWorkspace "
                  "isn't set. (Monte Carlo only)");
  declareProperty(Kernel::make_unique<WorkspaceProperty<ITableWorkspace>>(
                      "OutputWorkspace", "", Direction::Output,
                      Mantid::API::PropertyMode::Optional),
                  "Optional: A table workspace with parameter sets producing "
                  "the smallest values of cost function. (Monte Carlo only)");
  declareProperty("NIterations", 10,
                  "Number of iterations of the Cross Entropy algorithm.");
  declareProperty("Selection", 10,
                  "Size of the selection in the Cross Entropy "
                  "algorithm from which to estimate new "
                  "distribution parameters for the next "
                  "iteration.");
  declareProperty("FixBadParameters", false,
                  "If true try to estimate which "
                  "parameters may cause problems "
                  "for fitting and fix them.");
  declareProperty("Seed", 0,
                  "A seed value for the random number generator. "
                  "The default value (0) makes the generator use a "
                  "random seed.");
}

//----------------------------------------------------------------------------------------------
/// Execute the algorithm.
void EstimateFitParameters::execConcrete() {
  auto costFunction = getCostFunctionInitialized();
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

  // Ranges to use with random number generators: one for each free parameter.
  std::vector<std::pair<double, double>> ranges;
  ranges.reserve(costFunction->nParams());
  for (size_t i = 0; i < func->nParams(); ++i) {
    if (!func->isActive(i)) {
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
  if (costFunction->nParams() == 0) {
    throw std::runtime_error("No parameters are given for which to estimate "
                             "initial values. Set boundary constraints to "
                             "parameters that need to be estimated.");
  }

  size_t nSamples = static_cast<int>(getProperty("NSamples"));
  unsigned int seed = static_cast<int>(getProperty("Seed"));

  if (getPropertyValue("Type") == "Monte Carlo") {
    int nOutput = getProperty("NOutputs");
    auto outputWorkspaceProp = getPointerToProperty("OutputWorkspace");
    if (outputWorkspaceProp->isDefault() || nOutput <= 0) {
      nOutput = 1;
    }
    auto output = runMonteCarlo(*costFunction, ranges, constraints, nSamples,
                                static_cast<size_t>(nOutput), seed);

    if (!outputWorkspaceProp->isDefault()) {
      auto table = API::WorkspaceFactory::Instance().createTable();
      auto column = table->addColumn("str", "Name");
      column->setPlotType(6);
      for (size_t i = 0; i < output.size(); ++i) {
        column = table->addColumn("double", std::to_string(i + 1));
        column->setPlotType(2);
      }

      for (size_t i = 0, ia = 0; i < m_function->nParams(); ++i) {
        if (m_function->isActive(i)) {
          TableRow row = table->appendRow();
          row << m_function->parameterName(i);
          for (auto &j : output) {
            row << j[ia];
          }
          ++ia;
        }
      }
      setProperty("OutputWorkspace", table);
    }
  } else {
    size_t nSelection = static_cast<int>(getProperty("Selection"));
    size_t nIterations = static_cast<int>(getProperty("NIterations"));
    runCrossEntropy(*costFunction, ranges, constraints, nSamples, nSelection,
                    nIterations, seed);
  }
  bool fixBad = getProperty("FixBadParameters");
  if (fixBad) {
    fixBadParameters(*costFunction, ranges);
  }
}

} // namespace Algorithms
} // namespace CurveFitting
} // namespace Mantid
