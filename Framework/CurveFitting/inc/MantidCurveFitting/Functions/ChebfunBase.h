// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_CHEBFUNBASE_H
#define MANTID_CURVEFITTING_CHEBFUNBASE_H

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/GSLMatrix.h"

#include <boost/shared_ptr.hpp>
#include <functional>
#include <vector>

namespace Mantid {

namespace API {
class IFunction;
}

namespace CurveFitting {
namespace Functions {

/// Type of the approximated function
using ChebfunFunctionType = std::function<double(double)>;

/**

The ChebfunBase class provides a base for function approximation
with Chebyshev polynomials.

A smooth function on a finite interval [a,b] can be approximated
by a Chebyshev expansion of order n. Finding an approximation is
very easy: the function needs to be evaluated at n+1 specific x-
points. These n+1 values can be used to interpolate the function
at any x-point in interval [a,b]. This is done by calling the fit(...)
method.

Different functions require different polynomial orders to reach
the same accuracy of approximation. Static method bestFit(...) tries
to find the smallest value of n that provides the required accuracy.
If it fails to find an n smaller than some maximum number it returns
an empty shared pointer.

Knowing the vector of the function values (P) at the n+1 base x-points and the
related vector of the Chebyshev expansion coefficients (A) (claculated
by calcA(...) method) allows one to perform various manipulations on
the approximation:
  - algebraic operations: +,-,*,/
  - applying a function
  - root finding
  - differentiation
  - integration
  - convolution
  - solving of (integro-)differential equations
  - etc

This calss doesn't represent a function approximation itself but keeps
proerties that can be shared by multiple approximations.

This class is based on the ideas from the Chebfun matlab package
(http://www.chebfun.org/).

*/
class MANTID_CURVEFITTING_DLL ChebfunBase {
public:
  ChebfunBase(size_t n, double start, double end, double tolerance = 0.0);
  /// Copy constructor
  ChebfunBase(const ChebfunBase &) = default;
  /// Move constructor
  ChebfunBase(ChebfunBase &&) noexcept = default;
  /// Get the polynomial order of this base.
  size_t order() const { return m_n; }
  /// Get the size of the base which is the number of x-points.
  size_t size() const { return m_x.size(); }
  /// Start of the interval
  double startX() const { return m_x.front(); }
  /// End of the interval
  double endX() const { return m_x.back(); }
  /// Get the width of the interval
  double width() const { return endX() - startX(); }
  /// Get a reference to the x-points
  const std::vector<double> &xPoints() const { return m_x; }
  /// Get a reference to the integration weights
  const std::vector<double> &integrationWeights() const;
  /// Calculate an integral
  double integrate(const std::vector<double> &p) const;
  /// Calculate expansion coefficients
  std::vector<double> calcA(const std::vector<double> &p) const;
  /// Calculate function values
  std::vector<double> calcP(const std::vector<double> &a) const;
  /// Calculate function values at chebfun x-points
  std::vector<double> fit(ChebfunFunctionType f) const;
  /// Calculate function values at chebfun x-points
  std::vector<double> fit(const API::IFunction &f) const;

  /// Evaluate a function
  double eval(double x, const std::vector<double> &p) const;
  /// Evaluate a function
  void evalVector(const std::vector<double> &x, const std::vector<double> &p,
                  std::vector<double> &res) const;
  /// Evaluate a function
  std::vector<double> evalVector(const std::vector<double> &x,
                                 const std::vector<double> &p) const;
  /// Calculate the derivative
  void derivative(const std::vector<double> &a,
                  std::vector<double> &aout) const;
  /// Calculate the integral
  boost::shared_ptr<ChebfunBase> integral(const std::vector<double> &a,
                                          std::vector<double> &aout) const;
  /// Find all roots of a function on this interval
  std::vector<double> roots(const std::vector<double> &a) const;

  /// Fit a function until full convergence
  static boost::shared_ptr<ChebfunBase>
  bestFit(double start, double end, ChebfunFunctionType, std::vector<double> &p,
          std::vector<double> &a, double maxA = 0.0, double tolerance = 0.0,
          size_t maxSize = 0);
  /// Fit a function until full convergence
  static boost::shared_ptr<ChebfunBase>
  bestFit(double start, double end, const API::IFunction &,
          std::vector<double> &p, std::vector<double> &a, double maxA = 0.0,
          double tolerance = 0.0, size_t maxSize = 0);
  /// Tolerance for comparing doubles
  double tolerance() { return m_tolerance; }

  /// Find best fit with highest possible tolerance (to be used with noisy
  /// data).
  template <class FunctionType>
  static boost::shared_ptr<ChebfunBase>
  bestFitAnyTolerance(double start, double end, FunctionType f,
                      std::vector<double> &p, std::vector<double> &a,
                      double maxA = 0.0, double tolerance = 0.0,
                      size_t maxSize = 0);

  /// Create a vector of x values linearly spaced on the approximation interval
  std::vector<double> linspace(size_t n) const;
  std::vector<double> smooth(const std::vector<double> &xvalues,
                             const std::vector<double> &yvalues) const;

private:
  /// Private assingment operator to stress the immutability of ChebfunBase.
  ChebfunBase &operator=(const ChebfunBase &other) = delete;
  ChebfunBase &operator=(ChebfunBase &&other) = delete;
  /// Calculate the x-values based on the (start,end) interval.
  void calcX();
  /// Calculate the integration weights
  void calcIntegrationWeights() const;

  /// Calculate function values at odd-valued indices of the base x-points
  std::vector<double> fitOdd(ChebfunFunctionType f,
                             std::vector<double> &p) const;
  /// Calculate function values at odd-valued indices of the base x-points
  std::vector<double> fitOdd(const API::IFunction &f,
                             std::vector<double> &pEven) const;
  /// Test an array of Chebyshev coefficients for convergence
  static bool hasConverged(const std::vector<double> &a, double maxA,
                           double tolerance, size_t shift = 0);
  /// Templated implementation of bestFit method
  template <class FunctionType>
  static boost::shared_ptr<ChebfunBase>
  bestFitTempl(double start, double end, FunctionType f, std::vector<double> &p,
               std::vector<double> &a, double maxA, double tolerance,
               size_t maxSize);

  /// Actual tolerance in comparing doubles
  const double m_tolerance;
  /// Polynomial order.
  size_t m_n;
  /// Start of the interval
  double m_start;
  /// End of the interval
  double m_end;
  /// The x-points
  std::vector<double> m_x;
  /// The barycentric weights.
  std::vector<double> m_bw;
  /// The integration weights
  mutable std::vector<double> m_integrationWeights;
  /// Maximum tolerance in comparing doubles
  static const double g_tolerance;
  /// Maximum number of (x) points in a base.
  static const size_t g_maxNumberPoints;
};

using ChebfunBase_sptr = boost::shared_ptr<ChebfunBase>;

/// Find best fit with highest possible tolerance (to be used with noisy data).
template <class FunctionType>
boost::shared_ptr<ChebfunBase> ChebfunBase::bestFitAnyTolerance(
    double start, double end, FunctionType f, std::vector<double> &p,
    std::vector<double> &a, double maxA, double tolerance, size_t maxSize) {
  if (tolerance == 0.0)
    tolerance = g_tolerance;

  double tol = tolerance;
  while (tol < 0.1) {
    auto base = bestFit(start, end, f, p, a, maxA, tol, maxSize);
    if (base) {
      return base;
    }
    tol *= 100.0;
  }
  return ChebfunBase_sptr();
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid

#endif // MANTID_CURVEFITTING_CHEBFUNBASE_H
