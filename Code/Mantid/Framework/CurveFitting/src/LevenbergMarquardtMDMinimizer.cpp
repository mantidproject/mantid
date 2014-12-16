//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/CostFuncLeastSquares.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"
#include "MantidAPI/IFunction.h"

#include "MantidKernel/Logger.h"

#include <boost/lexical_cast.hpp>
#include <gsl/gsl_blas.h>
#include <iostream>
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace {
/// static logger object
Kernel::Logger g_log("LevenbergMarquardMD");
}

DECLARE_FUNCMINIMIZER(LevenbergMarquardtMDMinimizer, Levenberg - MarquardtMD)

/// Constructor
LevenbergMarquardtMDMinimizer::LevenbergMarquardtMDMinimizer()
    : IFuncMinimizer(), m_tau(1e-6), m_mu(1e-6), m_nu(2.0), m_rho(1.0) {
  declareProperty("MuMax", 1e6,
                  "Maximum value of mu - a stopping parameter in failure.");
  declareProperty("AbsError", 0.0001, "Absolute error allowed for parameters - "
                                      "a stopping parameter in success.");
  declareProperty("Debug", false, "Turn on the debug output.");
}

/// Initialize minimizer, i.e. pass a function to minimize.
void LevenbergMarquardtMDMinimizer::initialize(API::ICostFunction_sptr function,
                                               size_t) {
  m_leastSquares = boost::dynamic_pointer_cast<CostFuncLeastSquares>(function);
  if (!m_leastSquares) {
    throw std::invalid_argument("Levenberg-Marquardt minimizer works only with "
                                "least squares. Different function was given.");
  }
  m_mu = 0;
  m_nu = 2.0;
  m_rho = 1.0;
}

/// Do one iteration.
bool LevenbergMarquardtMDMinimizer::iterate(size_t) {
  const bool debug = getProperty("Debug");
  const double muMax = getProperty("MuMax");
  const double absError = getProperty("AbsError");

  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }
  size_t n = m_leastSquares->nParams();

  if (n == 0) {
    m_errorString = "No parameters to fit.";
    g_log.information(m_errorString);
    return false;
  }

  if (m_mu > muMax) {
    // m_errorString = "Failed to converge, maximum mu reached";
    // g_log.warning() << m_errorString << std::endl;
    return false;
  }

  // calculate the first and second derivatives of the cost function.
  if (m_mu == 0.0) { // first time calculate everything
    m_F = m_leastSquares->valDerivHessian();
  } else if (m_rho > 0) { // last iteration was good: calculate new m_der and
                          // m_hessian, dont't recalculate m_F
    m_leastSquares->valDerivHessian(false);
  }
  // else if m_rho < 0 last iteration was bad: reuse m_der and m_hessian

  // Calculate damping to hessian
  if (m_mu == 0) // first iteration or accidental zero
  {
    m_mu = m_tau;
    m_nu = 2.0;
  }

  if (debug) {
    g_log.warning()
        << "==========================================================="
        << std::endl;
    g_log.warning() << "mu=" << m_mu << std::endl << std::endl;
  }

  if (m_D.empty()) {
    m_D.resize(n);
  }

  // copy the hessian
  GSLMatrix H(m_leastSquares->getHessian());
  GSLVector dd(m_leastSquares->getDeriv());

  // scaling factors
  std::vector<double> sf(n);

  for (size_t i = 0; i < n; ++i) {
    double d = fabs(dd.get(i));
    if (m_D[i] > d)
      d = m_D[i];
    m_D[i] = d;
    double tmp = H.get(i, i) + m_mu * d;
    H.set(i, i, tmp);
    sf[i] = sqrt(tmp);
    if (tmp == 0.0) {
      m_errorString = "Singular matrix.";
      g_log.information(m_errorString);
      return false;
    }
  }

  // apply scaling
  for (size_t i = 0; i < n; ++i) {
    double d = dd.get(i);
    dd.set(i, d / sf[i]);
    for (size_t j = i; j < n; ++j) {
      const double f = sf[i] * sf[j];
      double tmp = H.get(i, j);
      H.set(i, j, tmp / f);
      if (i != j) {
        tmp = H.get(j, i);
        H.set(j, i, tmp / f);
      }
    }
  }

  if (debug && m_rho > 0) {
    g_log.warning() << "Hessian:\n" << H;
    g_log.warning() << "Right-hand side:\n";
    for (size_t j = 0; j < n; ++j) {
      g_log.warning() << dd.get(j) << ' ';
    }
    g_log.warning() << std::endl;
    g_log.warning() << "Determinant=" << H.det() << std::endl;
  }

  // Parameter corrections
  GSLVector dx(n);
  // To find dx solve the system of linear equations   H * dx == -m_der
  dd *= -1.0;
  H.solve(dd, dx);

  if (debug) {
    g_log.warning() << "\nScaling factors:" << std::endl;
    for (size_t j = 0; j < n; ++j) {
      g_log.warning() << sf[j] << ' ';
    }
    g_log.warning() << std::endl;
    g_log.warning() << "Corrections:" << std::endl;
    for (size_t j = 0; j < n; ++j) {
      g_log.warning() << dx.get(j) << ' ';
    }
    g_log.warning() << std::endl << std::endl;
  }

  // restore scaling
  for (size_t i = 0; i < n; ++i) {
    double d = dx.get(i);
    dx.set(i, d / sf[i]);
    d = dd.get(i);
    dd.set(i, d * sf[i]);
  }

  // save previous state
  m_leastSquares->push();
  // Update the parameters of the cost function.
  for (size_t i = 0; i < n; ++i) {
    double d = m_leastSquares->getParameter(i) + dx.get(i);
    m_leastSquares->setParameter(i, d);
    if (debug) {
      g_log.warning() << "Parameter(" << i << ")=" << d << std::endl;
    }
  }
  m_leastSquares->getFittingFunction()->applyTies();

  // --- prepare for the next iteration --- //

  double dL;
  // der -> - der - 0.5 * hessian * dx
  gsl_blas_dgemv(CblasNoTrans, -0.5, m_leastSquares->getHessian().gsl(),
                 dx.gsl(), 1., dd.gsl());
  // calculate the linear part of the change in cost function
  // dL = - der * dx - 0.5 * dx * hessian * dx
  gsl_blas_ddot(dd.gsl(), dx.gsl(), &dL);

  double F1 = m_leastSquares->val();
  if (debug) {
    g_log.warning() << std::endl;
    g_log.warning() << "Old cost function " << m_F << std::endl;
    g_log.warning() << "New cost function " << F1 << std::endl;
    g_log.warning() << "Linear part " << dL << std::endl;
  }

  // Try the stop condition
  if (m_rho >= 0) {
    GSLVector p(n);
    m_leastSquares->getParameters(p);
    double dx_norm = gsl_blas_dnrm2(dx.gsl());
    if (dx_norm < absError) {
      if (debug) {
        g_log.warning() << "Successful fit, parameters changed by less than "
                        << absError << std::endl;
      }
      return false;
    }
    if (m_rho == 0) {
      if (m_F != F1) {
        this->m_errorString = "Failed to converge, rho == 0";
        g_log.warning() << m_errorString << std::endl;
      }
      if (debug) {
        g_log.warning() << "Successful fit, cost function didn't change."
                        << std::endl;
      }
      return false;
    }
  }

  if (fabs(dL) == 0.0) {
    if (m_F == F1)
      m_rho = 1.0;
    else
      m_rho = 0;
  } else {
    m_rho = (m_F - F1) / dL;
    if (m_rho == 0) {
      return false;
    }
  }
  if (debug) {
    g_log.warning() << "rho=" << m_rho << std::endl;
  }

  if (m_rho > 0) { // good progress, decrease m_mu but no more than by 1/3
    // rho = 1 - (2*rho - 1)^3
    m_rho = 2.0 * m_rho - 1.0;
    m_rho = 1.0 - m_rho * m_rho * m_rho;
    const double I3 = 1.0 / 3.0;
    if (m_rho > I3)
      m_rho = I3;
    if (m_rho < 0.0001)
      m_rho = 0.1;
    m_mu *= m_rho;
    m_nu = 2.0;
    m_F = F1;
    if (debug) {
      g_log.warning() << "Good iteration, accept new parameters." << std::endl;
      g_log.warning() << "rho=" << m_rho << std::endl;
    }
    // drop saved state, accept new parameters
    m_leastSquares->drop();
  } else { // bad iteration. increase m_mu and revert changes to parameters
    m_mu *= m_nu;
    m_nu *= 2.0;
    // undo parameter update
    m_leastSquares->pop();
    m_F = m_leastSquares->val();
    if (debug) {
      g_log.warning()
          << "Bad iteration, increase mu and revert changes to parameters."
          << std::endl;
    }
  }

  return true;
}

/// Return current value of the cost function
double LevenbergMarquardtMDMinimizer::costFunctionVal() {
  if (!m_leastSquares) {
    throw std::runtime_error("Cost function isn't set up.");
  }
  return m_leastSquares->val();
}

} // namespace CurveFitting
} // namespace Mantid
