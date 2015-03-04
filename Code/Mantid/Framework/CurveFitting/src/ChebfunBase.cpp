#include "MantidCurveFitting/ChebfunBase.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidCurveFitting/HalfComplex.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_eigen.h>

#include <cmath>
#include <algorithm>
#include <numeric>
#include <functional>
#include <assert.h>
#include <sstream>

namespace Mantid {
namespace CurveFitting {

// Set the comparison tolerance.
const double ChebfunBase::g_tolerance = 1e-15;
// Set the maximum number of points.
const size_t ChebfunBase::g_maxNumberPoints = 1026;

/**
 * Constructor.
 * @param n :: Polynomial order == number of points - 1.
 * @param start :: The start (lower bound) of an interval on the x-axis.
 * @param end :: The end (upper bound) of an interval on the x-axis.
 * @param tolerance :: Tolerance in comparing the expansion coefficients.
 *   Setting this tolerance is a way to specify the accuracy of the
 * approximation.
 */
ChebfunBase::ChebfunBase(size_t n, double start, double end, double tolerance)
    : m_tolerance(std::max(tolerance, g_tolerance)), m_n(n), m_start(start),
      m_end(end) {
  if (n == 0) {
    throw std::invalid_argument("Chebfun order must be greater than 0.");
  }

  m_x.resize(n + 1);
  m_bw.resize(n + 1, 1.0);
  for (size_t i = 1; i <= n; i += 2) {
    m_bw[i - 1] = 1.0;
    m_bw[i] = -1.0;
  }
  m_bw.front() /= 2.0;
  m_bw.back() /= 2.0;
  calcX();
}

/**
 * Copy constructor
 * @param other :: A base to copy from.
 */
ChebfunBase::ChebfunBase(const ChebfunBase &other)
    : m_tolerance(other.m_tolerance), m_n(other.m_n), m_start(other.m_start),
      m_end(other.m_end), m_x(other.m_x), m_bw(other.m_bw),
      m_integrationWeights(other.m_integrationWeights) {}

/**
 * Return the integration weights that can be used in function manipulations
 * involving integration.
 */
const std::vector<double> &ChebfunBase::integrationWeights() const {
  if (m_integrationWeights.size() != m_x.size()) {
    calcIntegrationWeights();
  }
  return m_integrationWeights;
}

/**
 * Calculate an integral of a function given its values at the base x-points.
 * @param p :: Function values at the x-points.
 * @return :: The integral.
 */
double ChebfunBase::integrate(const std::vector<double> &p) const {
  if (p.size() != m_x.size()) {
    throw std::invalid_argument(
        "Function values have a wrong size in integration.");
  }
  if (m_integrationWeights.empty()) {
    calcIntegrationWeights();
  }
  std::vector<double> tmp(p.size());
  std::transform(p.begin(), p.end(), m_integrationWeights.begin(), tmp.begin(),
                 std::multiplies<double>());
  // NB. for some reason the commented out expression gives more accurate result
  // (when weights
  // are not multiplied by the same factor) than the uncommented one. But moving
  // the factor to the
  // weights makes formulas involving weights simpler
  // return std::accumulate(tmp.begin(),tmp.end(),0.0) * (m_end - m_start) / 2;
  return std::accumulate(tmp.begin(), tmp.end(), 0.0);
}

/**
 * Calculate the x-values based on the (start,end) interval.
 */
void ChebfunBase::calcX() {
  if (m_n == 0) {
    throw std::logic_error(
        "Cannot calculate x points of ChebfunBase: base is empty.");
  }
  if (m_x.size() != m_n + 1) {
    throw std::logic_error("X array has a wrong size.");
  }
  const double x0 = (m_start + m_end) / 2;
  const double b = (m_end - m_start) / 2;
  const double pin = M_PI / m_n;
  for (size_t i = 0; i <= m_n; ++i) {
    size_t j = m_n - i;
    m_x[i] = x0 + b * cos(j * pin);
  }
}

/**
 * Calculate the integration weights.
 */
void ChebfunBase::calcIntegrationWeights() const {
  size_t n = m_n + 1;
  m_integrationWeights.resize(n);
  // build an intermediate vector (these are different kind of weights)
  std::vector<double> w(n);
  for (size_t i = 0; i < n; ++i) {
    if (i % 2 == 0) {
      w[i] = 2.0 / (1.0 - static_cast<double>(i * i));
    }
  }
  w[0] /= 2;
  w[m_n] /= 2;
  const double factor = (m_end - m_start) / 2;
  // calculate the weights
  for (size_t i = 0; i < n; ++i) {
    double b = 0.0;
    for (size_t j = 0; j < n; ++j) {
      b += w[j] * cos(M_PI * i * j / m_n);
    }
    b /= m_n;
    if (i > 0 && i != m_n) {
      b *= 2;
    }
    m_integrationWeights[i] = b * factor;
  }
}

/**
 * Test if an array of Chebyshev expansion coefficients converged to the
 * specified tolerance.
 * @param a :: A vector of coefficients.
 * @param maxA :: A maximum value of the coefficients to compare against.
 * @param tolerance :: Convergence tolerance.
 * @return :: True if converged or false otherwise.
 */
bool ChebfunBase::hasConverged(const std::vector<double> &a, double maxA,
                               double tolerance, size_t shift) {
  if (a.empty())
    return true;
  if (maxA == 0.0) {
    maxA = fabs(*std::max_element(a.begin(), a.end(),
                                  [](double a, double b)
                                      -> bool { return fabs(a) < fabs(b); }));
  }
  if (maxA < tolerance || a.size() < 3) {
    return true;
  }

  if (shift > a.size() - 2)
    return true;
  for (auto i = a.rbegin() + shift; i != a.rend() - 1; ++i) {
    if (*i == 0.0)
      continue;
    if ((fabs(*i) + fabs(*(i + 1))) / maxA / 2 < tolerance)
      return true;
    else
      return false;
  }
  return false;
}

/**
 * Evaluate a function
 * @param x :: Point of evaluation.
 * @param p :: The function y-points
 * @return Value of the function.
 */
double ChebfunBase::eval(double x, const std::vector<double> &p) const {
  if (p.size() != m_x.size()) {
    throw std::invalid_argument("Wrong array size in ChebdunBase::eval.");
  }
  if (x < m_start || x > m_end)
    return 0.0;
  auto ix = std::find(m_x.begin(), m_x.end(), x);
  if (ix != m_x.end()) {
    auto i = std::distance(m_x.begin(), ix);
    return p[i];
  }
  double weight = 0.0;
  double res = 0.0;
  auto xend = m_x.end();
  auto ip = p.begin();
  auto iw = m_bw.begin();
  for (ix = m_x.begin(); ix != xend; ++ix, ++ip, ++iw) {
    double w = *iw / (x - *ix);
    weight += w;
    res += w * (*ip);
  }
  return res / weight;
}

/**
 * Evaluate function on a vector of x-values.
 * @param x :: A vector of x-values.
 * @param p :: The y-points of a function.
 * @param res :: Output result. res.size() == x.size()
 */
void ChebfunBase::evalVector(const std::vector<double> &x,
                             const std::vector<double> &p,
                             std::vector<double> &res) const {
  if (x.empty()) {
    throw std::invalid_argument("Vector of x-values cannot be empty.");
  }

  res.resize(x.size(), 0.0);
  auto ix = std::lower_bound(m_x.begin(), m_x.end(), x.front());
  if (ix == m_x.end()) {
    return;
  }

  auto mXBegin = m_x.begin();
  auto mXEnd = m_x.end();
  auto pBegin = p.begin();
  auto bwBegin = m_bw.begin();

  size_t i = 0;
  for (; i < x.size(); ++i) {
    if (x[i] >= m_start)
      break;
  }

  for (; i < x.size(); ++i) {
    double xi = x[i];
    while (ix != mXEnd && xi > *ix)
      ++ix;
    if (ix == mXEnd)
      break;

    if (xi == *ix) {
      auto j = std::distance(m_x.begin(), ix);
      res[i] = p[j];
    } else {
      double weight = 0.0;
      double value = 0.0;
      auto kp = pBegin;
      auto kw = bwBegin;
      for (auto kx = mXBegin; kx != mXEnd; ++kx, ++kp, ++kw) {
        double w = *kw / (xi - *kx);
        weight += w;
        value += w * (*kp);
      }
      res[i] = value / weight;
    }
  }
}

/**
 * Evaluate function on a vector of x-values.
 * @param x :: A vector of x-values.
 * @param p :: The y-points of a function.
 * @return :: Output result. res.size() == x.size()
 */
std::vector<double>
ChebfunBase::evalVector(const std::vector<double> &x,
                        const std::vector<double> &p) const {
  std::vector<double> res;
  evalVector(x, p, res);
  return res;
}

/**
 * Calculate the first derivative of a function.
 * @param a :: Chebyshev coefficients of the diffientiated function.
 * @param aout :: Output coeffs of the derivative.
 */
void ChebfunBase::derivative(const std::vector<double> &a,
                             std::vector<double> &aout) const {
  using namespace std::placeholders;
  if (a.size() != m_x.size()) {
    throw std::invalid_argument(
        "Cannot calculate derivative: coeffs vector has wrong size.");
  }
  if (m_n == 0) {
    aout.resize(2, 0.0);
    aout[0] = 2.0 * a[1];
    return;
  }
  aout.resize(m_n + 1);
  aout.back() = 0.0;
  aout[m_n - 1] = 2.0 * m_n * a.back();
  for (size_t k = m_n - 1; k > 1; --k) {
    aout[k - 1] = aout[k + 1] + 2.0 * k * a[k];
  }
  if (m_n > 2) {
    aout.front() = aout[2] / 2 + a[1];
  }
  double d = (m_end - m_start) / 2;
  std::transform(aout.begin(), aout.end(), aout.begin(),
                 std::bind2nd(std::divides<double>(), d));
}

/**
 * Calculate the first integral of a function.
 * @param a :: Chebyshev coefficients of the integrated function.
 * @param aout :: Output coeffs of the integral.
 * @return :: A base for the integral.
 */
ChebfunBase_sptr ChebfunBase::integral(const std::vector<double> &a,
                                       std::vector<double> &aout) const {
  using namespace std::placeholders;
  if (a.size() != m_x.size()) {
    throw std::invalid_argument(
        "Cannot calculate integral: coeffs vector has wrong size.");
  }
  aout.resize(m_n + 2);
  aout.front() = 0.0;
  for (size_t k = 1; k < m_n; ++k) {
    aout[k] = (a[k - 1] - a[k + 1]) / (2 * k);
  }
  aout[m_n] = a[m_n - 1] / (2 * m_n);
  aout[m_n + 1] = a[m_n] / (2 * (m_n + 1));
  double d = (m_end - m_start) / 2;
  std::transform(aout.begin(), aout.end(), aout.begin(),
                 std::bind(std::multiplies<double>(), _1, d));
  return ChebfunBase_sptr(new ChebfunBase(m_n + 1, m_start, m_end));
}

/**
 * Fit a function until full convergence. Increases size of the base until full
 * conversion
 * or a size limit is reached. If size limit is reached returns an empty
 * pointer. In this
 * case the calling method can divide the interval and fit each part separately.
 * @param start :: Lower limit of the x-interval.
 * @param end :: Upper limit of the x-interval.
 * @param f :: Function to fit.
 * @param p :: Function values at the found x-points.
 * @param maxA :: A maximum value of the coefficients to compare against.
 * @return :: A ChebfunBase of the best fit if succeeded or empty pointer if
 * failed.
 */
template <class FunctionType>
ChebfunBase_sptr
ChebfunBase::bestFitTempl(double start, double end, FunctionType f,
                          std::vector<double> &p, std::vector<double> &a,
                          double maxA, double tolerance, size_t maxSize) {

  std::vector<double> p1, p2;
  const size_t n0 = 8;
  bool calcMaxA = maxA == 0.0;
  if (tolerance == 0.0)
    tolerance = g_tolerance;
  // number of non-zero a-coefficients for checking if the function is a
  // polynomial
  size_t countNonZero = n0 / 2;
  if (maxSize == 0)
    maxSize = g_maxNumberPoints;
  for (size_t n = n0; n < maxSize; n *= 2) {
    // value of n must be even! or everything breaks!
    ChebfunBase base(n, start, end);
    if (p2.empty()) {
      p2 = base.fit(f);
    } else {
      p2 = base.fitOdd(f, p1);
    }
    a = base.calcA(p2);
    if (calcMaxA) {
      maxA =
          fabs(*std::max_element(a.begin(), a.end(), [](double a1, double a2) {
            return fabs(a1) < fabs(a2);
          }));
    }
    if (ChebfunBase::hasConverged(a, maxA, tolerance)) {
      // cut off the trailing a-values that are below the tolerance

      size_t m = n + 1;
      size_t dm = 0;
      while (dm < m - 2 && ChebfunBase::hasConverged(a, maxA, tolerance, dm)) {
        ++dm;
      }
      // restore a to the converged state
      if (dm > 0)
        --dm;
      m -= dm;

      // remove possible negligible trailing coefficients left over
      while (m > 2 && fabs(a[m - 1]) / maxA < tolerance) {
        --m;
      }

      if (m != n + 1) {
        auto newBase = ChebfunBase_sptr(new ChebfunBase(m - 1, start, end));
        a.resize(m);
        p = newBase->calcP(a);
        return newBase;
      } else {
        p.assign(p2.begin(), p2.end());
        return ChebfunBase_sptr(new ChebfunBase(base));
      }
    }
    size_t nNonZero = a.size();
    for (auto i = a.rbegin(); i != a.rend(); ++i) {
      if (*i == 0.0) {
        nNonZero -= 1;
      } else {
        break;
      }
    }
    if (nNonZero == countNonZero) {
      // it is a polynomial
      if (countNonZero < 2)
        countNonZero = 2;
      auto newBase =
          ChebfunBase_sptr(new ChebfunBase(countNonZero - 1, start, end));
      a.resize(countNonZero);
      p = newBase->calcP(a);
      return newBase;
    } else {
      countNonZero = nNonZero;
    }
    std::swap(p1, p2);
  }
  p.clear();
  a.clear();
  a.push_back(maxA);
  return ChebfunBase_sptr();
}

/// Template specialization for a generic function type.
ChebfunBase_sptr ChebfunBase::bestFit(double start, double end,
                                      ChebfunFunctionType f,
                                      std::vector<double> &p,
                                      std::vector<double> &a, double maxA,
                                      double tolerance, size_t maxSize) {
  return bestFitTempl(start, end, f, p, a, maxA, tolerance, maxSize);
}

/// Template specialization for IFunction
ChebfunBase_sptr ChebfunBase::bestFit(double start, double end,
                                      const API::IFunction &f,
                                      std::vector<double> &p,
                                      std::vector<double> &a, double maxA,
                                      double tolerance, size_t maxSize) {
  return bestFitTempl<const API::IFunction &>(start, end, f, p, a, maxA,
                                              tolerance, maxSize);
}

/**
 * Return a vector of linearly spaced values in the domain interval m_start <= x
 * <= m_end
 * @param n :: Number of points in the output vector.
 */
std::vector<double> ChebfunBase::linspace(size_t n) const {
  std::vector<double> space(n);
  double x = m_start;
  const double dx = width() / (n - 1);
  for (auto s = space.begin(); s != space.end(); ++s) {
    *s = x;
    x += dx;
  }
  space.back() = m_end;
  return space;
}

/**
* Calculate the chebyshev expansion coefficients given function values
* at the x-points.
* @param p :: Function values at chebyshev points.
*/
std::vector<double> ChebfunBase::calcA(const std::vector<double> &p) const {
  const size_t nn = m_n + 1;

  if (p.size() != nn) {
    throw std::invalid_argument(
        "ChebfunBase: function vector must have same size as the base.");
  }

  std::vector<double> a(nn);

  //// This is a correct and direct transform from m_p to m_a
  //// DO NOT DELETE !!!
  //    for(int i = 0; i < nn; ++i)
  //    {
  //      double t = 0.;
  //      for(int j = 0; j <= m_n; j++)
  //      {
  //        double p1 = p[m_n - j];
  //        if (j== 0 || j == m_n) p1 /= 2;
  //        t += cos(M_PI*i*(double(j))/m_n)*p1;
  //      }
  //      a[i] = 2*t/m_n;
  //      //if (i == 0) m_a[0] /= 2;
  //    }
  //    a[0] /= 2;
  //    a[m_n] /= 2;
  //    return a;
  //// End of the correct and direct transform from m_p to m_a

  if (m_n > 0) {
    // This is a magic trick which uses real fft to do the above cosine
    // transform
    std::vector<double> tmp(m_n * 2);
    std::reverse_copy(p.begin(), p.end(), tmp.begin());
    std::copy(p.begin() + 1, p.end() - 1, tmp.begin() + m_n + 1);

    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(2 * m_n);
    gsl_fft_real_wavetable *wavetable = gsl_fft_real_wavetable_alloc(2 * m_n);
    gsl_fft_real_transform(&tmp[0], 1, 2 * m_n, wavetable, workspace);
    gsl_fft_real_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    HalfComplex fc(&tmp[0], tmp.size());
    for (size_t i = 0; i < nn; ++i) {
      a[i] = fc.real(i) / m_n;
    }
    a[0] /= 2;
    a[m_n] /= 2;
    // End of the magic trick
  } else {
    a[0] = p[0];
  }
  return a;
}

/**
 * Calculate function values at chebyshev points given chebyshev
 * expansion coefficiens (inverse of calcA()).
 * @param a :: Chebyshev expansion coefficients.
 * @return Function values.
 */
std::vector<double> ChebfunBase::calcP(const std::vector<double> &a) const {
  if (m_n + 1 != a.size()) {
    std::stringstream mess;
    mess << "chebfun: cannot calculate P from A - different sizes: " << m_n + 1
         << " != " << a.size();
    throw std::invalid_argument(mess.str());
  }
  std::vector<double> p(m_n + 1);

  if (m_n > 0) {
    size_t nn = m_n + 1;
    std::vector<double> tmp(m_n * 2);
    HalfComplex fc(&tmp[0], tmp.size());
    for (size_t i = 0; i < nn; ++i) {
      double d = a[i] / 2;
      if (i == 0 || i == nn - 1)
        d *= 2;
      fc.set(i, d, 0.0);
    }
    gsl_fft_real_workspace *workspace = gsl_fft_real_workspace_alloc(2 * m_n);
    gsl_fft_halfcomplex_wavetable *wavetable =
        gsl_fft_halfcomplex_wavetable_alloc(2 * m_n);

    gsl_fft_halfcomplex_transform(tmp.data(), 1, 2 * m_n, wavetable, workspace);

    gsl_fft_halfcomplex_wavetable_free(wavetable);
    gsl_fft_real_workspace_free(workspace);

    std::reverse_copy(tmp.begin(), tmp.begin() + nn, p.begin());
  } else {
    p[0] = a[0];
  }
  return p;
}

/**
 * Approximate a function using this base.
 * @param f :: A function pointer.
 * @return Function values at the base x-points.
 */
std::vector<double> ChebfunBase::fit(ChebfunFunctionType f) const {
  std::vector<double> res(size());
  std::transform(m_x.begin(), m_x.end(), res.begin(), f);
  return res;
}

/**
 * Approximate a function using this base.
 * @param f :: A reference to an IFunction
 * @return Function values at the base x-points.
 */
std::vector<double> ChebfunBase::fit(const API::IFunction &f) const {
  const API::IFunction1D *fun1d = dynamic_cast<const API::IFunction1D *>(&f);
  if (!fun1d) {
    throw std::runtime_error("Function is not 1D.");
  }
  std::vector<double> res(size());
  fun1d->function1D(res.data(), m_x.data(), size());
  return res;
}

/**
 * Calculate function values at odd-valued indices of the base x-points.
 * This method is used by bestFit to minimize the number of calls to the
 * approximated function.
 * @param f :: Function to calculate.
 * @param p :: Values of function f at the even-valued indices of m_x.
 */
std::vector<double> ChebfunBase::fitOdd(ChebfunFunctionType f,
                                        std::vector<double> &p) const {
  assert(size() == p.size() * 2 - 1);
  assert(size() % 2 == 1);
  auto &xp = xPoints();
  std::vector<double> res(xp.size());
  auto it2 = res.begin();
  auto it1 = p.begin();
  // xp is odd-sized so the loop is ok
  for (auto x = xp.begin() + 1; x != xp.end(); x += 2, ++it1, ++it2) {
    *it2 = *it1; // one value from the previous iteration
    ++it2;
    *it2 = f(*x); // one new value
  }
  *(res.end() - 1) = p.back();
  return res;
}

/**
 * Calculate function values at odd-valued indices of the base x-points.
 * This method is used by bestFit to minimize the number of calls to the
 * approximated function.
 * @param f :: Function to calculate.
 * @param p :: Values of function f at the even-valued indices of m_x.
 */
std::vector<double> ChebfunBase::fitOdd(const API::IFunction &f,
                                        std::vector<double> &pEven) const {
  assert(size() == pEven.size() * 2 - 1);
  assert(size() % 2 == 1);
  const API::IFunction1D *fun1d = dynamic_cast<const API::IFunction1D *>(&f);
  if (!fun1d) {
    throw std::runtime_error("Function is not 1D.");
  }
  std::vector<double> pOdd(size() - pEven.size());
  std::vector<double> xOdd;
  xOdd.reserve(pOdd.size());
  // m_x is odd-sized so the loop is ok
  for (auto x = m_x.begin() + 1; x != m_x.end(); x += 2) {
    xOdd.push_back(*x);
  }

  fun1d->function1D(pOdd.data(), xOdd.data(), xOdd.size());

  std::vector<double> res(size());
  for (size_t i = 0; i < xOdd.size(); ++i) {
    res[2 * i] = pEven[i];
    res[2 * i + 1] = pOdd[i];
  }
  res.back() = pEven.back();
  return res;
}

/**
  * Find all roots of this chebfun.
  * @param a :: A vector with the Chebyshev expansion coefficients.
  * @return A vector with root values, unordered. If empty function
  * has no roots.
  */
std::vector<double> ChebfunBase::roots(const std::vector<double> &a) const {
  std::vector<double> r;
  // build the companion matrix
  size_t N = order();
  // ensure that the highest order coeff is > epsilon
  const double epsilon = std::numeric_limits<double>::epsilon() * 100;
  while (N > 0 && fabs(a[N]) < epsilon) {
    --N;
  }

  if (N == 0)
    return r; // function is a constant

  const size_t N2 = 2 * N;
  GSLMatrix C(N2, N2);
  C.zero();
  const double an = a[N];

  const size_t lasti = N2 - 1;
  for (size_t i = 0; i < N; ++i) {
    if (i > 0) {
      C.set(i, i - 1, 1.0);
    }
    C.set(N + i, N + i - 1, 1.0);
    C.set(i, lasti, -a[N - i] / an);
    double tmp = -a[i] / an;
    if (i == 0)
      tmp *= 2;
    C.set(N + i, lasti, tmp);
  }

  gsl_vector_complex *eval = gsl_vector_complex_alloc(N2);
  auto workspace = gsl_eigen_nonsymm_alloc(N2);
  gsl_eigen_nonsymm(C.gsl(), eval, workspace);
  gsl_eigen_nonsymm_free(workspace);

  const double Dx = endX() - startX();
  bool isFirst = true;
  double firstIm = 0;
  for (size_t i = 0; i < N2; ++i) {
    auto val = gsl_vector_complex_get(eval, i);
    double re = GSL_REAL(val);
    double im = GSL_IMAG(val);
    double ab = re * re + im * im;
    if (fabs(ab - 1.0) > 1e-2) {
      isFirst = true;
      continue;
    }
    if (isFirst) {
      isFirst = false;
      firstIm = im;
    } else {
      if (im + firstIm < 1e-10) {
        double x = startX() + (re + 1.0) / 2.0 * Dx;
        r.push_back(x);
      }
      isFirst = true;
    }
  }
  gsl_vector_complex_free(eval);

  return r;
}

} // CurveFitting
} // Mantid
