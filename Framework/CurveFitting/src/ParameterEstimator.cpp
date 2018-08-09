#include "MantidCurveFitting/ParameterEstimator.h"
#include "MantidCurveFitting/Functions/SimpleChebfun.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include "MantidKernel/Logger.h"

#include <cmath>
#include <mutex>

namespace Mantid {
namespace CurveFitting {
namespace ParameterEstimator {

using namespace Functions;

/// The logger.
Kernel::Logger g_log("ParameterEstimator");

namespace {
/// Mutex to prevent simultaneous access to functionMap
std::recursive_mutex FUNCTION_MAP_MUTEX;
} // namespace

enum Function { None, Gaussian, Lorentzian, BackToBackExponential };
using FunctionMapType = std::map<std::string, std::pair<size_t, Function>>;

//----------------------------------------------------------------------------------------------

/// Initializes a FunctionMapType object
/// @param functionMapType :: the function map to initialize
void initFunctionLookup(FunctionMapType &functionMapType) {
  assert(functionMapType.empty());

  functionMapType["Gaussian"] = std::make_pair(2, Gaussian);
  functionMapType["Lorentzian"] = std::make_pair(2, Lorentzian);
  functionMapType["BackToBackExponential"] =
      std::make_pair(4, BackToBackExponential);
}

/// Returns a reference to the static functionMapType
/// @returns :: a const reference to the functionMapType
const FunctionMapType &getFunctionMapType() {
  std::lock_guard<std::recursive_mutex> lock(FUNCTION_MAP_MUTEX);

  static FunctionMapType functionMapType;

  if (functionMapType.empty())
    initFunctionLookup(functionMapType);
  return functionMapType;
}

/// Return a function code for a function if it needs setting values or None
/// otherwise.
Function whichFunction(const API::IFunction &function) {
  const FunctionMapType &functionMap = getFunctionMapType();
  auto index = functionMap.find(function.name());
  if (index != functionMap.end()) {
    if (!function.isExplicitlySet(index->second.first))
      return index->second.second;
  }
  return None;
}

//----------------------------------------------------------------------------------------------
/// Test if initial values need to be set before fitting.
/// @param function :: A fitting function to test.
bool needSettingInitialValues(const API::IFunction &function) {
  if (auto cf = dynamic_cast<const API::CompositeFunction *>(&function)) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      if (needSettingInitialValues(*cf->getFunction(i)))
        return true;
    }
  } else {
    return whichFunction(function) != None;
  }
  return false;
}

//----------------------------------------------------------------------------------------------
/// Extract values from domain and values objects to vectors.
/// @param domain :: A domain with fitting data arguments.
/// @param values :: A FunctionValues object with the fitting data.
/// @param x :: A vector to store the domain values
/// @param y :: A vector to store the fitting data values.
void extractValues(const API::FunctionDomain1D &domain,
                   const API::FunctionValues &values, std::vector<double> &x,
                   std::vector<double> &y) {

  size_t n = domain.size();
  double start = domain[0];
  double end = domain[n - 1];
  auto dBegin = domain.getPointerAt(0);
  auto startIter = std::lower_bound(dBegin, dBegin + n, start);
  auto istart = static_cast<size_t>(std::distance(dBegin, startIter));
  if (istart == n) {
    x.clear();
    y.clear();
    return;
  }
  auto endIter = std::lower_bound(startIter, dBegin + n, end);
  auto iend = static_cast<size_t>(std::distance(dBegin, endIter));
  if (iend <= istart) {
    x.clear();
    y.clear();
    return;
  }
  n = iend - istart;
  x.resize(n);
  y.resize(n);
  for (size_t i = istart; i < iend; ++i) {
    auto j = i - istart;
    x[j] = domain[i];
    y[j] = values.getFitData(i);
  }
}

// Functions to extract features from a curve

//----------------------------------------------------------------------------------------------
/// Get a measure of peak's width. It finds approximate zeros of the second
/// derivative of a function. Zeros nearest to the centre are interpreted as
/// the peak's inflection points. The next zero is interpreted as peak boundary.
/// @param centre :: Approximate peak centre.
/// @param der2   :: A second derivative of a peak. It is expected to have a
///   minimum at centre.
/// @param n :: An order of the zeros to look for. If n == 1 it's the inflection
///   points, if n == 2 it's the boundary.
/// @return :: A pair of zero points on either side of the centre.
std::pair<double, double>
getPeakLeftRightWidth(double centre, const SimpleChebfun &der2, size_t n = 1) {
  double left = centre;
  double right = centre;

  auto &xp = der2.xPoints();
  auto roots = der2.roughRoots();
  if (!roots.empty()) {
    auto iright = std::upper_bound(roots.begin(), roots.end(), centre);
    if (iright == roots.end()) {
      left = roots.back();
      return std::make_pair(left, right);
    }
    if (static_cast<size_t>(std::distance(roots.begin(), iright)) < n) {
      left = xp.front();
      return std::make_pair(left, right);
    }
    left = *(iright - n);
    if (static_cast<size_t>(std::distance(iright, roots.end())) < n) {
      right = xp.back();
    } else {
      right = *(iright + n - 1);
    }
  }
  return std::make_pair(left, right);
}

//----------------------------------------------------------------------------------------------
/// Get the peak's extent on either side of the centre.
/// @param centre :: An approximate peak centre.
/// @param der2 :: A second derivative of a function.
/// @return :: The left and right boundaries.
std::pair<double, double> getPeakLeftRightExtent(double centre,
                                                 const SimpleChebfun &der2) {
  return getPeakLeftRightWidth(centre, der2, 2);
}

//----------------------------------------------------------------------------------------------
/// Get displacements from peak centre where peak reaches half the maximum.
/// @param centre :: Peak centre.
/// @param height :: Peak height above background. height == fun(centre) -
/// background.
/// @param fun :: A function which is expected to be a peak on a background.
/// @return :: The left and right displacements from peak centre.
std::pair<double, double> getPeakHWHM(double centre, double height,
                                      const SimpleChebfun &fun) {
  auto roots = fun.roughRoots(height / 2);
  double left = fun.startX();
  double right = fun.endX();
  if (roots.empty()) {
    return std::make_pair(left - centre, right - centre);
  } else if (roots.size() == 1) {
    if (roots.front() < centre) {
      left = roots.front();
    } else {
      right = roots.front();
    }
  } else {
    auto iright = std::upper_bound(roots.begin(), roots.end(), centre);
    if (iright == roots.end()) {
      left = roots.back();
    } else if (iright == roots.begin()) {
      right = roots.front();
    } else {
      left = *(iright - 1);
      right = *iright;
    }
  }
  return std::make_pair(left - centre, right - centre);
}

//----------------------------------------------------------------------------------------------
/// Estimate a peak width from a second derivative of the data.
/// @param centre :: Approximate peak centre.
/// @param der2 :: An approximation to the second derivative.
double getPeakWidth(double centre, const SimpleChebfun &der2) {
  auto leftRight = getPeakLeftRightWidth(centre, der2);
  return fabs(leftRight.second - leftRight.first) / 2;
}

//----------------------------------------------------------------------------------------------
/// Improve an estimate of a peak centre.
/// @param centre :: A rough estimate of the centre.
/// @param der1 :: A first derivative of a function.
/// @return :: Improved estimate for the centre.
double getPeakCentre(double centre, const SimpleChebfun &der1) {
  auto roots = der1.roughRoots();
  if (!roots.empty()) {
    double minDiff = der1.width();
    size_t n = roots.size();
    size_t imin = n;
    for (size_t i = 0; i < n; ++i) {
      auto dx = fabs(roots[i] - centre);
      if (dx < minDiff) {
        minDiff = dx;
        imin = i;
      }
    }
    if (imin < n)
      return roots[imin];
  }
  return centre;
}

/// A linear function.
class LinearFunction {
public:
  LinearFunction(double a0, double a1) : m_a0(a0), m_a1(a1) {}
  double operator()(double x) const { return m_a0 + m_a1 * x; }

private:
  double m_a0;
  double m_a1;
};

//----------------------------------------------------------------------------------------------
/// Set initial values to a BackToBackExponential.
/// @param function :: A fitting BackToBackExponential function.
/// @param fun :: A smooth approximation of the fitting data.
/// @param der1 :: The first derivative of the fitting data.
/// @param der2 :: The second derivative of the fitting data.
void setBackToBackExponential(API::IFunction &function,
                              const SimpleChebfun &fun,
                              const SimpleChebfun &der1,
                              const SimpleChebfun &der2) {
  // Find the actual peak centre and gaussian component of the width
  auto centre = getPeakCentre(function.getParameter("X0"), der1);
  double sigma = getPeakWidth(centre, der2);
  if (sigma == 0.0)
    sigma = 1e-06;
  function.setParameter("S", sigma);

  g_log.debug() << "Estimating parameters of BackToBackExponential\n";
  g_log.debug() << "centre= " << centre << '\n';
  g_log.debug() << "sigma = " << sigma << '\n';

  // Estimate the background level
  auto xlr = getPeakLeftRightExtent(centre, der2);
  g_log.debug() << "extent: " << xlr.first - centre << ' '
                << xlr.second - centre << '\n';
  double yl = fun(xlr.first);
  double yr = fun(xlr.second);
  double slope = (yr - yl) / (xlr.second - xlr.first);
  double background = yl + slope * (centre - xlr.first);
  double height = fun(centre) - background;
  g_log.debug() << "height= " << height << '\n';
  g_log.debug() << "background= " << background << '\n';
  g_log.debug() << "slope= " << slope << '\n';

  // Remove the background as it affects A and B parameters.
  LinearFunction bg(-yl + slope * xlr.first, -slope);
  SimpleChebfun fun1(fun);
  fun1 += bg;
  // Find left and right "HWHM".
  auto hwhm = getPeakHWHM(centre, height, fun1);
  g_log.debug() << "HWHM: " << hwhm.first << ' ' << hwhm.second << '\n';

  // Find the extent of the default fitting function (with new S set)
  // to be able to make an approximation with a SimpleChebfun.
  function.setParameter("I", 1.0);
  double x0 = function.getParameter("X0");
  double leftX = 1.0 / function.getParameter("A");
  if (leftX < 3 * sigma) {
    leftX = 3 * sigma;
  }
  leftX = x0 - leftX;
  double rightX = 1.0 / function.getParameter("B");
  if (rightX < 3 * sigma) {
    rightX = 3 * sigma;
  }
  rightX = x0 + rightX;

  // Find corrections to the default A and B parameters.
  // A and B are responsible for differences in the widths.
  {
    SimpleChebfun b2b(fun.order(), function, leftX, rightX);
    auto b2b_d1 = b2b.derivative();
    auto centre1 = getPeakCentre(x0, b2b_d1);

    double height1 = b2b(centre1);
    auto hwhm1 = getPeakHWHM(centre1, height1, b2b);
    g_log.debug() << "new HWHM: " << hwhm1.first << ' ' << hwhm1.second << '\n';

    double denom = hwhm.first + sigma;
    double aCorr = denom > 0 ? (hwhm1.first + sigma) / denom : 100.0;
    if (aCorr < 0) {
      aCorr = 100.0;
      function.fix(2);
    }
    denom = hwhm.second - sigma;
    double bCorr = denom > 0 ? (hwhm1.second - sigma) / denom : 100.0;
    if (bCorr < 0) {
      bCorr = 100.0;
      function.fix(3);
    }
    g_log.debug() << "corrections: " << aCorr << ' ' << bCorr << '\n';
    double a = function.getParameter("A") * aCorr;
    double b = function.getParameter("B") * bCorr;
    function.setParameter("A", a);
    function.setParameter("B", b);
  }

  // After all shape parameters are set (S, A and B) shift X0
  // and scale I.
  {
    SimpleChebfun b2b(fun.order(), function, leftX, rightX);
    auto b2b_d1 = b2b.derivative();
    double centre1 = getPeakCentre(x0, b2b_d1);
    double height1 = b2b(centre1);
    x0 += centre - centre1;
    function.setParameter("X0", x0);
    function.setParameter("I", height / height1);
  }

  g_log.debug() << "Parameters:\n";
  g_log.debug() << "I  " << function.getParameter("I") << '\n';
  g_log.debug() << "X0 " << function.getParameter("X0") << '\n';
  g_log.debug() << "A  " << function.getParameter("A") << '\n';
  g_log.debug() << "B  " << function.getParameter("B") << '\n';
  g_log.debug() << "S  " << function.getParameter("S") << '\n';
}

//----------------------------------------------------------------------------------------------
/// Set initial values to a function if it needs to.
/// @param function :: A fitting function.
/// @param fun :: A smooth approximation of the fitting data.
/// @param der1 :: The first derivative of the fitting data.
/// @param der2 :: The second derivative of the fitting data.
void setValues(API::IFunction &function, const SimpleChebfun &fun,
               const SimpleChebfun &der1, const SimpleChebfun &der2) {
  if (auto cf = dynamic_cast<const API::CompositeFunction *>(&function)) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      setValues(*cf->getFunction(i), fun, der1, der2);
    }
    return;
  }

  switch (whichFunction(function)) {
  case Gaussian: {
    double width = getPeakWidth(function.getParameter("PeakCentre"), der2);
    function.setParameter("Sigma", width);
    break;
  }
  case Lorentzian: {
    double width = getPeakWidth(function.getParameter("PeakCentre"), der2);
    function.setParameter("FWHM", width);
    break;
  }
  case BackToBackExponential: {
    setBackToBackExponential(function, fun, der1, der2);
    break;
  }
  default:
    break;
  }
}

//----------------------------------------------------------------------------------------------
/// ParameterEstimator estimates parameter values of some fitting functions
///  from fitting data.
/// @param function :: A function to estimate parameters for.
/// @param domain :: A domain with fitting data arguments.
/// @param values :: A FunctionValues object with the fitting data.
void estimate(API::IFunction &function, const API::FunctionDomain1D &domain,
              const API::FunctionValues &values) {
  if (!needSettingInitialValues(function))
    return;
  std::vector<double> x;
  std::vector<double> y;
  extractValues(domain, values, x, y);
  if (x.empty())
    return;
  SimpleChebfun fun(x, y);
  auto der1 = fun.derivative();
  auto der2 = der1.derivative();
  setValues(function, fun, der1, der2);
}

} // namespace ParameterEstimator
} // namespace CurveFitting
} // namespace Mantid
