#include "MantidCurveFitting/ParameterEstimator.h"
#include "MantidCurveFitting/SimpleChebfun.h"

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include "MantidKernel/Logger.h"

namespace Mantid {
namespace CurveFitting {

/// The logger.
Kernel::Logger g_log("ParameterEstimator");

//----------------------------------------------------------------------------------------------
/// Constructor
/// @param function :: A function to estimate parameters for.
/// @param domain :: A domain with fitting data arguments.
/// @param values :: A FunctionValues object with the fitting data.
ParameterEstimator::ParameterEstimator(API::IFunction &function,
                                       const API::FunctionDomain1D &domain,
                                       const API::FunctionValues &values)
    : m_function(function), m_domain(domain), m_values(values) {}

//----------------------------------------------------------------------------------------------
/// Do the job.
void ParameterEstimator::estimate() {
  if (!needSettingInitialValues(m_function))
    return;
  std::vector<double> x;
  std::vector<double> y;
  extractValues(x, y);
  if (x.empty())
    return;
  SimpleChebfun fun(x, y);
  auto der1 = fun.derivative();
  auto der2 = der1.derivative();
  setValues(m_function, fun, der1, der2);
}

//----------------------------------------------------------------------------------------------
/// Extract values from m_domain and m_values objects to vectors.
/// @param x :: A vector to store the domain values
/// @param y :: A vector to store the fitting data values.
void ParameterEstimator::extractValues(std::vector<double> &x,
                                       std::vector<double> &y) const {

  size_t n = m_domain.size();
  double start = m_domain[0];
  double end = m_domain[n - 1];
  auto dBegin = m_domain.getPointerAt(0);
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
    x[j] = m_domain[i];
    y[j] = m_values.getFitData(i);
  }
}

namespace {
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
  auto &xp = der2.xPoints();
  auto icentre = std::lower_bound(xp.begin(), xp.end(), centre);
  if (icentre != xp.end()) {
    auto ic = static_cast<size_t>(std::distance(xp.begin(), icentre));
    const double d2max = der2(centre);
    auto PD2 = der2.yPoints();
    double sign = d2max / fabs(d2max);
    size_t rootCount = 0;
    double left = 0.0;
    for (auto i = ic; i > 0; --i) {
      if (sign * PD2[i] < 0.0) {
        ++rootCount;
        if (rootCount == n) {
          break;
        } else {
          sign *= -1;
        }
      }
      left = xp[i];
    }
    sign = d2max / fabs(d2max);
    rootCount = 0;
    double right = 0.0;
    for (auto i = ic; i < xp.size(); ++i) {
      if (sign * PD2[i] < 0.0) {
        ++rootCount;
        if (rootCount == n) {
          break;
        } else {
          sign *= -1;
        }
      }
      right = xp[i];
    }

    return std::make_pair(left, right);
  }
  return std::make_pair(centre, centre);
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
/// @param height :: Peak height above background. height == fun(centre) - background.
/// @param background :: Background value at peak centre.
/// @param fun :: A function which is expected to be a peak on a background.
/// @return :: The left and right displacements from peak centre.
std::pair<double, double> getPeakHWHM(double centre, double height,
                                      double background,
                                      const SimpleChebfun &fun) {
  auto roots = fun.roughRoots(background + height / 2);
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

} // namespace

//----------------------------------------------------------------------------------------------
/// Set initial values to a function if it needs to.
/// @param function :: A fitting function.
/// @param fun :: A smooth approximation of the fitting data.
/// @param der1 :: The first derivative of the fitting data.
/// @param der2 :: The second derivative of the fitting data.
void ParameterEstimator::setValues(API::IFunction &function,
                                   const SimpleChebfun &fun,
                                   const SimpleChebfun &der1,
                                   const SimpleChebfun &der2) const {
  if (auto cf = dynamic_cast<const API::CompositeFunction *>(&function)) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      setValues(*cf->getFunction(i), fun, der1, der2);
    }
  } else if (function.name() == "Gaussian" && !function.isExplicitlySet(2)) {
    double width = getPeakWidth(function.getParameter("PeakCentre"), der2);
    function.setParameter("Sigma", width);
  } else if (function.name() == "Lorentzian" && !function.isExplicitlySet(2)) {
    double width = getPeakWidth(function.getParameter("PeakCentre"), der2);
    function.setParameter("FWHM", width);
  } else if (function.name() == "BackToBackExponential" &&
             !function.isExplicitlySet(4)) {
    auto centre = getPeakCentre(function.getParameter("X0"), der1);
    std::cerr << "C=" << centre << std::endl;
    double sigma = getPeakWidth(centre, der2);
    std::cerr << "sigma=" << sigma << std::endl;
    function.setParameter("S", sigma);

    auto lr = getPeakLeftRightWidth(centre, der2);
    std::cerr << "lr: " << lr.first - centre << ' ' << lr.second - centre
              << std::endl;
    auto xlr = getPeakLeftRightExtent(centre, der2);
    std::cerr << "xlr: " << xlr.first - centre << ' ' << xlr.second - centre
              << std::endl;
    double yl = fun(xlr.first);
    double yr = fun(xlr.second);
    double background =
        yl + (yr - yl) / (xlr.second - xlr.first) * (centre - xlr.first);
    double height = fun(centre) - background;

    std::cerr << "height: " << height << ' ' << background << std::endl;
    auto hwhm = getPeakHWHM(centre, height, background, fun);
    std::cerr << "HWHM: " << hwhm.first << ' ' << hwhm.second << std::endl;

    function.setParameter("I", 1.0);
    double x0 = function.getParameter("X0");
    double leftX = 1.0 / function.getParameter("A");
    if (leftX < 3 * sigma)
      leftX = 3 * sigma;
    leftX = x0 - leftX;
    double rightX = 1.0 / function.getParameter("B");
    if (rightX < 3 * sigma)
      rightX = 3 * sigma;
    if (rightX > 10.0)
      rightX = 10.0;
    rightX = x0 + rightX;
    SimpleChebfun b2b(fun.order(), function, leftX, rightX);
    auto b2b_d1 = b2b.derivative();
    auto b2b_d2 = b2b_d1.derivative();
    auto centre1 = getPeakCentre(x0, b2b_d1);
    double dx = centre - centre1;
    std::cerr << "dx=" << dx << std::endl;
    x0 += dx;
    function.setParameter("X0", x0);

    {
      leftX += dx;
      rightX += dx;
      SimpleChebfun b2b(fun.order(), function, leftX, rightX);
      auto b2b_d1 = b2b.derivative();
      auto b2b_d2 = b2b_d1.derivative();
      auto centre1 = getPeakCentre(x0, b2b_d1);
      auto lr = getPeakLeftRightWidth(centre1, b2b_d2);
      std::cerr << "new lr: " << centre1 << ' ' << lr.first - centre1 << ' '
                << lr.second - centre1 << std::endl;
      auto xlr = getPeakLeftRightExtent(centre1, b2b_d2);
      std::cerr << "new xlr: " << centre1 << ' ' << xlr.first - centre1 << ' '
                << xlr.second - centre1 << std::endl;
      double height1 = b2b(centre1);
      auto hwhm1 = getPeakHWHM(centre1, height1, 0, b2b);
      std::cerr << "new HWHM: " << hwhm1.first << ' ' << hwhm1.second
                << std::endl;
      double aCorr = (hwhm1.first + sigma) / (hwhm.first + sigma);
      double bCorr = (hwhm1.second - sigma) / (hwhm.second - sigma);
      std::cerr << "corrections: " << aCorr << ' ' << bCorr << std::endl;
      double a = function.getParameter("A") * aCorr;
      double b = function.getParameter("B") * bCorr;
      function.setParameter("A", a);
      function.setParameter("B", b);
      SimpleChebfun b2b1(fun.order(), function, leftX, rightX);
      auto b2b1_d1 = b2b1.derivative();
      centre1 = getPeakCentre(x0, b2b1_d1);
      height1 = b2b1(centre1);
      x0 += centre - centre1;
      function.setParameter("X0", x0);
      function.setParameter("I", height / height1);
      std::cerr << "Parameters:" << std::endl;
      std::cerr << "I  " << function.getParameter("I") << std::endl;
      std::cerr << "X0 " << function.getParameter("X0") << std::endl;
      std::cerr << "A  " << function.getParameter("A") << std::endl;
      std::cerr << "B  " << function.getParameter("B") << std::endl;
      std::cerr << "S  " << function.getParameter("S") << std::endl;

      SimpleChebfun bbb(function, centre, rightX);
      auto &x = bbb.xPoints();
      std::cerr << "Size " << x.size() << ' ' << bbb.isGood() << std::endl;
      for (size_t i = 1; i < x.size(); ++i) {
        double s = x[i - 1];
        double e = x[i];
        SimpleChebfun cheb(bbb, s, e);
        std::cerr << "interval " << s << ' ' << e << std::endl;
        std::cerr << cheb.size() << ' ' << cheb.isGood() << std::endl;
      }
    }
  }
}

//----------------------------------------------------------------------------------------------
/// Test if initial values need to be set before fitting.
/// @param function :: A fitting function to test.
bool ParameterEstimator::needSettingInitialValues(
    const API::IFunction &function) {
  if (auto cf = dynamic_cast<const API::CompositeFunction *>(&function)) {
    for (size_t i = 0; i < cf->nFunctions(); ++i) {
      if (needSettingInitialValues(*cf->getFunction(i)))
        return true;
    }
  } else if (function.name() == "Gaussian" && !function.isExplicitlySet(2))
    return true;
  else if (function.name() == "Lorentzian" && !function.isExplicitlySet(2))
    return true;
  else if (function.name() == "BackToBackExponential" &&
           !function.isExplicitlySet(4))
    return true;
  return false;
}

} // namespace CurveFitting
} // namespace Mantid
