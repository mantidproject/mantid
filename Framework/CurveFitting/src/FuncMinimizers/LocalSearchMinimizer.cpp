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

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

DECLARE_FUNCMINIMIZER(LocalSearchMinimizer, LocalSearch)

using Functions::Chebfun;

namespace {

size_t iiter;
size_t iparam;
bool isGrad;

//-----------------------------------------------------------------------------
/// Helper class to calculate the chi squared along a direction in the parameter
/// space.
class Slice {
public:
  /// Constructor.
  /// @param function  :: The cost function.
  /// @param direction :: A normalised direction vector in the parameter space.
  Slice(API::ICostFunction &function, const std::vector<double> &direction)
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
  const std::vector<double> m_direction;
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
/// Normalise a vector to 1
bool normalise(std::vector<double> &direction) {
  //double norm = 0.0;
  //for(auto x : direction) {
  //  norm += x * x;
  //}
  //norm = sqrt(norm);
  //for(auto &x : direction) {
  //  x /= norm;
  //}

  double norm = sqrt(std::accumulate(direction.begin(), direction.end(), 0.0,
                              [](double init, double x) { return init + x * x; }));
  if (norm == 0.0 || !boost::math::isfinite(norm)) {
    return false;
  }
  std::transform(direction.begin(), direction.end(), direction.begin(),
                 std::bind2nd(std::divides<double>(), norm));
  return true;
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
  double x = shift;
  double fun1 = fun(x);
  //std::cerr << "extent fun " << fun0 << ' ' << fun1 << ' ' << shift <<
  //std::endl;
  if (fun1 >= fun0) {
    shift = -shift;
    x = shift;
    fun0 = fun1;
    fun1 = fun(x);
    //std::cerr << "extent fun " << fun1 << ' ' << shift << std::endl;
    if (fun1 >= fun0) {
      return std::make_tuple(shift, -shift, makeAccuracy(fun1, fun1 - fun0), isGood);
    }
  }
  size_t count = 0;
  double maxDifference = fun0 - fun1;
  double xAtMaxDifference = x;
  bool canStop = false;
  std::vector<double> X, Y;
  for (size_t i = 0; i < 100; ++i) {
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
        if (ratio > 0.5) {
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

    // std::cerr << "extent " << i << ' ' << shift << std::endl;
    x += shift;
    fun1 = fun(x);
    if (boost::math::isfinite(fun1)){
      X.push_back(x);
      Y.push_back(fun1);
    }
    ++count;
  }

  if (!boost::math::isfinite(fun1)) {
    x = xAtMaxDifference;
    std::cerr << "Revert to " << x << " fun1= " << fun(x) << std::endl;
  }

  auto xLeft = 0.0;
  auto xRight = x;
  auto accuracy = makeAccuracy(fun0, maxDifference);
  if (xLeft > xRight) {
    std::swap(xLeft, xRight);
  }

  std::string suffix = "_" + boost::lexical_cast<std::string>(iiter) + "_" + (isGrad? "grad" : boost::lexical_cast<std::string>(iparam));
  std::vector<double> XX, YY;
  for(size_t i = 0; i < X.size(); ++i)
  {
    if (X[i] >= xLeft && X[i] <= xRight){
      XX.push_back(X[i]);
      YY.push_back(Y[i]);
    }
  }
  CHECK_OUT_2("x"+suffix, XX);
  CHECK_OUT_2("y"+suffix, YY);
  std::cerr << count << " evaluations" << std::endl;

  return std::make_tuple(xLeft, xRight, accuracy, isGood);
}

//-----------------------------------------------------------------------------
std::vector<double> getParameters(const API::ICostFunction &function) {
  std::vector<double> parameters(function.nParams());
  for (size_t i = 0; i < parameters.size(); ++i) {
    parameters[i] = function.getParameter(i);
  }
  return parameters;
}

//-----------------------------------------------------------------------------
bool setParameters(API::ICostFunction &function,
                       const std::vector<double> &parameters) {
  for(auto p : parameters) {
    if (!boost::math::isfinite(p)) {
      return false;
    }
  }
  for (size_t i = 0; i < parameters.size(); ++i) {
    function.setParameter(i, parameters[i]);
  }
  return true;
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
      //if (cheb.startX() != 0.0 && cheb.endX() != 0.0) {
      //  throw std::logic_error("LocalSearchMinimizer: slice interval must have a 0 at one end of the interval.");
      //}
      return std::make_tuple(0.0, valueAtStartX);
    } else if (valueAtStartX < valueAtEndX) {
      return std::make_tuple(cheb.startX(), valueAtStartX);
    } else {
      return std::make_tuple(cheb.endX(), valueAtEndX);
    }
  }
  auto minima = cheb(roots);
  auto lowestMinimum = std::min(minima.begin(), minima.end());
  auto argumentAtMinimum = roots[std::distance(minima.begin(), lowestMinimum)];
  auto valueAtMinimum = cheb(argumentAtMinimum);
  return std::make_tuple(argumentAtMinimum, valueAtMinimum);
}

//-----------------------------------------------------------------------------
/// Perform an iteration by searching for the minimum on a grid defined by
/// Chebfun slices.
void iterationSearch(API::ICostFunction &function,
                     const std::vector<Chebfun> &slices) {
  std::cerr << "Search" << std::endl;
  size_t n = function.nParams();
  size_t nPoints = 1;
  std::vector<std::vector<double>> ps;
  std::vector<size_t> multiSizes(n);
  for (size_t j = 0; j < n; ++j) {
    auto x = slices[j].getAllXPoints();
    nPoints *= x.size();
    multiSizes[j] = x.size();
    ps.push_back(x);
  }
  auto p = getParameters(function);
  auto p0 = p;
  double funMin = function.val();
  std::vector<size_t> multiIndex(n);
  for (size_t i = 0; i < nPoints; ++i) {
    auto f = function.val();
    if (f < funMin) {
      funMin = f;
      p = getParameters(function);
    }
    multiIndex[0] += 1;
    for (size_t j = 0; j < n - 1; ++j) {
      if (multiIndex[j] == multiSizes[j]) {
        multiIndex[j] = 0;
        auto m1 = multiIndex[j + 1] + 1;
        multiIndex[j + 1] = m1;
        function.setParameter(j, p0[j] + ps[j][0]);
        if (m1 < multiSizes[j + 1]) {
          function.setParameter(j + 1, p0[j + 1] + ps[j + 1][m1]);
        }
      } else {
        function.setParameter(j, p0[j] + ps[j][multiIndex[j]]);
        break;
      }
    }
  }

  setParameters(function, p);
}

//-----------------------------------------------------------------------------
/// Perform an iteration of the Newton algorithm.
bool iterationNewton(API::ICostFunction &function) {
  auto fittingFunction =
      dynamic_cast<CostFunctions::CostFuncFitting *>(&function);
  auto n = function.nParams();
  if (fittingFunction) {
    std::cerr << "Newton" << std::endl;
    auto hessian = fittingFunction->getHessian();
    auto derivatives = fittingFunction->getDeriv();

    // Scaling factors
    std::vector<double> scalingFactors(n);

    for (size_t i = 0; i < n; ++i) {
      double tmp = hessian.get(i, i);
      scalingFactors[i] = sqrt(tmp);
      if (tmp == 0.0) {
        // treat this case as a logic error for now
        //throw std::logic_error("Singular matrix in Newton iteration.");
        return false;
      }
    }

    // Apply scaling
    for (size_t i = 0; i < n; ++i) {
      double d = derivatives.get(i);
      derivatives.set(i, d / scalingFactors[i]);
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

    // Parameter corrections
    GSLVector corrections(n);
    // To find dx solve the system of linear equations   hessian * corrections == -derivatives
    derivatives *= -1.0;
    hessian.solve(derivatives, corrections);

    // Apply the corrections
    auto parameters = getParameters(function);
    auto oldParameters = parameters;
    auto oldCostFunction = function.val();
    for (size_t i = 0; i < n; ++i) {
      parameters[i] += corrections.get(i) / scalingFactors[i];
    }
    bool paramsAreOK = setParameters(function, parameters);
    if (!paramsAreOK) {
      return false;
    }
    if (function.val() > oldCostFunction) {
      setParameters(function, oldParameters);
      return false;
    }
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
/// Perform an iteration of the gradient descent algorithm.
bool iterationGradientDescent(API::ICostFunction &function,
                              const std::vector<Chebfun> &slices, double accuracy) {
  std::cerr << "gradient" << std::endl;
  auto params = getParameters(function);
  std::vector<double> negativeGradient;
  double extent = 0.0;
  auto n = function.nParams();
  negativeGradient.reserve(n);
  for(size_t i = 0; i < n; ++i){
    auto &slice = slices[i];
    auto derivative = slice.derivative();
    negativeGradient.push_back(-derivative(0.0));
    auto width = slice.width();
    extent += width * width;
  }
  extent = sqrt(extent);
  if (!normalise(negativeGradient)) {
    return false;
  }
  Slice slice(function, negativeGradient);
  double minBound = 0.0;
  double maxBound = 0.0;
  double accuracy1 = 0.0;
  bool isGood = false;
  isGrad = true;
  std::tie(minBound, maxBound, accuracy1, isGood) = findExtent(slice, 0.0);
  Chebfun cheb(slice, minBound, maxBound, accuracy);

  //auto x = cheb.linspace();
  //auto suffix = "_" + boost::lexical_cast<std::string>(iiter);
  //CHECK_OUT_2("x"+suffix, x);
  //CHECK_OUT_2("y"+suffix, cheb(x));

  double parameterShiftAtMinimum = 0.0;
  double costFunctionMinimum = 0.0;
  std::tie(parameterShiftAtMinimum, costFunctionMinimum) = findMinimum(cheb);
  std::cerr << "minimum " << costFunctionMinimum << std::endl;

  boost::optional<size_t> indexOfParameterWithLowerMinimum;
  for(size_t i = 0; i < slices.size(); ++i) {
    auto &slice = slices[i];
    double paramMin = 0;
    double valueMin = 0;
    std::tie(paramMin, valueMin) = findMinimum(slice);
    if (valueMin < costFunctionMinimum) {
      indexOfParameterWithLowerMinimum = i;
      parameterShiftAtMinimum = paramMin;
      costFunctionMinimum = valueMin;
      std::cerr << "lower minimum " << costFunctionMinimum << " for " << i << " at " << params[i] + parameterShiftAtMinimum << std::endl;
    }
  }

  if (indexOfParameterWithLowerMinimum) {
    params[indexOfParameterWithLowerMinimum.get()] += parameterShiftAtMinimum;
  } else {
    for(size_t i = 0; i < n; ++i){
      params[i] += negativeGradient[i] * parameterShiftAtMinimum;
    }
  }

  auto ok = setParameters(function, params);
  if (!ok) {
    std::cerr << "gradient not ok" << std::endl;
  }
  return ok;
}

} // anonymous namespace

//-----------------------------------------------------------------------------
/// Constructor
LocalSearchMinimizer::LocalSearchMinimizer() {}

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
}

//-----------------------------------------------------------------------------
/// Do one iteration.
bool LocalSearchMinimizer::iterate(size_t iter) {

  iiter = iter;

  if (iter >= 20) {
    return false;
  }

  size_t n = m_costFunction->nParams();

  std::vector<Chebfun> slices;
  slices.reserve(n);
  bool allQuadratics = true;
  double lowestAccuracy = 0.0;
  for (size_t i = 0; i < n; ++i) {
    double p = m_costFunction->getParameter(i);
    std::vector<double> dir(n, 0.0);
    dir[i] = 1.0;
    Slice slice(*m_costFunction, dir);
    double minBound = 0.0;
    double maxBound = 0.0;
    double accuracy = 0.0;
    bool isGood = false;
    iparam = i;
    isGrad = false;
    std::tie(minBound, maxBound, accuracy, isGood) = findExtent(slice, p);
    if (accuracy > lowestAccuracy) {
      lowestAccuracy = accuracy;
    }
    Chebfun::Options options(accuracy, 3, 100, true);
    Chebfun cheb(slice, minBound, maxBound, options);
    slices.push_back(cheb);
    allQuadratics &= isGood && cheb.numberOfParts() == 1 && cheb.size() == 3;

    std::cerr << "slice " << minBound << ' ' << maxBound << ' ' << accuracy
              << ' ' << cheb.size() << ' ' << isGood << std::endl;
    auto si = "_" + boost::lexical_cast<std::string>(iter) + "_" +
              boost::lexical_cast<std::string>(i);
    //auto x = cheb.linspace();
    //auto y = cheb(x);
    //CHECK_OUT_2("xx" + si, x);
    //CHECK_OUT_2("yy" + si, y);
  }

  auto success = false; //iterationNewton(*m_costFunction);
  if (!success) {
    if (!iterationGradientDescent(*m_costFunction, slices, lowestAccuracy)) {
      iterationSearch(*m_costFunction, slices);
    }
  }

  std::cerr << "Params:" << std::endl;
  for(size_t i = 0; i < n; ++i) {
    std::cerr << "     " << m_costFunction->getParameter(i) << std::endl;
  }
  return true;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
