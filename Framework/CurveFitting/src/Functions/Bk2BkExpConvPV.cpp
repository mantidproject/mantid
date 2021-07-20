// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include <cmath>

#include "MantidAPI/Axis.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidCurveFitting/Functions/Bk2BkExpConvPV.h"
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidKernel/System.h"
#include "MantidKernel/UnitFactory.h"

#include <gsl/gsl_sf_erf.h>

using namespace Mantid::Kernel;

using namespace Mantid::API;

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
using namespace CurveFitting::SpecialFunctionSupport;
namespace {
/// static logger
Kernel::Logger g_log("Bk2BkExpConvPV");
} // namespace

DECLARE_FUNCTION(Bk2BkExpConvPV)

// ----------------------------
/** Constructor and Desctructor
 */
Bk2BkExpConvPV::Bk2BkExpConvPV() : mFWHM(0.0) {}

/** Initialize:  declare paraemters
 */
void Bk2BkExpConvPV::init() {
  declareParameter("X0", -0.0, "Location of the peak");
  declareParameter("Intensity", 0.0, "Integrated intensity");
  declareParameter("Alpha", 1.0, "Exponential rise");
  declareParameter("Beta", 1.0, "Exponential decay");
  declareParameter("Sigma2", 1.0, "Sigma squared");
  declareParameter("Gamma", 0.0);
}

/** Set peak height
 */
void Bk2BkExpConvPV::setHeight(const double h) {
  setParameter("Intensity", 1);
  double h0 = height();

  // avoid divide by zero
  double minCutOff = 100.0 * std::numeric_limits<double>::min();
  if (h0 >= 0 && h0 < minCutOff)
    h0 = minCutOff;
  else if (h0 < 0 && h0 > -minCutOff)
    h0 = -minCutOff;

  setParameter("Intensity", h / h0);
}

/** Get peak height
 */
double Bk2BkExpConvPV::height() const {
  double height[1];
  double peakCentre[1];
  peakCentre[0] = this->getParameter("X0");
  this->functionLocal(height, peakCentre, 1);
  return height[0];
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
 * This cannot be set directly so we assume Gamma is 0 and set Sigma by doing
 * the reverse of calHandEta
 */
void Bk2BkExpConvPV::setFwhm(const double w) {
  this->setParameter("Gamma", 0);
  auto sigma2 = std::pow(w, 2) / (8.0 * M_LN2);
  this->setParameter("Sigma2", sigma2);
}

/** Set peak center
 */
void Bk2BkExpConvPV::setCentre(const double c) { setParameter("X0", c); }

/** Center
 */
double Bk2BkExpConvPV::centre() const { return getParameter("X0"); }

void Bk2BkExpConvPV::setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi, double startX,
                                        double endX) {
  if (workspace) {
    // convert alpha and beta to correct units so inital guess is resonable
    auto tof = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    const auto centre = getParameter("X0");
    const auto scaleFactor = centre / convertValue(centre, tof, workspace, wi);
    if (scaleFactor != 0) {
      if (isActive(parameterIndex("Alpha")))
        setParameter("Alpha", getParameter("Alpha") / scaleFactor);
      if (isActive(parameterIndex("Beta")))
        setParameter("Beta", getParameter("Beta") / scaleFactor);
    }
  }
  IFunctionMW::setMatrixWorkspace(workspace, wi, startX, endX);
}

/** Implement the peak calculating formula
 */
void Bk2BkExpConvPV::functionLocal(double *out, const double *xValues, const size_t nData) const {
  // 1. Prepare constants
  const double alpha = this->getParameter("Alpha");
  const double beta = this->getParameter("Beta");
  const double sigma2 = this->getParameter("Sigma2");
  const double gamma = this->getParameter("Gamma");
  const double intensity = this->getParameter("Intensity");
  const double x0 = this->getParameter("X0");

  double invert_sqrt2sigma = 1.0 / sqrt(2.0 * sigma2);
  double N = alpha * beta * 0.5 / (alpha + beta);

  double H, eta;
  calHandEta(sigma2, gamma, H, eta);

  g_log.debug() << "DB1143:  nData = " << nData << " From " << xValues[0] << " To " << xValues[nData - 1]
                << " X0 = " << x0 << " Intensity = " << intensity << " alpha = " << alpha << " beta = " << beta
                << " H = " << H << " eta = " << eta << '\n';

  // 2. Do calculation for each data point
  for (size_t id = 0; id < nData; ++id) {
    double dT = xValues[id] - x0;
    double omega = calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma);
    out[id] = intensity * omega;
  }
}

/** Local derivative
 */
void Bk2BkExpConvPV::functionDerivLocal(API::Jacobian * /*jacobian*/, const double * /*xValues*/,
                                        const size_t /*nData*/) {
  throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for Bk2BkExpConvPV.");
}

/** Numerical derivative
 */
void Bk2BkExpConvPV::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

/** Calculate Omega(x) = ... ...
 */
double Bk2BkExpConvPV::calOmega(double x, double eta, double N, double alpha, double beta, double H, double sigma2,
                                double invert_sqrt2sigma) const {
  // 1. Prepare
  std::complex<double> p(alpha * x, alpha * H * 0.5);
  std::complex<double> q(-beta * x, beta * H * 0.5);

  double u = 0.5 * alpha * (alpha * sigma2 + 2 * x);
  double y = (alpha * sigma2 + x) * invert_sqrt2sigma;

  double v = 0.5 * beta * (beta * sigma2 - 2 * x);
  double z = (beta * sigma2 - x) * invert_sqrt2sigma;

  // 2. Calculate
  double omega1 = (1 - eta) * N * (exp(u + gsl_sf_log_erfc(y)) + exp(v + gsl_sf_log_erfc(z)));
  double omega2;
  if (eta < 1.0E-8) {
    omega2 = 0.0;
  } else {
    omega2 = 2 * N * eta / M_PI * (imag(exponentialIntegral(p)) + imag(exponentialIntegral(q)));
  }
  double omega = omega1 - omega2;

  return omega;
}

void Bk2BkExpConvPV::geneatePeak(double *out, const double *xValues, const size_t nData) {
  this->functionLocal(out, xValues, nData);
}

void Bk2BkExpConvPV::calHandEta(double sigma2, double gamma, double &H, double &eta) const {
  // 1. Calculate H
  double H_G = sqrt(8.0 * sigma2 * M_LN2);
  double H_L = gamma;

  double temp1 = std::pow(H_L, 5) + 0.07842 * H_G * std::pow(H_L, 4) + 4.47163 * std::pow(H_G, 2) * std::pow(H_L, 3) +
                 2.42843 * std::pow(H_G, 3) * std::pow(H_L, 2) + 2.69269 * std::pow(H_G, 4) * H_L + std::pow(H_G, 5);

  H = std::pow(temp1, 0.2);

  mFWHM = H;

  // 2. Calculate eta
  double gam_pv = H_L / H;
  eta = 1.36603 * gam_pv - 0.47719 * std::pow(gam_pv, 2) + 0.11116 * std::pow(gam_pv, 3);

  if (eta > 1 || eta < 0) {
    g_log.error() << "Bk2BkExpConvPV: Calculated eta = " << eta << " is out of range [0, 1].\n";
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
