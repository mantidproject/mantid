//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidKernel/Math/ChebyshevPolyFit.h"
#include "MantidKernel/Math/Distributions/ChebyshevPolynomial.h"
#include <gsl/gsl_multifit.h>
#include <cassert>

namespace Mantid {
namespace Kernel {

//-----------------------------------------------------------------------------
// ChebyshevPolyFitImpl - Keeps the gsl stuff out of the headers
//-----------------------------------------------------------------------------
///@cond
class ChebyshevPolyFitImpl {
public:
  ChebyshevPolyFitImpl(const size_t order) : m_order(order) {}

  std::vector<double> fit(const std::vector<double> &xs,
                          const std::vector<double> &ys,
                          const std::vector<double> &wgts);

private:
  const size_t m_order;
};

//-----------------------------------------------------------------------------
// ChebyshevPolyFitImpl Public Members
//-----------------------------------------------------------------------------
// See doc for main class for parameter descriptions
std::vector<double> ChebyshevPolyFitImpl::fit(const std::vector<double> &xs,
                                              const std::vector<double> &ys,
                                              const std::vector<double> &wgts) {
  assert(xs.size() == ys.size());
  assert(ys.size() == wgts.size());

  const size_t npoints = ys.size();
  auto gslY = gsl_vector_const_view_array(ys.data(), npoints);
  auto gslW = gsl_vector_const_view_array(wgts.data(), npoints);

  // Fit kth-order chebyshev polynomial to each point essentially solving
  //   y = Xc
  // where c_i are the expansion cofficients
  const double xmin(xs.front()), xmax(xs.back());
  const size_t nparams = m_order + 1;
  auto *X = gsl_matrix_alloc(npoints, nparams);
  ChebyshevPolynomial chebyp;
  for (size_t i = 0; i < npoints; ++i) {
    const double xi = xs[i];
    // Map to the applicable domain [-1,1] for the Chebyshev polynomial
    const double xp = ((xi - xmin) - (xmax - xi)) / (xmax - xmin);
    for (size_t j = 0; j < nparams; ++j) {
      gsl_matrix_set(X, i, j, chebyp(j, xp));
    }
  }
  auto *coef = gsl_vector_alloc(nparams);
  auto *covar = gsl_matrix_alloc(nparams, nparams);
  // output variables
  double chisq(0.0);
  auto *work = gsl_multifit_linear_alloc(npoints, nparams);
  gsl_multifit_wlinear(X, &gslW.vector, &gslY.vector, coef, covar, &chisq,
                       work);
  gsl_multifit_linear_free(work);

  gsl_matrix_free(X);
  gsl_matrix_free(covar);

  std::vector<double> coeffs(coef->data, coef->data + nparams);
  gsl_vector_free(coef);
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
ChebyshevPolyFit::~ChebyshevPolyFit() {}

/**
 * Perform the fit for the given data set
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
