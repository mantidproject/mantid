#ifndef MANTID_CURVEFITTING_CHEBFUNBASE_H
#define MANTID_CURVEFITTING_CHEBFUNBASE_H

#include "DllConfig.h"
#include "GSLMatrix.h"

#include <boost/shared_ptr.hpp>
#include <vector>
#include <functional>

namespace Mantid {

namespace API {
class IFunction;
}

namespace CurveFitting {

/// Type of the approximated function
typedef std::function<double(double)> ChebfunFunctionType;

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

Copyright &copy; 2007-8 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>

*/
class MANTID_CURVEFITTING_DLL ChebfunBase {
public:
  ChebfunBase(size_t n, double start, double end, double tolerance = 0.0);
  /// Copy constructor
  ChebfunBase(const ChebfunBase &other);
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

  /// Create a vector of x values linearly spaced on the approximation interval
  std::vector<double> linspace(size_t n) const;

private:
  /// Private assingment operator to stress the immutability of ChebfunBase.
  ChebfunBase &operator=(const ChebfunBase &other);
  /// Calculate the x-values based on the (start,end) interval.
  void calcX();
  /// Calculate the integration weights
  void calcIntegrationWeights() const;

  /// Calculate function values at odd-valued indices of the base x-points
  std::vector<double> fitOdd(ChebfunFunctionType f,
                             std::vector<double> &p) const;
  /// Calculate function values at odd-valued indices of the base x-points
  std::vector<double> fitOdd(const API::IFunction &f,
                             std::vector<double> &p) const;
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

typedef boost::shared_ptr<ChebfunBase> ChebfunBase_sptr;

} // CurveFitting
} // Mantid

#endif // MANTID_CURVEFITTING_CHEBFUNBASE_H
