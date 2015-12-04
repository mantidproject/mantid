#include "MantidCurveFitting/FuncMinimizers/LocalSearchMinimizer.h"
#include "MantidCurveFitting/Functions/Chebfun.h"
#include "MantidCurveFitting/GSLVector.h"
#include "MantidCurveFitting/CostFunctions/CostFuncFitting.h"

#include "MantidAPI/FuncMinimizerFactory.h"

#include <boost/lexical_cast.hpp>
#include <functional>
#include <tuple>

#include "C:/Users/hqs74821/Work/Mantid_stuff/Testing/class/MyTestDef.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

DECLARE_FUNCMINIMIZER(LocalSearchMinimizer, LocalSearch)

using Functions::Chebfun;

namespace {

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
  const GSLVector &m_direction;
};

//-----------------------------------------------------------------------------
double makeAccuracy(double value1, double value2) {
  auto accuracy = (value1 == 0.0) ? value2 : value2 / value1;
  if (accuracy > 1.0) accuracy = 1.0;
  return fabs(accuracy * 1e-4);
}
//-----------------------------------------------------------------------------
///
std::tuple<double, double, double> findExtent(std::function<double(double)> fun,
                                              double startX) {
  const double fac = 1e-4;
  double shift = fabs(startX * fac);
  if (shift == 0.0) {
    shift = fac;
  }
  double fun0 = fun(startX);
  double x = startX + shift;
  double fun1 = fun(x);
  // std::cerr << "extent fun " << fun0 << ' ' << fun1 << ' ' << shift <<
  // std::endl;
  if (fun1 >= fun0) {
    shift = -shift;
    x = startX + shift;
    fun1 = fun(x);
    // std::cerr << "extent fun " << fun1 << ' ' << shift << std::endl;
    if (fun1 >= fun0) {
      return std::make_tuple(startX + shift, startX - shift,
                             makeAccuracy(fun0, fun1));
    }
  }
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
      if (fabs(difference) / maxDifference > 10.0) {
        x -= shift;
        shift *= 0.75;
      } else {
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
    X.push_back(x);
    Y.push_back(fun1);
  }
  CHECK_OUT_2("sx", X);
  CHECK_OUT_2("sy", Y);

  auto accuracy = makeAccuracy(fun0, maxDifference);
  if (shift > 0) {
    return std::make_tuple(startX, x, accuracy);
  }
  return std::make_tuple(x, startX, accuracy);
}

//-----------------------------------------------------------------------------
std::vector<double> saveParameters(const API::ICostFunction &function) {
  std::vector<double> parameters(function.nParams());
  for (size_t i = 0; i < parameters.size(); ++i) {
    parameters[i] = function.getParameter(i);
  }
  return parameters;
}

//-----------------------------------------------------------------------------
void restoreParameters(API::ICostFunction &function,
                       const std::vector<double> &parameters) {
  for (size_t i = 0; i < parameters.size(); ++i) {
    function.setParameter(i, parameters[i]);
  }
}

//-----------------------------------------------------------------------------
/// Perform an iteration by searching for the minimum on a grid defined by
/// Chebfun slices.
void iterationSearch(API::ICostFunction &function,
                     const std::vector<Chebfun> &slices) {
  size_t n = function.nParams();
  size_t nPoints = 1;
  std::vector<std::vector<double>> ps;
  std::vector<size_t> multiSizes(n);
  for (size_t j = 0; j < n; ++j) {
    auto x = slices[j].getAllXPoints();
    nPoints *= x.size();
    multiSizes[j] = x.size();
    ps.push_back(x);
    function.setParameter(j, ps[j][0]);
  }
  auto p = saveParameters(function);
  double funMin = function.val();
  std::vector<size_t> multiIndex(n);
  for (size_t i = 0; i < nPoints; ++i) {
    auto f = function.val();
    // std::cerr << i << ' ' << function.getParameter(0) << ' '
    //  << function.getParameter(1) << ' ' << f << std::endl;
    // std::cerr << multiIndex[0] << ' ' << multiIndex[1] << std::endl;
    if (f < funMin) {
      funMin = f;
      p = saveParameters(function);
    }
    multiIndex[0] += 1;
    for (size_t j = 0; j < n - 1; ++j) {
      if (multiIndex[j] == multiSizes[j]) {
        multiIndex[j] = 0;
        auto m1 = multiIndex[j + 1] + 1;
        multiIndex[j + 1] = m1;
        function.setParameter(j, ps[j][0]);
        if (m1 < multiSizes[j + 1]) {
          function.setParameter(j + 1, ps[j + 1][m1]);
        }
      } else {
        function.setParameter(j, ps[j][multiIndex[j]]);
        break;
      }
    }
  }

  restoreParameters(function, p);
}

//-----------------------------------------------------------------------------
/// Perform an iteration of the Newton algorithm.
void iterationNewton(API::ICostFunction &function,
                     const std::vector<Chebfun> &slices) {
  auto fittingFunction =
      dynamic_cast<CostFunctions::CostFuncFitting *>(&function);
  auto n = function.nParams();
  if (fittingFunction) {
    auto hessian = fittingFunction->getHessian();
    auto derivatives = fittingFunction->getDeriv();

    // Scaling factors
    std::vector<double> scalingFactors(n);

    for (size_t i = 0; i < n; ++i) {
      double tmp = hessian.get(i, i);
      scalingFactors[i] = sqrt(tmp);
      if (tmp == 0.0) {
        // treat this case as a logic error for now
        throw std::logic_error("Singular matrix in Newton iteration.");
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
    auto parameters = saveParameters(function);
    for (size_t i = 0; i < n; ++i) {
      parameters[i] += corrections.get(i) / scalingFactors[i];
    }
    restoreParameters(function, parameters);
  }
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

  if (iter >= 10) return false;

  size_t n = m_costFunction->nParams();
  std::vector<Chebfun> slices;
  slices.reserve(n);
  bool allQuadratics = true;
  for (size_t i = 0; i < n; ++i) {
    GSLVector dir(n);
    dir.zero();
    dir[i] = 1.0;
    Slice slice(*m_costFunction, dir);
    double minBound = 0.0;
    double maxBound = 0.0;
    double accuracy = 0.0;
    std::tie(minBound, maxBound, accuracy) = findExtent(slice, 0.0);
    std::cerr << "chebfun " << i << ' ' << accuracy << std::endl;
    double p = m_costFunction->getParameter(i);
    Chebfun::Options options(accuracy, 3, 100, true);
    Chebfun cheb(slice, p + minBound, p + maxBound, options);
    std::cerr << "chebfun " << cheb.startX() << ' ' << cheb.endX() << ' '
              << cheb.numberOfParts() << ' ' << cheb.size() << std::endl;
    slices.push_back(cheb);
    allQuadratics &= cheb.numberOfParts() == 1 && cheb.size() == 3;

    auto si = boost::lexical_cast<std::string>(i);
    CHECK_OUT_2("x" + si, cheb.getAllXPoints());
    CHECK_OUT_2("y" + si, cheb.getAllYPoints());
  }

  if (iter > 3 && allQuadratics) {
    iterationNewton(*m_costFunction, slices);
  } else {
    iterationSearch(*m_costFunction, slices);
  }

  return true;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
