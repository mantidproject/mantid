#include "MantidCurveFitting/Bk2BkExpConvPV.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <gsl/gsl_sf_erf.h>


using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace Mantid {
namespace CurveFitting {
namespace {
/// static logger
Kernel::Logger g_log("Bk2BkExpConvPV");
}

DECLARE_FUNCTION(Bk2BkExpConvPV)

// ----------------------------
/** Constructor and Desctructor
 */
Bk2BkExpConvPV::Bk2BkExpConvPV() : mFWHM(0.0) {}

Bk2BkExpConvPV::~Bk2BkExpConvPV() {}

/** Initialize:  declare paraemters
 */
void Bk2BkExpConvPV::init() {
  declareParameter("TOF_h", -0.0);
  declareParameter("Height", 1.0);
  declareParameter("Alpha", 1.0);
  declareParameter("Beta", 1.0);
  declareParameter("Sigma2", 1.0);
  declareParameter("Gamma", 0.0);

  return;
}

/** Set peak height
  */
void Bk2BkExpConvPV::setHeight(const double h) {
  setParameter("Height", h);

  return;
}

/** Get peak height
  */
double Bk2BkExpConvPV::height() const {
  double height = this->getParameter("Height");
  return height;
}

/** Get peak's FWHM
  */
double Bk2BkExpConvPV::fwhm() const {
  double sigma2 = this->getParameter("Sigma2");
  double gamma = this->getParameter("Gamma");
  double H, eta;
  calHandEta(sigma2, gamma, H, eta);

  return mFWHM;
}

/** Set FWHM
  * It is an illegal operation of this type of peak
  */
void Bk2BkExpConvPV::setFwhm(const double w) {
  UNUSED_ARG(w);
  throw std::invalid_argument("Bk2BkExpConvPV is not allowed to set FWHM.");
}

/** Set peak center
  */
void Bk2BkExpConvPV::setCentre(const double c) { setParameter("TOF_h", c); }

/** Center
  */
double Bk2BkExpConvPV::centre() const {
  double tofh = getParameter("TOF_h");

  return tofh;
}

/** Implement the peak calculating formula
 */
void Bk2BkExpConvPV::functionLocal(double *out, const double *xValues,
                                   const size_t nData) const {
  // 1. Prepare constants
  const double alpha = this->getParameter("Alpha");
  const double beta = this->getParameter("Beta");
  const double sigma2 = this->getParameter("Sigma2");
  const double gamma = this->getParameter("Gamma");
  const double height = this->getParameter("Height");
  const double tof_h = this->getParameter("TOF_h");

  double invert_sqrt2sigma = 1.0 / sqrt(2.0 * sigma2);
  double N = alpha * beta * 0.5 / (alpha + beta);

  double H, eta;
  calHandEta(sigma2, gamma, H, eta);

  /*
  g_log.debug() << "DB1143:  nData = " << nData << " From " << xValues[0] << "
  To " << xValues[nData-1]
                << " TOF_h = " << tof_h << " Height = " << height << " alpha = "
  << alpha << " beta = "
                << beta << " H = " << H << " eta = " << eta << std::endl;
                */

  // 2. Do calculation for each data point
  for (size_t id = 0; id < nData; ++id) {
    double dT = xValues[id] - tof_h;
    double omega =
        calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma);
    out[id] = height * omega;
  }

  return;
}

/** Local derivative
  */
void Bk2BkExpConvPV::functionDerivLocal(API::Jacobian *, const double *,
                                        const size_t) {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "functionDerivLocal is not implemented for IkedaCarpenterPV.");
}

/** Numerical derivative
  */
void Bk2BkExpConvPV::functionDeriv(const API::FunctionDomain &domain,
                                   API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

/** Calculate Omega(x) = ... ...
 */
double Bk2BkExpConvPV::calOmega(double x, double eta, double N, double alpha,
                                double beta, double H, double sigma2,
                                double invert_sqrt2sigma) const {
  // 1. Prepare
  std::complex<double> p(alpha * x, alpha * H * 0.5);
  std::complex<double> q(-beta * x, beta * H * 0.5);

  double u = 0.5 * alpha * (alpha * sigma2 + 2 * x);
  double y = (alpha * sigma2 + x) * invert_sqrt2sigma;

  double v = 0.5 * beta * (beta * sigma2 - 2 * x);
  double z = (beta * sigma2 - x) * invert_sqrt2sigma;

  // 2. Calculate
  double omega1 =
      (1 - eta) * N * (exp(u) * gsl_sf_erfc(y) + std::exp(v) * gsl_sf_erfc(z));
  double omega2;
  if (eta < 1.0E-8) {
    omega2 = 0.0;
  } else {
    omega2 = 2 * N * eta / M_PI * (imag(exp(p) * E1(p)) + imag(exp(q) * E1(q)));
  }
  double omega = omega1 + omega2;

  return omega;
}

/** Implementation of complex integral E_1
 */
std::complex<double> Bk2BkExpConvPV::E1(std::complex<double> z) const {
  std::complex<double> e1;

  double rz = real(z);
  double az = abs(z);

  if (fabs(az) < 1.0E-8) {
    // If z = 0, then the result is infinity... diverge!
    complex<double> r(1.0E300, 0.0);
    e1 = r;
  } else if (az <= 10.0 || (rz < 0.0 && az < 20.0)) {
    // Some interesting region, equal to integrate to infinity, converged
    complex<double> r(1.0, 0.0);
    e1 = r;
    complex<double> cr = r;

    for (size_t k = 0; k < 150; ++k) {
      double dk = double(k);
      cr = -cr * dk * z / ((dk + 2.0) * (dk + 2.0));
      e1 += cr;
      if (abs(cr) < abs(e1) * 1.0E-15) {
        // cr is converged to zero
        break;
      }
    } // ENDFOR k

    e1 = -e1 - log(z) + (z * e1);
  } else {
    complex<double> ct0(0.0, 0.0);
    for (int k = 120; k > 0; --k) {
      complex<double> dk(double(k), 0.0);
      ct0 = dk / (10.0 + dk / (z + ct0));
    } // ENDFOR k

    e1 = 1.0 / (z + ct0);
    e1 = e1 * exp(-z);
    if (rz < 0.0 && fabs(imag(z)) < 1.0E-10) {
      complex<double> u(0.0, 1.0);
      e1 = e1 - (M_PI * u);
    }
  }

  return e1;
}

void Bk2BkExpConvPV::geneatePeak(double *out, const double *xValues,
                                 const size_t nData) {
  this->functionLocal(out, xValues, nData);

  return;
}

void Bk2BkExpConvPV::calHandEta(double sigma2, double gamma, double &H,
                                double &eta) const {
  // 1. Calculate H
  double H_G = sqrt(8.0 * sigma2 * log(2.0));
  double H_L = gamma;

  double temp1 = std::pow(H_L, 5) + 0.07842 * H_G * std::pow(H_L, 4) +
                 4.47163 * std::pow(H_G, 2) * std::pow(H_L, 3) +
                 2.42843 * std::pow(H_G, 3) * std::pow(H_L, 2) +
                 2.69269 * std::pow(H_G, 4) * H_L + std::pow(H_G, 5);

  H = std::pow(temp1, 0.2);

  mFWHM = H;

  // 2. Calculate eta
  double gam_pv = H_L / H;
  eta = 1.36603 * gam_pv - 0.47719 * std::pow(gam_pv, 2) +
        0.11116 * std::pow(gam_pv, 3);

  if (eta > 1 || eta < 0) {
    g_log.error() << "Bk2BkExpConvPV: Calculated eta = " << eta
                  << " is out of range [0, 1]." << std::endl;
  }

  return;
}

} // namespace Mantid
} // namespace CurveFitting
