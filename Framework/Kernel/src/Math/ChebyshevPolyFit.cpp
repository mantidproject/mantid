//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Math/ChebyshevPolyFit.h"
#include "MantidKernel/Math/Distributions/ChebyshevPolynomial.h"
#include <cassert>
#include <gsl/gsl_multifit.h>

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// ChebyshevPolyFitImpl - Keeps the gsl stuff out of the headers
//-----------------------------------------------------------------------------
///@cond
class ChebyshevPolyFitImpl {
public:
  explicit ChebyshevPolyFitImpl(const size_t order) : m_order(order) {}

  std::vector<double> fit(const std::vector<double> &x,
                          const std::vector<double> &y,
                          const std::vector<double> &w);

private:
  const size_t m_order;
};

//-----------------------------------------------------------------------------
// ChebyshevPolyFitImpl Public Members
//-----------------------------------------------------------------------------
// See doc for main class for parameter descriptions
std::vector<double> ChebyshevPolyFitImpl::fit(const std::vector<double> &x,
                                              const std::vector<double> &y,
                                              const std::vector<double> &w) {
  assert(x.size() == y.size());
  assert(y.size() == w.size());

  const size_t npts(y.size()), degp1(m_order + 1);
  const double xmin(x.front()), xmax(x.back());

  // Compute the weighted pseudo Vandermonde matrix
  // and y*w
  gsl_matrix *MX = gsl_matrix_alloc(npts, degp1);
  auto *yw = gsl_vector_alloc(npts);
  ChebyshevPolynomial chebyp;
  for (size_t i = 0; i < npts; ++i) {
    const auto xi = x[i];
    const auto wi = w[i];
    const auto xbar = ((xi - xmin) - (xmax - xi)) / (xmax - xmin);
    gsl_vector_set(yw, i, wi * y[i]);
    for (size_t j = 0; j < degp1; ++j) {
      gsl_matrix_set(MX, i, j, wi * chebyp(j, xbar));
    }
  }

  // Least-squares fit to determine c
  auto *c = gsl_vector_alloc(degp1);
  auto *cov = gsl_matrix_alloc(degp1, degp1);
  auto *work = gsl_multifit_linear_alloc(npts, degp1);
  double chisq(0.0);
  gsl_multifit_linear(MX, yw, c, cov, &chisq, work);
  gsl_vector_free(yw);
  gsl_matrix_free(cov);
  gsl_matrix_free(MX);
  gsl_multifit_linear_free(work);

  std::vector<double> coeffs(c->data, c->data + degp1);
  gsl_vector_free(c);
  return coeffs;
}

///@endcond

//-----------------------------------------------------------------------------
// ChebyshevPolyFit Public members
//-----------------------------------------------------------------------------
/**
 * Constructor
 * @param n The maximum degree of polynomial required (+1 for the zeroth-order
 * term)
 */
ChebyshevPolyFit::ChebyshevPolyFit(const size_t n)
    : m_impl(new ChebyshevPolyFitImpl(n)) {}

/// Destructor
ChebyshevPolyFit::~ChebyshevPolyFit() = default;

/**
 * Find coefficients of polynomial that minimizes the sum
 * of the squares of the residuals:
 *      e_r = w_r(y_r - f_r)
 * Solves the overdetermined equation
 *      w * V(x) * c = w * y
 *
 * @param xs X values
 * @param ys Y data points
 * @param wgts Weights for each Y point
 * @return
 */
std::vector<double> ChebyshevPolyFit::
operator()(const std::vector<double> &xs, const std::vector<double> &ys,
           const std::vector<double> &wgts) {
  return m_impl->fit(xs, ys, wgts);
}

} // namespace Kernel
} // namespace Mantid
