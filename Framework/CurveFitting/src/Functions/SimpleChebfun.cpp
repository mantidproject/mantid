// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/SimpleChebfun.h"
#include "MantidAPI/IFunction.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

//----------------------------------------------------------------------------------------------
/// Constructs a SimpleChebfun that approximates a function with a polynomial of
/// a given order
/// in an interval of x-values.
/// @param n :: Polynomial order == number of points - 1.
/// @param fun :: A function to approximate.
/// @param start :: The start (lower bound) of an interval on the x-axis.
/// @param end :: The end (upper bound) of an interval on the x-axis.
SimpleChebfun::SimpleChebfun(size_t n, ChebfunFunctionType fun, double start,
                             double end)
    : m_badFit(false) {
  m_base = boost::make_shared<ChebfunBase>(n, start, end);
  m_P = m_base->fit(fun);
}

SimpleChebfun::SimpleChebfun(size_t n, const API::IFunction &fun, double start,
                             double end)
    : m_badFit(false) {
  m_base = boost::make_shared<ChebfunBase>(n, start, end);
  m_P = m_base->fit(fun);
}

/// Constructs a SimpleChebfun that approximates a function to a given accuracy
/// in an interval of x-values.
/// The approximation may fail (too large interval, function discontinuous,
/// etc).
/// In this case a default sized polynomial is created and isGood() will be
/// returning false.
/// @param fun :: A function to approximate.
/// @param start :: The start (lower bound) of an interval on the x-axis.
/// @param end :: The end (upper bound) of an interval on the x-axis.
/// @param accuracy :: The accuracy of the approximation.
/// @param badSize :: If automatic approxiamtion fails the base will have this
/// size.
SimpleChebfun::SimpleChebfun(ChebfunFunctionType fun, double start, double end,
                             double accuracy, size_t badSize)
    : m_badFit(false) {
  m_base = ChebfunBase::bestFitAnyTolerance<ChebfunFunctionType>(
      start, end, fun, m_P, m_A, accuracy);
  if (!m_base) {
    m_base = boost::make_shared<ChebfunBase>(badSize - 1, start, end, accuracy);
    m_P = m_base->fit(fun);
    m_badFit = true;
  }
}

SimpleChebfun::SimpleChebfun(const API::IFunction &fun, double start,
                             double end, double accuracy, size_t badSize)
    : m_badFit(false) {
  m_base = ChebfunBase::bestFitAnyTolerance<const API::IFunction &>(
      start, end, fun, m_P, m_A, accuracy);
  if (!m_base) {
    m_base = boost::make_shared<ChebfunBase>(badSize - 1, start, end, accuracy);
    m_P = m_base->fit(fun);
    m_badFit = true;
  }
}

/// Construct a SimpleChebfun by smoothing data in vectors with x and y data.
/// @param x :: A vector of x values.
/// @param y :: A vector of y values. Must have same size as x.
SimpleChebfun::SimpleChebfun(const std::vector<double> &x,
                             const std::vector<double> &y)
    : m_badFit(false) {
  m_base = boost::make_shared<ChebfunBase>(x.size() - 1, x.front(), x.back());
  m_P = m_base->smooth(x, y);
}

/// Construct an empty SimpleChebfun with shared base.
SimpleChebfun::SimpleChebfun(ChebfunBase_sptr base) : m_badFit(false) {
  assert(base);
  m_base = base;
  m_P.resize(base->size());
}

/// Get a reference to the Chebyshev expansion coefficients
const std::vector<double> &SimpleChebfun::coeffs() const {
  if (m_A.empty()) {
    m_A = m_base->calcA(m_P);
  }
  return m_A;
}

/// Evaluate the function.
/// @param x :: Point where the function is evaluated.
double SimpleChebfun::operator()(double x) const {
  return m_base->eval(x, m_P);
}

/// Evaluate the function for each value in a vector.
/// @param x :: Points where the function is evaluated.
std::vector<double> SimpleChebfun::
operator()(const std::vector<double> &x) const {
  return m_base->evalVector(x, m_P);
}

/// Create a vector of x values linearly spaced on the approximation interval.
/// @param n :: Number of points in the vector.
std::vector<double> SimpleChebfun::linspace(size_t n) const {
  return m_base->linspace(n);
}

/// Get the accuracy of the approximation
double SimpleChebfun::accuracy() const { return m_base->tolerance(); }

/// Create a derivative of this function.
SimpleChebfun SimpleChebfun::derivative() const {
  SimpleChebfun cheb(m_base);
  if (m_A.empty()) {
    m_A = m_base->calcA(m_P);
  }
  m_base->derivative(m_A, cheb.m_A);
  cheb.m_P = m_base->calcP(cheb.m_A);
  return cheb;
}

/// Create an integral of this function.
SimpleChebfun SimpleChebfun::integral() const {
  if (m_A.empty()) {
    m_A = m_base->calcA(m_P);
  }
  std::vector<double> A;
  auto base = m_base->integral(m_A, A);
  SimpleChebfun cheb(base);
  cheb.m_P = base->calcP(A);
  cheb.m_A = A;
  return cheb;
}

/// Get rough estimates of the roots.
/// @param level :: An optional right-hand-side of equation (*this)(x) == level.
std::vector<double> SimpleChebfun::roughRoots(double level) const {
  std::vector<double> rs;
  if (m_P.empty())
    return rs;
  auto &x = m_base->xPoints();
  auto y1 = m_P.front() - level;
  for (size_t i = 1; i < m_P.size(); ++i) {
    auto y = m_P[i] - level;
    if (y == 0.0) {
      rs.push_back(x[i]);
    } else if (y1 * y < 0.0) {
      rs.push_back((-x[i - 1] * y + x[i] * y1) / (y1 - y));
    }
    y1 = y;
  }
  return rs;
}

/// Integrate the function on its interval
double SimpleChebfun::integrate() const { return m_base->integrate(m_P); }

/// Add a C++ function to the function
/// @param fun :: A function to add.
SimpleChebfun &SimpleChebfun::operator+=(ChebfunFunctionType fun) {
  auto &x = xPoints();
  for (size_t i = 0; i < x.size(); ++i) {
    m_P[i] += fun(x[i]);
  }
  m_A.clear();
  return *this;
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
