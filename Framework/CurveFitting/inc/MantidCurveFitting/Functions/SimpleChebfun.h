// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include "MantidCurveFitting/Functions/ChebfunBase.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/** SimpleChebfun : approximates smooth 1d functions and
  provides methods to manipulate them.

  Main functionality is implemented in ChebfunBase class.
*/
class MANTID_CURVEFITTING_DLL SimpleChebfun {
public:
  /// Constructor.
  SimpleChebfun(size_t n, ChebfunFunctionType fun, double start, double end);
  /// Constructor.
  SimpleChebfun(size_t n, const API::IFunction &fun, double start, double end);
  /// Constructor.
  SimpleChebfun(const ChebfunFunctionType &fun, double start, double end, double accuracy = 0.0, size_t badSize = 10);
  /// Constructor.
  SimpleChebfun(const API::IFunction &fun, double start, double end, double accuracy = 0.0, size_t badSize = 10);
  /// Constructor.
  SimpleChebfun(const std::vector<double> &x, const std::vector<double> &y);
  /// Number of points in the approximation.
  size_t size() const { return m_base->size(); }
  /// Order of the approximating polynomial.
  size_t order() const { return m_base->order(); }
  /// Check if approximation is good.
  bool isGood() const { return !m_badFit; }
  /// Start of the interval
  double startX() const { return m_base->startX(); }
  /// End of the interval
  double endX() const { return m_base->endX(); }
  /// Get the width of the interval
  double width() const { return m_base->width(); }
  /// Get a reference to the x-points
  const std::vector<double> &xPoints() const { return m_base->xPoints(); }
  /// Get a reference to the y-points
  const std::vector<double> &yPoints() const { return m_P; }
  /// Get a reference to the Chebyshev expansion coefficients
  const std::vector<double> &coeffs() const;
  /// Evaluate the function.
  double operator()(double x) const;
  /// Evaluate the function.
  std::vector<double> operator()(const std::vector<double> &x) const;
  /// Create a vector of x values linearly spaced on the approximation interval
  std::vector<double> linspace(size_t n) const;
  /// Get the accuracy of the approximation
  double accuracy() const;
  /// Create a derivative of this function.
  SimpleChebfun derivative() const;
  /// Create an integral of this function.
  SimpleChebfun integral() const;
  /// Get rough estimates of the roots
  std::vector<double> roughRoots(double level = 0.0) const;
  /// Integrate the function on its interval
  double integrate() const;
  /// Add a C++ function to the function
  SimpleChebfun &operator+=(const ChebfunFunctionType &fun);

private:
  /// Constructor
  SimpleChebfun(const ChebfunBase_sptr &base);
  /// Underlying base that does actual job.
  ChebfunBase_sptr m_base;
  /// Function values at the chebfun x-points.
  std::vector<double> m_P;
  /// Chebyshev expansion coefficients.
  mutable std::vector<double> m_A;

  /// Set in a case of a bad fit
  bool m_badFit;
};

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
