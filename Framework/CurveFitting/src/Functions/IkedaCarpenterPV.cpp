// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/IkedaCarpenterPV.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/PeakFunctionIntegrator.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidKernel/UnitFactory.h"

#include <cmath>
#include <gsl/gsl_math.h>
#include <gsl/gsl_multifit_nlin.h>
#include <gsl/gsl_sf_erf.h>
#include <limits>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

namespace {
/// static logger
Kernel::Logger g_log("IkedaCarpenterPV");
} // namespace

using namespace Kernel;

using namespace CurveFitting::SpecialFunctionSupport;

using namespace Geometry;
using namespace Constraints;

DECLARE_FUNCTION(IkedaCarpenterPV)

double IkedaCarpenterPV::centre() const { return getParameter("X0"); }

void IkedaCarpenterPV::setHeight(const double h) {
  // calculate height of peakshape function corresponding to intensity = 1
  setParameter("I", 1);
  double h0 = height();

  // to avoid devide by zero and to insane value for I to be set
  double minCutOff = 100.0 * std::numeric_limits<double>::min();
  if (h0 > 0 && h0 < minCutOff)
    h0 = minCutOff;
  if (h0 < 0 && h0 > -minCutOff)
    h0 = -minCutOff;

  // The intensity is then estimated to be h/h0
  setParameter("I", h / h0);
}

double IkedaCarpenterPV::height() const {
  // return the function value at centre()
  // using arrays - otherwise coverity warning
  double h0[1];
  double toCentre[1];
  toCentre[0] = centre();
  constFunction(h0, toCentre, 1);
  return h0[0];
}

double IkedaCarpenterPV::fwhm() const {
  double sigmaSquared = getParameter("SigmaSquared");
  double gamma = getParameter("Gamma");

  if (sigmaSquared < 0) {
    g_log.debug() << "SigmaSquared NEGATIVE!.\n"
                  << "Likely due to a fit not converging properly\n"
                  << "If this is frequent problem please report to Mantid team.\n"
                  << "For now to calculate width force SigmaSquared positive.\n";
    sigmaSquared = -sigmaSquared;
  }
  if (gamma < 0) {
    g_log.debug() << "Gamma NEGATIVE!.\n"
                  << "Likely due to a fit not converging properly\n"
                  << "If this is frequent problem please report to Mantid team.\n"
                  << "For now to calculate width force Gamma positive.\n";
    gamma = -gamma;
    ;
  }
  return sqrt(8.0 * M_LN2 * sigmaSquared) + gamma;
}

void IkedaCarpenterPV::setFwhm(const double w) {
  setParameter("SigmaSquared", w * w / (32.0 * M_LN2)); // used 4.0 * 8.0 = 32.0
  setParameter("Gamma", w / 2.0);
}

void IkedaCarpenterPV::setCentre(const double c) { setParameter("X0", c); }

void IkedaCarpenterPV::init() {
  declareParameter("I", 0.0,
                   "The integrated intensity of the peak. I.e. "
                   "approximately equal to HWHM times height of "
                   "peak");
  this->lowerConstraint0("I");
  declareParameter("Alpha0", 1.6, "Used to model fast decay constant");
  this->lowerConstraint0("Alpha0");
  declareParameter("Alpha1", 1.5, "Used to model fast decay constant");
  this->lowerConstraint0("Alpha1");
  declareParameter("Beta0", 31.9, "Inverse of slow decay constant");
  this->lowerConstraint0("Beta0");
  declareParameter("Kappa", 46.0, "Controls contribution of slow decay term");
  this->lowerConstraint0("Kappa");
  declareParameter("SigmaSquared", 1.0, "standard deviation squared (Voigt Guassian broadening)");
  this->lowerConstraint0("SigmaSquared");
  declareParameter("Gamma", 1.0, "Voigt Lorentzian broadening");
  this->lowerConstraint0("Gamma");
  declareParameter("X0", 0.0, "Peak position");
  this->lowerConstraint0("X0");
}

void IkedaCarpenterPV::lowerConstraint0(const std::string &paramName) {
  auto mixingConstraint = std::make_unique<BoundaryConstraint>(this, paramName, 0.0, true);
  mixingConstraint->setPenaltyFactor(1e9);

  addConstraint(std::move(mixingConstraint));
}

/** Method for updating m_waveLength.
 *  If size of m_waveLength is equal to number of data (for a new instance of
 *this
 *  class this vector is empty initially) then don't recalculate it.
 *
 *  @param xValues :: x values
 *  @param nData :: length of xValues
 */
void IkedaCarpenterPV::calWavelengthAtEachDataPoint(const double *xValues, const size_t &nData) const {
  // if wavelength vector already have the right size no need for resizing it
  // further we make the assumption that no need to recalculate this vector if
  // it already has the right size

  if (m_waveLength.size() != nData) {
    m_waveLength.resize(nData);

    Mantid::Kernel::Unit_sptr wavelength = Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    for (size_t i = 0; i < nData; i++) {
      m_waveLength[i] = xValues[i];
    }

    // note if a version of convertValue was added which allows a double* as
    // first argument then could avoid copying above plus only have to resize
    // m_wavelength when its size smaller than nData
    API::MatrixWorkspace_const_sptr mws = getMatrixWorkspace();
    if (mws) {
      Instrument_const_sptr instrument = mws->getInstrument();
      Geometry::IComponent_const_sptr sample = instrument->getSample();
      if (sample != nullptr) {
        convertValue(m_waveLength, wavelength, mws, m_workspaceIndex);
      } else {
        g_log.warning() << "No sample set for instrument in workspace.\n"
                        << "Can't calculate wavelength in IkedaCarpenter.\n"
                        << "Default all wavelengths to one.\n"
                        << "Solution is to load appropriate instrument into workspace.\n";
        for (size_t i = 0; i < nData; i++)
          m_waveLength[i] = 1.0;
      }
    } else {
      g_log.warning() << "Workspace not set.\n"
                      << "Can't calculate wavelength in IkedaCarpenter.\n"
                      << "Default all wavelengths to one.\n"
                      << "Solution call setMatrixWorkspace() for function.\n";
      for (size_t i = 0; i < nData; i++)
        m_waveLength[i] = 1.0;
    }
  }
}

/** convert voigt params to pseudo voigt params
 *
 *  @param voigtSigmaSq :: voigt param
 *  @param voigtGamma :: voigt param
 *  @param H :: pseudo voigt param
 *  @param eta :: pseudo voigt param
 */
void IkedaCarpenterPV::convertVoigtToPseudo(const double &voigtSigmaSq, const double &voigtGamma, double &H,
                                            double &eta) const {
  double fwhmGsq = 8.0 * M_LN2 * voigtSigmaSq;
  double fwhmG = sqrt(fwhmGsq);
  double fwhmG4 = fwhmGsq * fwhmGsq;
  double fwhmL = voigtGamma;
  double fwhmLsq = voigtGamma * voigtGamma;
  double fwhmL4 = fwhmLsq * fwhmLsq;

  H = pow(fwhmG4 * fwhmG + 2.69269 * fwhmG4 * fwhmL + 2.42843 * fwhmGsq * fwhmG * fwhmLsq +
              4.47163 * fwhmGsq * fwhmLsq * fwhmL + 0.07842 * fwhmG * fwhmL4 + fwhmL4 * fwhmL,
          0.2);

  if (H == 0.0)
    H = std::numeric_limits<double>::epsilon() * 1000.0;

  double tmp = fwhmL / H;

  eta = 1.36603 * tmp - 0.47719 * tmp * tmp + 0.11116 * tmp * tmp * tmp;
}

void IkedaCarpenterPV::constFunction(double *out, const double *xValues, const int &nData) const {
  const double I = getParameter("I");
  const double alpha0 = getParameter("Alpha0");
  const double alpha1 = getParameter("Alpha1");
  const double beta0 = getParameter("Beta0");
  const double kappa = getParameter("Kappa");
  const double voigtsigmaSquared = getParameter("SigmaSquared");
  const double voigtgamma = getParameter("Gamma");
  const double X0 = getParameter("X0");

  // cal pseudo voigt sigmaSq and gamma and eta
  double gamma = 1.0; // dummy initialization
  double eta = 0.5;   // dummy initialization
  convertVoigtToPseudo(voigtsigmaSquared, voigtgamma, gamma, eta);
  double sigmaSquared = gamma * gamma / (8.0 * M_LN2);

  const double beta = 1 / beta0;

  // equations taken from Fullprof manual

  const double k = 0.05;

  // Not entirely sure what to do if sigmaSquared ever negative
  // for now just post a warning
  double someConst = std::numeric_limits<double>::max() / 100.0;
  if (sigmaSquared > 0)
    someConst = 1 / sqrt(2.0 * sigmaSquared);
  else if (sigmaSquared < 0) {
    g_log.warning() << "sigmaSquared negative in functionLocal.\n";
  }

  // update wavelength vector
  calWavelengthAtEachDataPoint(xValues, nData);

  for (int i = 0; i < nData; i++) {
    double diff = xValues[i] - X0;

    double R = exp(-81.799 / (m_waveLength[i] * m_waveLength[i] * kappa));
    double alpha = 1.0 / (alpha0 + m_waveLength[i] * alpha1);

    double a_minus = alpha * (1 - k);
    double a_plus = alpha * (1 + k);
    double x = a_minus - beta;
    double y = alpha - beta;
    double z = a_plus - beta;

    double Nu = 1 - R * a_minus / x;
    double Nv = 1 - R * a_plus / z;
    double Ns = -2 * (1 - R * alpha / y);
    double Nr = 2 * R * alpha * alpha * beta * k * k / (x * y * z);

    double u = a_minus * (a_minus * sigmaSquared - 2 * diff) / 2.0;
    double v = a_plus * (a_plus * sigmaSquared - 2 * diff) / 2.0;
    double s = alpha * (alpha * sigmaSquared - 2 * diff) / 2.0;
    double r = beta * (beta * sigmaSquared - 2 * diff) / 2.0;

    double yu = (a_minus * sigmaSquared - diff) * someConst;
    double yv = (a_plus * sigmaSquared - diff) * someConst;
    double ys = (alpha * sigmaSquared - diff) * someConst;
    double yr = (beta * sigmaSquared - diff) * someConst;

    std::complex<double> zs = std::complex<double>(-alpha * diff, 0.5 * alpha * gamma);
    std::complex<double> zu = (1 - k) * zs;
    std::complex<double> zv = (1 + k) * zs;
    std::complex<double> zr = std::complex<double>(-beta * diff, 0.5 * beta * gamma);

    double N = 0.25 * alpha * (1 - k * k) / (k * k);

    out[i] = I * N *
             ((1 - eta) * (Nu * exp(u + gsl_sf_log_erfc(yu)) + Nv * exp(v + gsl_sf_log_erfc(yv)) +
                           Ns * exp(s + gsl_sf_log_erfc(ys)) + Nr * exp(r + gsl_sf_log_erfc(yr))) -
              eta * 2.0 / M_PI *
                  (Nu * exponentialIntegral(zu).imag() + Nv * exponentialIntegral(zv).imag() +
                   Ns * exponentialIntegral(zs).imag() + Nr * exponentialIntegral(zr).imag()));
  }
}

void IkedaCarpenterPV::functionLocal(double *out, const double *xValues, const size_t nData) const {
  const double I = getParameter("I");
  const double alpha0 = getParameter("Alpha0");
  const double alpha1 = getParameter("Alpha1");
  const double beta0 = getParameter("Beta0");
  const double kappa = getParameter("Kappa");
  const double voigtsigmaSquared = getParameter("SigmaSquared");
  const double voigtgamma = getParameter("Gamma");
  const double X0 = getParameter("X0");

  // cal pseudo voigt sigmaSq and gamma and eta
  double gamma = 1.0; // dummy initialization
  double eta = 0.5;   // dummy initialization
  convertVoigtToPseudo(voigtsigmaSquared, voigtgamma, gamma, eta);
  double sigmaSquared = gamma * gamma / (8.0 * M_LN2); // pseudo voigt sigma^2

  const double beta = 1 / beta0;

  // equations taken from Fullprof manual

  const double k = 0.05;

  // Not entirely sure what to do if sigmaSquared ever negative
  // for now just post a warning
  double someConst = std::numeric_limits<double>::max() / 100.0;
  if (sigmaSquared > 0)
    someConst = 1 / sqrt(2.0 * sigmaSquared);
  else if (sigmaSquared < 0) {
    g_log.warning() << "sigmaSquared negative in functionLocal.\n";
  }

  // update wavelength vector
  calWavelengthAtEachDataPoint(xValues, nData);

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - X0;

    double R = exp(-81.799 / (m_waveLength[i] * m_waveLength[i] * kappa));
    double alpha = 1.0 / (alpha0 + m_waveLength[i] * alpha1);

    double a_minus = alpha * (1 - k);
    double a_plus = alpha * (1 + k);
    double x = a_minus - beta;
    double y = alpha - beta;
    double z = a_plus - beta;

    double Nu = 1 - R * a_minus / x;
    double Nv = 1 - R * a_plus / z;
    double Ns = -2 * (1 - R * alpha / y);
    double Nr = 2 * R * alpha * alpha * beta * k * k / (x * y * z);

    double u = a_minus * (a_minus * sigmaSquared - 2 * diff) / 2.0;
    double v = a_plus * (a_plus * sigmaSquared - 2 * diff) / 2.0;
    double s = alpha * (alpha * sigmaSquared - 2 * diff) / 2.0;
    double r = beta * (beta * sigmaSquared - 2 * diff) / 2.0;

    double yu = (a_minus * sigmaSquared - diff) * someConst;
    double yv = (a_plus * sigmaSquared - diff) * someConst;
    double ys = (alpha * sigmaSquared - diff) * someConst;
    double yr = (beta * sigmaSquared - diff) * someConst;

    std::complex<double> zs = std::complex<double>(-alpha * diff, 0.5 * alpha * gamma);
    std::complex<double> zu = (1 - k) * zs;
    std::complex<double> zv = (1 + k) * zs;
    std::complex<double> zr = std::complex<double>(-beta * diff, 0.5 * beta * gamma);

    double N = 0.25 * alpha * (1 - k * k) / (k * k);

    out[i] = I * N *
             ((1 - eta) * (Nu * exp(u + gsl_sf_log_erfc(yu)) + Nv * exp(v + gsl_sf_log_erfc(yv)) +
                           Ns * exp(s + gsl_sf_log_erfc(ys)) + Nr * exp(r + gsl_sf_log_erfc(yr))) -
              eta * 2.0 / M_PI *
                  (Nu * exponentialIntegral(zu).imag() + Nv * exponentialIntegral(zv).imag() +
                   Ns * exponentialIntegral(zs).imag() + Nr * exponentialIntegral(zr).imag()));
  }
}

void IkedaCarpenterPV::functionDerivLocal(API::Jacobian *jacobian, const double *xValues, const size_t nData) {
  // Parameter order from init():
  // 0:I, 1:Alpha0, 2:Alpha1, 3:Beta0, 4:Kappa, 5:SigmaSquared, 6:Gamma, 7:X0

  const double I = getParameter("I");
  const double alpha0 = getParameter("Alpha0");
  const double alpha1 = getParameter("Alpha1");
  const double beta0 = getParameter("Beta0");
  const double kappa = getParameter("Kappa");
  const double voigtSigmaSq = getParameter("SigmaSquared");
  const double voigtGamma = getParameter("Gamma");
  const double X0 = getParameter("X0");

  // Convert user Voigt params -> pseudo-Voigt H, eta exactly as functionLocal does
  double H = 1.0;
  double eta = 0.5;
  convertVoigtToPseudo(voigtSigmaSq, voigtGamma, H, eta);

  const double eps = std::numeric_limits<double>::epsilon() * 1000.0;
  const double Hsafe = std::max(H, eps);
  const double S = std::max(Hsafe * Hsafe / (8.0 * M_LN2), eps); // pseudo-Voigt sigma^2
  const double beta = 1.0 / beta0;

  const double k = 0.05;
  const double kk = k * k;
  const double twoOverPi = 2.0 / M_PI;
  const double invSqrtPi = 1.0 / std::sqrt(M_PI);

  // Precompute chain derivatives for Voigt -> pseudo-Voigt conversion
  // g = Gaussian FWHM, H = pseudo-Voigt FWHM
  const double g2 = std::max(8.0 * M_LN2 * voigtSigmaSq, eps);
  const double g = std::sqrt(g2);
  const double g3 = g2 * g;
  const double g4 = g2 * g2;

  const double L = voigtGamma;
  const double L2 = L * L;
  const double L3 = L2 * L;
  const double L4 = L2 * L2;

  const double c1 = 2.69269;
  const double c2 = 2.42843;
  const double c3 = 4.47163;
  const double c4 = 0.07842;

  const double P = g4 * g + c1 * g4 * L + c2 * g3 * L2 + c3 * g2 * L3 + c4 * g * L4 + L4 * L;
  const double H4 = std::max(std::pow(Hsafe, 4), eps);

  const double dPdg = 5.0 * g4 + 4.0 * c1 * g3 * L + 3.0 * c2 * g2 * L2 + 2.0 * c3 * g * L3 + c4 * L4;
  const double dPdL = c1 * g4 + 2.0 * c2 * g3 * L + 3.0 * c3 * g2 * L2 + 4.0 * c4 * g * L3 + 5.0 * L4;

  const double dgdV = (4.0 * M_LN2) / g;
  const double dHdV = (dPdg * dgdV) / (5.0 * H4);
  const double dHdL = dPdL / (5.0 * H4);

  const double t = L / Hsafe;
  const double deta_dt = 1.36603 - 0.95438 * t + 0.33348 * t * t;

  const double dtdV = -L * dHdV / (Hsafe * Hsafe);
  const double dtdL = 1.0 / Hsafe - L * dHdL / (Hsafe * Hsafe);

  const double deta_dV = deta_dt * dtdV;
  const double deta_dL = deta_dt * dtdL;

  const double dSdV = (Hsafe * dHdV) / (4.0 * M_LN2);
  const double dSdL = (Hsafe * dHdL) / (4.0 * M_LN2);

  calWavelengthAtEachDataPoint(xValues, nData);

  auto Kval = [](double q, double y) { return std::exp(q + gsl_sf_log_erfc(y)); };

  // Assumes exponentialIntegral(z) in SpecialFunctionSupport is the same scaled
  // complex exponential-integral used in functionLocal.
  auto Fprime = [](const std::complex<double> &z, const std::complex<double> &F) {
    return F - (1.0 / z); // d/dz [ e^z E1(z) ] = e^z E1(z) - 1/z
  };

  for (size_t i = 0; i < nData; ++i) {
    const double lambda = m_waveLength[i];
    const double lambda2 = lambda * lambda;
    const double diff = xValues[i] - X0;

    const double R = std::exp(-81.799 / (lambda2 * kappa));
    const double alpha = 1.0 / (alpha0 + lambda * alpha1);

    const double a_minus = alpha * (1.0 - k);
    const double a_plus = alpha * (1.0 + k);

    const double x = a_minus - beta;
    const double y = alpha - beta;
    const double z = a_plus - beta;

    const double Nu = 1.0 - R * a_minus / x;
    const double Nv = 1.0 - R * a_plus / z;
    const double Ns = -2.0 * (1.0 - R * alpha / y);
    const double Nr = 2.0 * R * alpha * alpha * beta * kk / (x * y * z);

    const double u = 0.5 * a_minus * (a_minus * S - 2.0 * diff);
    const double v = 0.5 * a_plus * (a_plus * S - 2.0 * diff);
    const double s = 0.5 * alpha * (alpha * S - 2.0 * diff);
    const double r = 0.5 * beta * (beta * S - 2.0 * diff);

    const double root2S = std::sqrt(2.0 * S);
    const double invRoot2S = 1.0 / root2S;

    const double yu = (a_minus * S - diff) * invRoot2S;
    const double yv = (a_plus * S - diff) * invRoot2S;
    const double ys = (alpha * S - diff) * invRoot2S;
    const double yr = (beta * S - diff) * invRoot2S;

    const double Ku = Kval(u, yu);
    const double Kv = Kval(v, yv);
    const double Ks = Kval(s, ys);
    const double Kr = Kval(r, yr);

    const std::complex<double> zu(-a_minus * diff, 0.5 * a_minus * Hsafe);
    const std::complex<double> zv(-a_plus * diff, 0.5 * a_plus * Hsafe);
    const std::complex<double> zs(-alpha * diff, 0.5 * alpha * Hsafe);
    const std::complex<double> zr(-beta * diff, 0.5 * beta * Hsafe);

    const std::complex<double> Fu = exponentialIntegral(zu);
    const std::complex<double> Fv = exponentialIntegral(zv);
    const std::complex<double> Fs = exponentialIntegral(zs);
    const std::complex<double> Fr = exponentialIntegral(zr);

    const double Ju = Fu.imag();
    const double Jv = Fv.imag();
    const double Js = Fs.imag();
    const double Jr = Fr.imag();

    const std::complex<double> Fpu = Fprime(zu, Fu);
    const std::complex<double> Fpv = Fprime(zv, Fv);
    const std::complex<double> Fps = Fprime(zs, Fs);
    const std::complex<double> Fpr = Fprime(zr, Fr);

    const double Q = 0.25 * alpha * (1.0 - kk) / kk;

    const double G = Nu * Ku + Nv * Kv + Ns * Ks + Nr * Kr;
    const double Lmix = Nu * Ju + Nv * Jv + Ns * Js + Nr * Jr;

    const double modelCore = (1.0 - eta) * G - twoOverPi * eta * Lmix;

    // Common erfc derivative factor
    const double Cexp = 2.0 * invSqrtPi * std::exp(-(diff * diff) / (2.0 * S));
    const double rootSOver2 = std::sqrt(S / 2.0);

    // ---- Kernel derivatives K(a,S,diff) ----
    auto dK_da = [&](double a, double K) { return (a * S - diff) * K - Cexp * rootSOver2; };
    auto dK_dS = [&](double a, double K) {
      const double denom = 2.0 * S * std::sqrt(2.0 * S);
      return 0.5 * a * a * K - Cexp * (a * S + diff) / denom;
    };
    auto dK_dd = [&](double a, double K) { return -a * K + Cexp * invRoot2S; };

    // ---- Kernel derivatives J(a,H,diff) = imag(exponentialIntegral(z)) ----
    auto dJ_da = [&](const std::complex<double> &Fp) {
      return std::imag(Fp * std::complex<double>(-diff, 0.5 * Hsafe));
    };
    auto dJ_dH = [&](double a, const std::complex<double> &Fp) { return 0.5 * a * std::real(Fp); };
    auto dJ_dd = [&](double a, const std::complex<double> &Fp) { return -a * std::imag(Fp); };

    // ---- Coefficient derivatives wrt alpha, beta, R ----
    const double dNu_dalpha = R * (1.0 - k) * beta / (x * x);
    const double dNv_dalpha = R * (1.0 + k) * beta / (z * z);
    const double dNs_dalpha = -2.0 * R * beta / (y * y);
    const double dNr_dalpha = Nr * (2.0 / alpha - (1.0 - k) / x - 1.0 / y - (1.0 + k) / z);

    const double dNu_dbeta = -R * a_minus / (x * x);
    const double dNv_dbeta = -R * a_plus / (z * z);
    const double dNs_dbeta = 2.0 * R * alpha / (y * y);
    const double dNr_dbeta = Nr * (1.0 / beta + 1.0 / x + 1.0 / y + 1.0 / z);

    const double dNu_dR = -a_minus / x;
    const double dNv_dR = -a_plus / z;
    const double dNs_dR = 2.0 * alpha / y;
    const double dNr_dR = 2.0 * alpha * alpha * beta * kk / (x * y * z);

    // ---- Derivatives of K and J wrt natural variables alpha, beta, S, H, diff ----
    const double dKu_dalpha = (1.0 - k) * dK_da(a_minus, Ku);
    const double dKv_dalpha = (1.0 + k) * dK_da(a_plus, Kv);
    const double dKs_dalpha = dK_da(alpha, Ks);
    const double dKr_dalpha = 0.0;

    const double dKu_dbeta = 0.0;
    const double dKv_dbeta = 0.0;
    const double dKs_dbeta = 0.0;
    const double dKr_dbeta = dK_da(beta, Kr);

    const double dKu_dS = dK_dS(a_minus, Ku);
    const double dKv_dS = dK_dS(a_plus, Kv);
    const double dKs_dS = dK_dS(alpha, Ks);
    const double dKr_dS = dK_dS(beta, Kr);

    const double dKu_dd = dK_dd(a_minus, Ku);
    const double dKv_dd = dK_dd(a_plus, Kv);
    const double dKs_dd = dK_dd(alpha, Ks);
    const double dKr_dd = dK_dd(beta, Kr);

    const double dJu_dalpha = (1.0 - k) * dJ_da(Fpu);
    const double dJv_dalpha = (1.0 + k) * dJ_da(Fpv);
    const double dJs_dalpha = dJ_da(Fps);
    const double dJr_dalpha = 0.0;

    const double dJu_dbeta = 0.0;
    const double dJv_dbeta = 0.0;
    const double dJs_dbeta = 0.0;
    const double dJr_dbeta = dJ_da(Fpr);

    const double dJu_dH = dJ_dH(a_minus, Fpu);
    const double dJv_dH = dJ_dH(a_plus, Fpv);
    const double dJs_dH = dJ_dH(alpha, Fps);
    const double dJr_dH = dJ_dH(beta, Fpr);

    const double dJu_dd = dJ_dd(a_minus, Fpu);
    const double dJv_dd = dJ_dd(a_plus, Fpv);
    const double dJs_dd = dJ_dd(alpha, Fps);
    const double dJr_dd = dJ_dd(beta, Fpr);

    // ---- Mix derivatives wrt natural variables ----
    const double dG_dalpha = dNu_dalpha * Ku + Nu * dKu_dalpha + dNv_dalpha * Kv + Nv * dKv_dalpha + dNs_dalpha * Ks +
                             Ns * dKs_dalpha + dNr_dalpha * Kr + Nr * dKr_dalpha;

    const double dL_dalpha = dNu_dalpha * Ju + Nu * dJu_dalpha + dNv_dalpha * Jv + Nv * dJv_dalpha + dNs_dalpha * Js +
                             Ns * dJs_dalpha + dNr_dalpha * Jr + Nr * dJr_dalpha;

    const double dG_dbeta = dNu_dbeta * Ku + Nu * dKu_dbeta + dNv_dbeta * Kv + Nv * dKv_dbeta + dNs_dbeta * Ks +
                            Ns * dKs_dbeta + dNr_dbeta * Kr + Nr * dKr_dbeta;

    const double dL_dbeta = dNu_dbeta * Ju + Nu * dJu_dbeta + dNv_dbeta * Jv + Nv * dJv_dbeta + dNs_dbeta * Js +
                            Ns * dJs_dbeta + dNr_dbeta * Jr + Nr * dJr_dbeta;

    const double dG_dR = dNu_dR * Ku + dNv_dR * Kv + dNs_dR * Ks + dNr_dR * Kr;

    const double dL_dR = dNu_dR * Ju + dNv_dR * Jv + dNs_dR * Js + dNr_dR * Jr;

    const double dG_dS = Nu * dKu_dS + Nv * dKv_dS + Ns * dKs_dS + Nr * dKr_dS;

    const double dL_dS = 0.0; // J terms depend on H, not S, in this split

    const double dG_dH = 0.0;
    const double dL_dH = Nu * dJu_dH + Nv * dJv_dH + Ns * dJs_dH + Nr * dJr_dH;

    const double dG_dd = Nu * dKu_dd + Nv * dKv_dd + Ns * dKs_dd + Nr * dKr_dd;

    const double dL_dd = Nu * dJu_dd + Nv * dJv_dd + Ns * dJs_dd + Nr * dJr_dd;

    // ---- Derivatives wrt natural variables ----
    const double dQ_dalpha = Q / alpha;

    const double df_dI = Q * modelCore;

    const double df_dalpha = I * (dQ_dalpha * modelCore + Q * ((1.0 - eta) * dG_dalpha - twoOverPi * eta * dL_dalpha));

    const double df_dbeta = I * Q * ((1.0 - eta) * dG_dbeta - twoOverPi * eta * dL_dbeta);

    const double df_dR = I * Q * ((1.0 - eta) * dG_dR - twoOverPi * eta * dL_dR);

    const double df_dS = I * Q * ((1.0 - eta) * dG_dS - twoOverPi * eta * dL_dS);

    const double df_dH = I * Q * ((1.0 - eta) * dG_dH - twoOverPi * eta * dL_dH);

    const double df_deta = -I * Q * (G + twoOverPi * Lmix);

    const double df_ddiff = I * Q * ((1.0 - eta) * dG_dd - twoOverPi * eta * dL_dd);

    // ---- Chain rule back to Mantid fit parameters ----
    const double dalpha_dAlpha0 = -alpha * alpha;
    const double dalpha_dAlpha1 = -lambda * alpha * alpha;
    const double dbeta_dBeta0 = -beta * beta;
    const double dR_dKappa = R * 81.799 / (lambda2 * kappa * kappa);

    const double df_dAlpha0 = df_dalpha * dalpha_dAlpha0;
    const double df_dAlpha1 = df_dalpha * dalpha_dAlpha1;
    const double df_dBeta0 = df_dbeta * dbeta_dBeta0;
    const double df_dKappa = df_dR * dR_dKappa;

    const double df_dSigmaSquared = df_dS * dSdV + df_dH * dHdV + df_deta * deta_dV;

    const double df_dGamma = df_dS * dSdL + df_dH * dHdL + df_deta * deta_dL;

    const double df_dX0 = -df_ddiff;

    jacobian->set(i, 0, df_dI);
    jacobian->set(i, 1, df_dAlpha0);
    jacobian->set(i, 2, df_dAlpha1);
    jacobian->set(i, 3, df_dBeta0);
    jacobian->set(i, 4, df_dKappa);
    jacobian->set(i, 5, df_dSigmaSquared);
    jacobian->set(i, 6, df_dGamma);
    jacobian->set(i, 7, df_dX0);
  }
}

void IkedaCarpenterPV::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian) {
  const auto *domain1D = dynamic_cast<const API::FunctionDomain1D *>(&domain);
  if (!domain1D) {
    calNumericalDeriv(domain, jacobian);
    return;
  }
  functionDerivLocal(&jacobian, domain1D->getPointerAt(0), domain1D->size());
}

/// Returns the integral intensity of the peak
double IkedaCarpenterPV::intensity() const {
  auto interval = getDomainInterval(1e-2);

  API::PeakFunctionIntegrator integrator;
  API::IntegrationResult result = integrator.integrate(*this, interval.first, interval.second);

  return result.result;
}

void IkedaCarpenterPV::setMatrixWorkspace(std::shared_ptr<const API::MatrixWorkspace> workspace, size_t wi,
                                          double startX, double endX) {
  IFunctionMW::setMatrixWorkspace(workspace, wi, startX, endX);

  // Invalidate cached wavelengths so they are recalculated on the next
  // function evaluation using the newly-set workspace and workspace index.
  m_waveLength.clear();

  if (workspace) {
    // Scale all time-dependent IC parameters so the function evaluates
    // correctly in non-TOF workspaces (e.g. d-spacing, wavelength).
    //
    // scaleFactor = X0_workspace / X0_tof  (approx d/tof = 1/DIFC for d-spacing)
    //
    // Only scale when the workspace is not already in TOF and X0 has been set.
    const auto peakCentre = getParameter("X0");
    if (peakCentre != 0.0) {
      auto tof = Mantid::Kernel::UnitFactory::Instance().create("TOF");
      const auto tofCentre = convertValue(peakCentre, tof, workspace, wi);
      if (tofCentre != 0.0) {
        const double scaleFactor = peakCentre / tofCentre;
        if (std::abs(scaleFactor - 1.0) > 1e-6) {
          const auto scaleIfDefault = [&](const std::string &name, const double factor) {
            const size_t idx = parameterIndex(name);
            if (!isExplicitlySet(idx))
              setParameter(idx, getParameter(idx) * factor, false);
          };
          scaleIfDefault("Alpha0", scaleFactor);
          scaleIfDefault("Alpha1", scaleFactor);
          scaleIfDefault("Beta0", scaleFactor);
          scaleIfDefault("SigmaSquared", scaleFactor * scaleFactor);
          scaleIfDefault("Gamma", scaleFactor);
        }
      }
    }
  }
}

} // namespace Mantid::CurveFitting::Functions
