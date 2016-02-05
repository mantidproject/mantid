#include "MantidCurveFitting/FuncMinimizers/LocalSearchMinimizer.h"
#include "MantidCurveFitting/Functions/Chebfun.h"
#include "MantidCurveFitting/Functions/ChebfunBase.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

#include "MantidAPI/FuncMinimizerFactory.h"

#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/optional.hpp>
#include <algorithm>
#include <functional>
#include <numeric>
#include <tuple>

#include "C:/Users/hqs74821/Work/Mantid_stuff/Testing/class/MyTestDef.h"
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

DECLARE_FUNCMINIMIZER(LocalSearchMinimizer, LocalSearch)

using Functions::Chebfun;

namespace {

static size_t iiter = 0;
std::string suffix(size_t i) {
  return "_" + boost::lexical_cast<std::string>(iiter) + "_" +
         boost::lexical_cast<std::string>(i);
}
std::string suffix() {
  return "_" + boost::lexical_cast<std::string>(iiter);
}

void check_cheb(const Chebfun& cheb) {
  auto x = cheb.linspace();
  auto y = cheb(x);
  auto si = suffix();
  CHECK_OUT_2("x" + si, x);
  CHECK_OUT_2("y" + si, y);
}

typedef boost::optional<GSLVector> OptionalParameters;

//-----------------------------------------------------------------------------
/// Helper class to calculate the chi squared along a direction in the parameter
/// space.
class Slice {
public:
  /// Constructor.
  /// @param function  :: The cost function.
  /// @param direction :: A normalised direction vector in the parameter space.
  Slice(API::ICostFunction &function, const GSLVector &direction)
      : m_function(function), m_direction(direction) {}

  /// Calculate a value of the cost function along the chosen direction at a
  /// distance from the current point.
  /// @param p :: A distance from the starting point.
  double operator()(double p) {
    std::vector<double> par0(m_function.nParams());
    for (size_t ip = 0; ip < m_function.nParams(); ++ip) {
      par0[ip] = m_function.getParameter(ip);
      m_function.setParameter(ip, par0[ip] + p * m_direction[ip]);
    }
    double res = m_function.val();
    for (size_t ip = 0; ip < m_function.nParams(); ++ip) {
      m_function.setParameter(ip, par0[ip]);
    }
    return res;
  }

private:
  /// The cost function.
  API::ICostFunction &m_function;
  /// The direction of the slice.
  const GSLVector m_direction;
};

//-----------------------------------------------------------------------------
double makeAccuracy(double value1, double value2) {
  const auto highestAccuracy = Functions::ChebfunBase::defaultTolerance();
  if (value2 == 0.0) {
    return highestAccuracy;
  }
  auto accuracy = (value1 == 0.0) ? value2 : value2 / value1;
  accuracy = fabs(accuracy);
  if (accuracy > 1.0) accuracy = 1.0;
  accuracy = accuracy * 1e-4;
  return accuracy > highestAccuracy ? accuracy : highestAccuracy;
}

//-----------------------------------------------------------------------------
///
std::tuple<double, double, double, bool> findExtent(std::function<double(double)> fun,
                                              double paramValue) {
  const double fac = 1e-4;
  double shift = fabs(paramValue * fac);
  if (shift == 0.0) {
    shift = fac;
  }
  bool isGood = false;
  double fun0 = fun(0);
  if (!boost::math::isfinite(fun0)) {
    throw std::runtime_error("Cost function has non-finite value at starting point.0");
  }

  const size_t maxSteps = 100;
  size_t istep = 0;
  double x = shift;
  double fun1 = fun(x);
  if (!boost::math::isfinite(fun0)) {
    for (istep = 0; istep < maxSteps; ++istep) {
      x /= 2;
      fun1 = fun(x);
      if (boost::math::isfinite(fun1)) {
        break;
      }
    }
    throw std::runtime_error("Cost function has non-finite value at starting point.1");
  }
  if (fun1 >= fun0) {
    shift = -shift;
    x = shift;
    fun0 = fun1;
    fun1 = fun(x);
    if (!boost::math::isfinite(fun1)) {
      for (istep = 0; istep < maxSteps; ++istep) {
        x /= 2;
        fun1 = fun(x);
        if (boost::math::isfinite(fun1)) {
          break;
        }
      }
      if (istep == maxSteps) {
        return std::make_tuple(0.0, -shift, makeAccuracy(fun1, fun1 - fun0), isGood);
      }
    }
    if (fun1 >= fun0) {
      return std::make_tuple(shift, -shift, makeAccuracy(fun1, fun1 - fun0), isGood);
    }
  }
  size_t count = 0;
  double maxDifference = fun0 - fun1;
  double xAtMaxDifference = x;
  bool canStop = false;
  std::vector<double> X, Y;
  for (istep = 0; istep < maxSteps; ++istep) {
    double difference = fun0 - fun1;

    if (difference == 0.0) {
      break;
    }

    if (difference < 0.0) {
      auto ratio = fabs(difference) / maxDifference;
      if (ratio > 10.0) {
        x -= shift;
        shift *= 0.75;
      } else {
        if (ratio > 0.1) {
          isGood = true;
        }
        break;
      }
    } else {
      if (difference > maxDifference) {
        maxDifference = difference;
        xAtMaxDifference = x;
      }
      shift = x;
    }

    x += shift;
    fun1 = fun(x);
    if (boost::math::isfinite(fun1)){
      X.push_back(x);
      Y.push_back(fun1);
    }
    ++count;
  }

  if (!boost::math::isfinite(fun1) || istep == maxSteps) {
    x = xAtMaxDifference;
  }

  auto xLeft = 0.0;
  auto xRight = x;
  auto accuracy = makeAccuracy(fun0, maxDifference);
  if (xLeft > xRight) {
    std::swap(xLeft, xRight);
  }

  return std::make_tuple(xLeft, xRight, accuracy, isGood);
}

//-----------------------------------------------------------------------------
GSLVector getParameters(const API::ICostFunction &function) {
  GSLVector parameters(function.nParams());
  for (size_t i = 0; i < parameters.size(); ++i) {
    parameters[i] = function.getParameter(i);
  }
  return parameters;
}

//-----------------------------------------------------------------------------
bool checkParameters(const GSLVector &parameters) {
  for (size_t i = 0; i < parameters.size(); ++i) {
    if (!boost::math::isfinite(parameters[i])) {
      return false;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
void setParameters(API::ICostFunction &function,
                       const GSLVector &parameters) {
  for (size_t i = 0; i < parameters.size(); ++i) {
    function.setParameter(i, parameters[i]);
  }
}

//-----------------------------------------------------------------------------
Chebfun makeChebfunSlice(API::ICostFunction &function, const GSLVector& direction, double p = 0.0, bool *ok = nullptr) {
  Slice slice(function, direction);
  double minBound = 0.0;
  double maxBound = 0.0;
  double accuracy = 0.0;
  bool isGood = false;
  std::tie(minBound, maxBound, accuracy, isGood) = findExtent(slice, p);
  if (ok != nullptr) {
    *ok = isGood;
  }
  Chebfun::Options options(accuracy, 2, 20, true);
  return Chebfun(slice, minBound, maxBound, options);
}

//-----------------------------------------------------------------------------
/// Find the smallest minimum of a slice.
std::tuple<double, double> findMinimum(const Chebfun& cheb) {
  auto derivative = cheb.derivative();
  auto roots = derivative.roughRoots();
  if (roots.empty()) {
    auto valueAtStartX = cheb(cheb.startX());
    auto valueAtEndX = cheb(cheb.endX());
    if (valueAtStartX == valueAtEndX) {
      return std::make_tuple(0.0, valueAtStartX);
    } else if (valueAtStartX < valueAtEndX) {
      return std::make_tuple(cheb.startX(), valueAtStartX);
    } else {
      return std::make_tuple(cheb.endX(), valueAtEndX);
    }
  }
  auto breakPoints = cheb.getBreakPoints();
  roots.insert(roots.end(), breakPoints.begin(), breakPoints.end());
  std::sort(roots.begin(), roots.end());
  auto minima = cheb(roots);
  auto lowestMinimum = std::min_element(minima.begin(), minima.end());
  auto argumentAtMinimum = roots[std::distance(minima.begin(), lowestMinimum)];
  if (boost::math::isfinite(argumentAtMinimum)) {
    auto valueAtMinimum = cheb(argumentAtMinimum);
    return std::make_tuple(argumentAtMinimum, valueAtMinimum);
  }
  return std::make_tuple(0.0, std::numeric_limits<double>::infinity());
}

//-----------------------------------------------------------------------------
const GSLMatrix& getHessian(const API::ICostFunction &function) {
  auto fittingFunction =
      dynamic_cast<const CostFunctions::CostFuncFitting *>(&function);
  if (fittingFunction) {
    return fittingFunction->getHessian();
  } else {
    throw std::runtime_error("Cannot return the Hessian: the cost function doesn't support it.");
  }
}

//-----------------------------------------------------------------------------
/// Perform an iteration of the Newton algorithm.
OptionalParameters iterationLMStep(API::ICostFunction &function, const GSLVector& newtonParameters) {
  auto oldParameters = getParameters(function);
  GSLVector direction = newtonParameters;
  direction -= oldParameters;
  setParameters(function, oldParameters);
  
  OptionalParameters parameters;
  try {
    direction.normalize();
    auto cheb = makeChebfunSlice(function, direction);
    double paramMin = 0;
    double valueMin = 0;
    std::tie(paramMin, valueMin) = findMinimum(cheb);
    parameters = oldParameters;
    direction *= paramMin;
    parameters.get() += direction;
    if (!checkParameters(parameters.get())) {
      parameters = OptionalParameters();
    } else {
      std::cerr << "LM step " << valueMin << ' ' << cheb.size() << ' ' << cheb.startX() << ' ' << cheb.endX() << std::endl;
    }
  } catch (std::runtime_error&) {
    parameters = OptionalParameters();
  }
  return parameters;
}
//-----------------------------------------------------------------------------
/// Perform an iteration of the gradient descent algorithm.
OptionalParameters iterationGradientDescent(API::ICostFunction &function) {
  auto params = getParameters(function);

  auto n = function.nParams();
  auto fittingFunction =
      dynamic_cast<CostFunctions::CostFuncFitting *>(&function);

  if (!fittingFunction) {
    return OptionalParameters();
  }

  GSLVector negativeGradient = fittingFunction->getDeriv();
  negativeGradient *= -1.0;

  try {
    negativeGradient.normalize();
  } catch(std::runtime_error&) {
    return OptionalParameters();
  }

  Chebfun cheb = makeChebfunSlice(function, negativeGradient);
  double paramMin = 0.0;
  double valueMin = 0.0;
  std::tie(paramMin, valueMin) = findMinimum(cheb);

  negativeGradient *= paramMin;
  params += negativeGradient;

  if (!checkParameters(params)) {
    return OptionalParameters();
  }

  std::cerr << "gradient " << valueMin << std::endl;
  return params;
}


//-----------------------------------------------------------------------------
/// Perform an iteration of the Newton algorithm.
OptionalParameters iterationNewton(API::ICostFunction &function, bool smallChange) {
  auto fittingFunction =
      dynamic_cast<CostFunctions::CostFuncFitting *>(&function);
  if (fittingFunction) {
    auto n = function.nParams();
    auto parameters = getParameters(function);

    auto hessian = fittingFunction->getHessian();
    auto negativeDerivatives = fittingFunction->getDeriv();
    negativeDerivatives *= -1.0;

    // Calculate corrections to the parameters
    GSLVector corrections(n);
    // Scaling factors
    std::vector<double> scalingFactors(n);

    for (size_t i = 0; i < n; ++i) {
      double tmp = hessian.get(i, i);
      scalingFactors[i] = sqrt(tmp);
      if (tmp == 0.0) {
        return OptionalParameters();
      }
    }

    // Apply scaling
    for (size_t i = 0; i < n; ++i) {
      double d = negativeDerivatives.get(i);
      negativeDerivatives.set(i, d / scalingFactors[i]);
      for (size_t j = i; j < n; ++j) {
        const double f = scalingFactors[i] * scalingFactors[j];
        double tmp = hessian.get(i, j);
        hessian.set(i, j, tmp / f);
        if (i != j) {
          tmp = hessian.get(j, i);
          hessian.set(j, i, tmp / f);
        }
      }
    }

    // To find dx solve the system of linear equations   hessian * corrections
    // == -derivatives
    hessian.solve(negativeDerivatives, corrections);

    // Apply the corrections
    for (size_t i = 0; i < n; ++i) {
      parameters[i] += corrections.get(i) / scalingFactors[i];
    }

    if (!checkParameters(parameters)) {
      return OptionalParameters();
    }

    std::cerr << "Newton " << std::endl;
    return parameters;
  }
  return OptionalParameters();
}


//-----------------------------------------------------------------------------
/// 
OptionalParameters iterationSingleParameters(API::ICostFunction &function, double oldValue, bool lastIteration = false) {
  auto n = function.nParams();
  std::vector<Chebfun> slices;
  slices.reserve(n);
  std::vector<double> parametersAtMinimum(n);
  std::vector<double> valuesAtMinimum(n);
  std::vector<bool> converged(n);
  bool allConverged = true;
  size_t indexOfLowestMinimum = 0;

  for (size_t i = 0; i < n; ++i) {
    double p = function.getParameter(i);
    GSLVector dir(n);
    dir.zero();
    dir[i] = 1.0;

    bool ok = false;
    auto cheb = makeChebfunSlice(function, dir, p, &ok);
    auto accuracy = cheb.accuracy();
    slices.push_back(cheb);

    double paramMin = 0;
    double valueMin = 0;
    std::tie(paramMin, valueMin) = findMinimum(cheb);

    parametersAtMinimum[i] = p + paramMin;
    valuesAtMinimum[i] = valueMin;
    if (valueMin == 0.0 || fabs(valueMin - oldValue) / fabs(valueMin) < accuracy * 100) {
      converged[i] = true;
      std::cerr << "   converged " << i << std::endl;
    }
    allConverged = allConverged && converged[i];

    if (i == 0) {
      indexOfLowestMinimum = 0;
    } else if (valueMin < valuesAtMinimum[indexOfLowestMinimum]) {
      indexOfLowestMinimum = i;
    }
  }

  if (allConverged || lastIteration) {

    for(size_t i = 0; i < n; ++i) {
      std::cerr << "   slice " << i << ' ' << slices[i].size() << ' ' << slices[i].accuracy() << std::endl;
      auto si = "_" + boost::lexical_cast<std::string>(iiter) + "_" +
                boost::lexical_cast<std::string>(i);
      auto x = slices[i].linspace();
      auto y = slices[i](x);
      CHECK_OUT_2("xx" + si, x);
      CHECK_OUT_2("yy" + si, y);
    }

    return OptionalParameters();
  }

  OptionalParameters parameters = getParameters(function);
  parameters.get()[indexOfLowestMinimum] = parametersAtMinimum[indexOfLowestMinimum];
  return parameters;
}

//-----------------------------------------------------------------------------
void initializeDirections(API::ICostFunction &function,
                          std::vector<std::vector<double>> &directions) {
  auto n = function.nParams();
  directions.resize(n);
  for(size_t i = 0; i < n; ++i) {
    directions[i] = std::vector<double>(n);
    directions[i][i] = 1.0;
  }
}

//-----------------------------------------------------------------------------
/// Calculate eigenvalues and eigenvectors of the hessian.
void eigenSystem(const API::ICostFunction &function, GSLVector &e, GSLMatrix &V) {
  auto fittingFunction =
      dynamic_cast<const CostFunctions::CostFuncFitting *>(&function);
  if (fittingFunction) {
    auto hessian = fittingFunction->getHessian();
    //std::cerr << "Hessian:" << std::endl;
    //std::cerr << hessian << std::endl;
    hessian.eigenSystem(e, V);
    std::cerr << "Eigenvalues:\n" << e << std::endl;
    //std::cerr << "Eigenvectors:" << std::endl;
    //std::cerr << V << std::endl;
  }
}

//-----------------------------------------------------------------------------
OptionalParameters iterationSingularHessian(API::ICostFunction &function,
                                            GSLVector &e, GSLMatrix &V,
                                            bool smallChange) {
  auto parameters = getParameters(function);
  auto oldParameters = parameters;
  auto oldValue = function.val();
  bool isHessianSingular = false;
  std::vector<size_t> badIndices;
  std::vector<GSLVector> badSpace;
  auto hessian = getHessian(function);
  for (size_t i = 0; i < e.size(); ++i) {
    auto ratio = fabs(e[i] / e[0]);
    if (ratio <= 1e-5 && ratio > 1e-14) { // < literals
      badIndices.push_back(i);
      badSpace.push_back(V.copyColumn(i));

      double maxCorr = 0.0;
      size_t jMaxCorr = i;
      for (size_t j = 0; j < e.size(); ++j) {
        if (i == j)
          continue;
        double r = fabs(e[i] / e[j]);
        if (r <= 1e-5) {
          double corr =
              e[i] - (e[i] * pow(V.get(i, i), 2) + e[j] * pow(V.get(i, j), 2));
          corr = fabs(corr / e[i]);
          if (corr > maxCorr) {
            maxCorr = corr;
            jMaxCorr = j;
          }
        }
      }
      std::cerr << "Correlation " << i << ' ' << maxCorr << ' ' << jMaxCorr
                << std::endl;
      if (maxCorr > 0.1) {
        isHessianSingular = true;
      }
    }
  }

  if (!isHessianSingular) {
    return OptionalParameters();
  }

  if (badIndices.size() > 0 && smallChange) {
    std::cerr << "Steps in bad direction" << std::endl;
    double minValue = oldValue;
    GSLVector minParameters;
    for (auto &badVector : badSpace) {
      parameters = oldParameters;
      parameters += badVector;
      auto newParameters = iterationLMStep(function, parameters);
      if (newParameters) {
        setParameters(function, newParameters.get());
        auto value = function.val();
        if (value < minValue) {
          minValue = value;
          minParameters = newParameters.get();
        }
      }
    }
    if (minValue != oldValue) {
      std::cerr << "Min bad value " << minValue << std::endl;
      return OptionalParameters(minParameters);
    } else {
      std::cerr << "Min bad value is bad" << std::endl;
      return OptionalParameters();
    }
  }
  return iterationGradientDescent(function);
}

//-----------------------------------------------------------------------------
OptionalParameters iterationRecoverFromBadIteration(API::ICostFunction &function, const GSLVector &oldParameters) {
  auto dir = getParameters(function);
  dir -= oldParameters;
  dir *= 0.5;
  dir += oldParameters;
  return OptionalParameters(dir);
}

} // anonymous namespace

//-----------------------------------------------------------------------------
/// Constructor
LocalSearchMinimizer::LocalSearchMinimizer():m_smallChange(false), m_parametersInitialized(false), m_badIteration(false) {}

//-----------------------------------------------------------------------------
/// Return current value of the cost function
double LocalSearchMinimizer::costFunctionVal() {
  return m_costFunction ? m_costFunction->val() : 0.0;
}

//-----------------------------------------------------------------------------
/// Initialize minimizer, i.e. pass a function to minimize.
void LocalSearchMinimizer::initialize(API::ICostFunction_sptr function,
                                      size_t maxIterations) {
  (void)maxIterations;
  m_costFunction = function;
  initializeDirections(*m_costFunction, m_directions);
}

//-----------------------------------------------------------------------------
void LocalSearchMinimizer::checkStatus(const GSLVector &e, const GSLMatrix &V) {
  if (!m_parametersInitialized) {
    m_badParameters.resize(e.size(), GOOD);
  }
  m_badIteration = false;
  size_t nBads = 0;
  size_t nVeryBads = 0;
  for(size_t i = 0; i < e.size(); ++i) {
    auto ratio = fabs(e[i] / e[0]);
    if (ratio < 1e-5) { // < replace literal
      ++nBads;
    } else {
      if (m_badParameters[i] == BAD) {
        std::cerr << "Recovered parameter " << i << std::endl;
      }
      m_badParameters[i] = GOOD;
    }
    if (ratio < 1e-14) { // < replace literal
      if (!m_parametersInitialized) {
        m_badParameters[i] = BAD;
        std::cerr << "Bad parameter " << i << std::endl;
        //throw std::runtime_error("Bad parameters found.");
      } else if (m_badParameters[i] == GOOD) {
        std::cerr << "Bad parameter " << i << std::endl;
        ++nVeryBads;
      }
    }
  }
  m_badIteration = (nBads > 0 && nBads == nVeryBads);
  m_parametersInitialized = true;
}

//-----------------------------------------------------------------------------
/// Do one iteration.
bool LocalSearchMinimizer::iterate(size_t iter) {

  iiter = iter;

  auto oldParameters = getParameters(*m_costFunction);
  auto oldValue = m_costFunction->val();
  std::cerr << "\nIteration " << iter << ' ' << oldValue << std::endl;

  GSLVector e;
  GSLMatrix V;

  eigenSystem(*m_costFunction, e, V);

  checkStatus(e, V);
  std::cerr << "Bad? " << m_badIteration << std::endl;

  OptionalParameters parameters;

  if (m_badIteration) {
    parameters = iterationRecoverFromBadIteration(*m_costFunction, GSLVector(m_oldParameters));
    setParameters(*m_costFunction, parameters.get());
    return true;
  }

  parameters = iterationSingularHessian(*m_costFunction, e, V, m_smallChange);

  if (!parameters) {
    parameters = iterationNewton(*m_costFunction, m_smallChange);

    if (parameters) {
      setParameters(*m_costFunction, parameters.get());
      auto value = m_costFunction->val();
      std::cerr << "   " << value << std::endl;
      if (!boost::math::isfinite(value) || value > oldValue) {
        std::cerr << "Going for LM step" << std::endl;
        setParameters(*m_costFunction, oldParameters);
        parameters = iterationLMStep(*m_costFunction, parameters.get());
        setParameters(*m_costFunction, parameters.get());
        value = m_costFunction->val();
      }

      if (!boost::math::isfinite(value) || value >= oldValue) {
        std::cerr << "Too large value" << std::endl;
        setParameters(*m_costFunction, oldParameters);
        parameters = OptionalParameters();
      }
    }
  }

  if (!parameters) {
    parameters = iterationGradientDescent(*m_costFunction);
  }

  if (parameters) {
    setParameters(*m_costFunction, parameters.get());
  }

  double newValue = m_costFunction->val();
  if (!boost::math::isfinite(newValue)) {
    throw std::runtime_error("New params give inf.");
  }

  if (newValue == 0.0) {
    return false;
  }

  if (!parameters || fabs(oldValue / newValue) < 1.001) { // < replace literal
    std::cerr << "Small change" << std::endl;
    m_smallChange = true;
    auto altParameters = iterationSingleParameters(*m_costFunction, oldValue);
    if (!altParameters) {
      return false;
    }
    setParameters(*m_costFunction, altParameters.get());
    auto altValue = m_costFunction->val();
    if (altValue >= newValue) {
      if (parameters) {
        if (fabs(oldValue / newValue) < 1.0 + 1e-8) { // < replace literal
          iterationSingleParameters(*m_costFunction, oldValue, true);
          return false;
        }
        setParameters(*m_costFunction, parameters.get());
      } else {
        throw std::runtime_error("Minimizer failed.");
      }
    } else {
      std::cerr << "Single parameter " << altValue << std::endl;
    }
  } else {
    m_smallChange = false;
  }

  double delta = newValue - oldValue;
  std::cerr << "Delta " << delta << std::endl;
  if (delta < 0) {
    m_oldParameters = oldParameters.toStdVector();
  } else {
    setParameters(*m_costFunction, oldParameters);
    m_smallChange = true;
  }

  return true;
}


} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
