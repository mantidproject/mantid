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
#include "MantidAPI/Axis.h"
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
  }

  // Use the same Thompson-Cox-Hastings approximation that the peak shape
  // itself uses (via convertVoigtToPseudo) so that the reported FWHM is
  // consistent with the actual profile width.
  double H = 1.0, eta = 0.5;
  convertVoigtToPseudo(sigmaSquared, gamma, H, eta);
  return H;
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
  // Save the wavelength cache so that single-point evaluations (e.g. from
  // height()) do not invalidate the full-spectrum cache used by functionLocal.
  auto savedWaveLength = std::move(m_waveLength);
  m_waveLength.clear();
  functionLocal(out, xValues, static_cast<size_t>(nData));
  m_waveLength = std::move(savedWaveLength);
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
  const double eps = std::numeric_limits<double>::epsilon() * 1000.0;
  double sigmaSquared = std::max(gamma * gamma / (8.0 * M_LN2), eps); // pseudo voigt sigma^2

  const double beta = 1 / beta0;

  // equations taken from Fullprof manual

  const double k = 0.05;

  const double someConst = 1.0 / sqrt(2.0 * sigmaSquared);

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
  // Parameter order: 0:I 1:Alpha0 2:Alpha1 3:Beta0 4:Kappa 5:SigmaSquared 6:Gamma 7:X0
  // For more details on the origin of this derivative see
  // docs/source/fitting/fitfunctions/IkedaCarpenterPV_analytical_derivative.rst

  const double I = getParameter("I");
  const double alpha0 = getParameter("Alpha0");
  const double alpha1 = getParameter("Alpha1");
  const double beta0 = getParameter("Beta0");
  const double kappa = getParameter("Kappa");
  const double voigtSigmaSq = getParameter("SigmaSquared");
  const double voigtGamma = getParameter("Gamma");
  const double X0 = getParameter("X0");

  double H = 1.0, eta = 0.5;
  convertVoigtToPseudo(voigtSigmaSq, voigtGamma, H, eta);

  const double eps = std::numeric_limits<double>::epsilon() * 1000.0;
  const double Hsafe = std::max(H, eps);
  const double Hsafe2 = Hsafe * Hsafe;
  const double S = std::max(Hsafe2 / (8.0 * M_LN2), eps);
  const double beta = 1.0 / beta0;
  const double beta2 = beta * beta;

  constexpr double k = 0.05;
  constexpr double kk = k * k;
  constexpr double one_minus_k = 1.0 - k;
  constexpr double one_plus_k = 1.0 + k;
  const double one_minus_eta = 1.0 - eta;
  constexpr double twoOverPi = 2.0 / M_PI;
  const double etaTwoOverPi = eta * twoOverPi;
  const double halfHsafe = 0.5 * Hsafe;
  const double halfH2 = halfHsafe * halfHsafe;

  const double invRoot2S = 1.0 / std::sqrt(2.0 * S);
  const double rootSOver2 = std::sqrt(S / 2.0);
  const double twoInvSqrtPi = 2.0 / std::sqrt(M_PI);
  const double CexpSFactor = invRoot2S / (2.0 * S); // = 1/(2*S*sqrt(2S)), for dK/dS

  // Voigt -> pseudo-Voigt chain derivatives
  const double g2 = std::max(8.0 * M_LN2 * voigtSigmaSq, eps);
  const double g = std::sqrt(g2);
  const double g3 = g2 * g;
  const double g4 = g2 * g2;

  const double L = voigtGamma;
  const double L2 = L * L;
  const double L3 = L2 * L;
  const double L4 = L2 * L2;

  constexpr double c1 = 2.69269, c2 = 2.42843, c3 = 4.47163, c4 = 0.07842;

  const double H4 = Hsafe2 * Hsafe2;
  const double inv5H4 = 1.0 / (5.0 * std::max(H4, eps));

  const double dPdg = 5.0 * g4 + 4.0 * c1 * g3 * L + 3.0 * c2 * g2 * L2 + 2.0 * c3 * g * L3 + c4 * L4;
  const double dPdL = c1 * g4 + 2.0 * c2 * g3 * L + 3.0 * c3 * g2 * L2 + 4.0 * c4 * g * L3 + 5.0 * L4;

  const double dgdV = (4.0 * M_LN2) / g;
  const double dHdV = dPdg * dgdV * inv5H4;
  const double dHdL = dPdL * inv5H4;

  const double t = L / Hsafe;
  const double deta_dt = 1.36603 - 0.95438 * t + 0.33348 * t * t;
  const double invHsafe2 = 1.0 / Hsafe2;

  const double deta_dV = deta_dt * (-L * dHdV * invHsafe2);
  const double deta_dL = deta_dt * (1.0 / Hsafe - L * dHdL * invHsafe2);

  const double inv4Ln2 = 1.0 / (4.0 * M_LN2);
  const double dSdV = Hsafe * dHdV * inv4Ln2;
  const double dSdL = Hsafe * dHdL * inv4Ln2;

  const double kappa2_inv = 1.0 / (kappa * kappa);

  calWavelengthAtEachDataPoint(xValues, nData);

  for (size_t i = 0; i < nData; ++i) {
    const double lambda = m_waveLength[i];
    const double lambda2 = lambda * lambda;
    const double diff = xValues[i] - X0;
    const double diff2 = diff * diff;

    const double R = std::exp(-81.799 / (lambda2 * kappa));
    const double alpha = 1.0 / (alpha0 + lambda * alpha1);
    const double alpha2 = alpha * alpha;

    const double a_minus = alpha * one_minus_k;
    const double a_plus = alpha * one_plus_k;

    const double x = a_minus - beta;
    const double y = alpha - beta;
    const double z = a_plus - beta;

    const double inv_x = 1.0 / x;
    const double inv_y = 1.0 / y;
    const double inv_z = 1.0 / z;
    const double inv_x2 = inv_x * inv_x;
    const double inv_y2 = inv_y * inv_y;
    const double inv_z2 = inv_z * inv_z;

    const double Nu = 1.0 - R * a_minus * inv_x;
    const double Nv = 1.0 - R * a_plus * inv_z;
    const double Ns = -2.0 * (1.0 - R * alpha * inv_y);
    const double Nr = 2.0 * R * alpha2 * beta * kk * inv_x * inv_y * inv_z;

    // Gaussian kernel values: K = exp(u + log_erfc(y))
    const double amuS_d = a_minus * S - diff; // a_minus*S - diff (reused in dK)
    const double apuS_d = a_plus * S - diff;
    const double alS_d = alpha * S - diff;
    const double beS_d = beta * S - diff;

    const double u = 0.5 * a_minus * (amuS_d - diff);
    const double v = 0.5 * a_plus * (apuS_d - diff);
    const double s = 0.5 * alpha * (alS_d - diff);
    const double r = 0.5 * beta * (beS_d - diff);

    const double yu = amuS_d * invRoot2S;
    const double yv = apuS_d * invRoot2S;
    const double ys = alS_d * invRoot2S;
    const double yr = beS_d * invRoot2S;

    const double Ku = std::exp(u + gsl_sf_log_erfc(yu));
    const double Kv = std::exp(v + gsl_sf_log_erfc(yv));
    const double Ks = std::exp(s + gsl_sf_log_erfc(ys));
    const double Kr = std::exp(r + gsl_sf_log_erfc(yr));

    // Lorentzian kernel values
    const std::complex<double> zu(-a_minus * diff, a_minus * halfHsafe);
    const std::complex<double> zv(-a_plus * diff, a_plus * halfHsafe);
    const std::complex<double> zs(-alpha * diff, alpha * halfHsafe);
    const std::complex<double> zr(-beta * diff, beta * halfHsafe);

    const std::complex<double> Fu = exponentialIntegral(zu);
    const std::complex<double> Fv = exponentialIntegral(zv);
    const std::complex<double> Fs = exponentialIntegral(zs);
    const std::complex<double> Fr = exponentialIntegral(zr);

    const double Ju = Fu.imag();
    const double Jv = Fv.imag();
    const double Js = Fs.imag();
    const double Jr = Fr.imag();

    // F'(z) = F(z) - 1/z, decomposed into real/imag to avoid complex division.
    // For z = (-a*diff, a*halfH): 1/z = (-diff, -halfH) / (a*(diff^2+halfH^2))
    // So F'_r = F_r + diff/(a*d2h2), F'_i = F_i + halfH/(a*d2h2)
    const double inv_d2h2 = 1.0 / (diff2 + halfH2);
    const double d_term = diff * inv_d2h2;      // diff / (diff^2 + halfH^2)
    const double h_term = halfHsafe * inv_d2h2; // halfH / (diff^2 + halfH^2)

    const double Fpu_r = Fu.real() + d_term / a_minus;
    const double Fpu_i = Fu.imag() + h_term / a_minus;
    const double Fpv_r = Fv.real() + d_term / a_plus;
    const double Fpv_i = Fv.imag() + h_term / a_plus;
    const double Fps_r = Fs.real() + d_term / alpha;
    const double Fps_i = Fs.imag() + h_term / alpha;
    const double Fpr_r = Fr.real() + d_term / beta;
    const double Fpr_i = Fr.imag() + h_term / beta;

    // Function value components
    const double Q = 0.25 * alpha * (1.0 - kk) / kk;
    const double IQ = I * Q;

    const double G = Nu * Ku + Nv * Kv + Ns * Ks + Nr * Kr;
    const double Lmix = Nu * Ju + Nv * Jv + Ns * Js + Nr * Jr;
    const double modelCore = one_minus_eta * G - etaTwoOverPi * Lmix;

    // Common Gaussian derivative factor: (2/sqrt(pi)) * exp(-diff^2/(2S))
    const double Cexp = twoInvSqrtPi * std::exp(-diff2 / (2.0 * S));
    const double CexpRS2 = Cexp * rootSOver2; // for dK/da

    // -- Inlined K kernel derivatives --
    // dK/da = (a*S - diff)*K - Cexp*sqrt(S/2)
    const double dKu_da = amuS_d * Ku - CexpRS2;
    const double dKv_da = apuS_d * Kv - CexpRS2;
    const double dKs_da = alS_d * Ks - CexpRS2;
    const double dKr_da = beS_d * Kr - CexpRS2;

    // dK/dS = 0.5*a^2*K - Cexp*(a*S+diff)/(2*S*sqrt(2S))
    const double dKu_dS = 0.5 * a_minus * a_minus * Ku - Cexp * (a_minus * S + diff) * CexpSFactor;
    const double dKv_dS = 0.5 * a_plus * a_plus * Kv - Cexp * (a_plus * S + diff) * CexpSFactor;
    const double dKs_dS = 0.5 * alpha2 * Ks - Cexp * (alpha * S + diff) * CexpSFactor;
    const double dKr_dS = 0.5 * beta2 * Kr - Cexp * (beta * S + diff) * CexpSFactor;

    // dK/dd = -a*K + Cexp/sqrt(2S)
    const double CexpIR2S = Cexp * invRoot2S;
    const double dKu_dd = -a_minus * Ku + CexpIR2S;
    const double dKv_dd = -a_plus * Kv + CexpIR2S;
    const double dKs_dd = -alpha * Ks + CexpIR2S;
    const double dKr_dd = -beta * Kr + CexpIR2S;

    // -- J kernel derivatives --
    // dJ/da = Im(Fp * (-diff, halfH)) = Fp_r*halfH - Fp_i*diff
    const double dJu_da = Fpu_r * halfHsafe - Fpu_i * diff;
    const double dJv_da = Fpv_r * halfHsafe - Fpv_i * diff;
    const double dJs_da = Fps_r * halfHsafe - Fps_i * diff;
    const double dJr_da = Fpr_r * halfHsafe - Fpr_i * diff;

    // dJ/dH = 0.5*a*Re(Fp)
    const double dJu_dH = 0.5 * a_minus * Fpu_r;
    const double dJv_dH = 0.5 * a_plus * Fpv_r;
    const double dJs_dH = 0.5 * alpha * Fps_r;
    const double dJr_dH = 0.5 * beta * Fpr_r;

    // dJ/dd = -a*Im(Fp)
    const double dJu_dd = -a_minus * Fpu_i;
    const double dJv_dd = -a_plus * Fpv_i;
    const double dJs_dd = -alpha * Fps_i;
    const double dJr_dd = -beta * Fpr_i;

    // -- Coefficient derivatives wrt alpha, beta, R --
    const double Rbeta = R * beta;
    const double dNu_dalpha = Rbeta * one_minus_k * inv_x2;
    const double dNv_dalpha = Rbeta * one_plus_k * inv_z2;
    const double dNs_dalpha = -2.0 * Rbeta * inv_y2;
    const double dNr_dalpha = Nr * (2.0 / alpha - one_minus_k * inv_x - inv_y - one_plus_k * inv_z);

    const double dNu_dbeta = -R * a_minus * inv_x2;
    const double dNv_dbeta = -R * a_plus * inv_z2;
    const double dNs_dbeta = 2.0 * R * alpha * inv_y2;
    const double dNr_dbeta = Nr * (1.0 / beta + inv_x + inv_y + inv_z);

    const double dNu_dR = -a_minus * inv_x;
    const double dNv_dR = -a_plus * inv_z;
    const double dNs_dR = 2.0 * alpha * inv_y;
    const double dNr_dR = 2.0 * alpha2 * beta * kk * inv_x * inv_y * inv_z;

    // -- Mix derivatives --
    // dG/dalpha: Kr doesn't depend on alpha, so dKr_dalpha term is zero
    const double dG_dalpha = dNu_dalpha * Ku + Nu * one_minus_k * dKu_da + dNv_dalpha * Kv + Nv * one_plus_k * dKv_da +
                             dNs_dalpha * Ks + Ns * dKs_da + dNr_dalpha * Kr;

    const double dL_dalpha = dNu_dalpha * Ju + Nu * one_minus_k * dJu_da + dNv_dalpha * Jv + Nv * one_plus_k * dJv_da +
                             dNs_dalpha * Js + Ns * dJs_da + dNr_dalpha * Jr;

    // dG/dbeta: Ku,Kv,Ks don't depend on beta, so only Nr*dKr_da survives
    const double dG_dbeta = dNu_dbeta * Ku + dNv_dbeta * Kv + dNs_dbeta * Ks + dNr_dbeta * Kr + Nr * dKr_da;

    const double dL_dbeta = dNu_dbeta * Ju + dNv_dbeta * Jv + dNs_dbeta * Js + dNr_dbeta * Jr + Nr * dJr_da;

    // dG/dR, dL/dR: R doesn't affect kernel arguments, only coefficients
    const double dG_dR = dNu_dR * Ku + dNv_dR * Kv + dNs_dR * Ks + dNr_dR * Kr;
    const double dL_dR = dNu_dR * Ju + dNv_dR * Jv + dNs_dR * Js + dNr_dR * Jr;

    // dG/dS (K depends on S; J does not, so dL/dS = 0)
    const double dG_dS = Nu * dKu_dS + Nv * dKv_dS + Ns * dKs_dS + Nr * dKr_dS;

    // dL/dH (J depends on H; K does not, so dG/dH = 0)
    const double dL_dH = Nu * dJu_dH + Nv * dJv_dH + Ns * dJs_dH + Nr * dJr_dH;

    // dG/dd, dL/dd
    const double dG_dd = Nu * dKu_dd + Nv * dKv_dd + Ns * dKs_dd + Nr * dKr_dd;
    const double dL_dd = Nu * dJu_dd + Nv * dJv_dd + Ns * dJs_dd + Nr * dJr_dd;

    // -- Derivatives wrt natural variables (noting dG_dH=0, dL_dS=0) --
    const double df_dI = Q * modelCore;
    const double df_dalpha = I * (Q / alpha * modelCore + Q * (one_minus_eta * dG_dalpha - etaTwoOverPi * dL_dalpha));
    const double df_dbeta = IQ * (one_minus_eta * dG_dbeta - etaTwoOverPi * dL_dbeta);
    const double df_dR = IQ * (one_minus_eta * dG_dR - etaTwoOverPi * dL_dR);
    const double df_dS = IQ * one_minus_eta * dG_dS; // dL_dS = 0
    const double df_dH = -IQ * etaTwoOverPi * dL_dH; // dG_dH = 0
    const double df_deta = -IQ * (G + twoOverPi * Lmix);
    const double df_ddiff = IQ * (one_minus_eta * dG_dd - etaTwoOverPi * dL_dd);

    // -- Chain rule to Mantid fit parameters --
    const double neg_alpha2 = -alpha2;
    jacobian->set(i, 0, df_dI);
    jacobian->set(i, 1, df_dalpha * neg_alpha2);
    jacobian->set(i, 2, df_dalpha * neg_alpha2 * lambda);
    jacobian->set(i, 3, df_dbeta * (-beta2));
    jacobian->set(i, 4, df_dR * R * 81.799 * kappa2_inv / lambda2);
    jacobian->set(i, 5, df_dS * dSdV + df_dH * dHdV + df_deta * deta_dV);
    jacobian->set(i, 6, df_dS * dSdL + df_dH * dHdL + df_deta * deta_dL);
    jacobian->set(i, 7, -df_ddiff);
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
    // If the instrument definition provided IC parameters the base-class call above handles
    // the formula evaluation and unit conversion.
    if (isExplicitlySet(parameterIndex("Alpha0")))
      return;

    // This next section handles conversion when no instrument defaults are present
    // in such a scenario the generic defaults will be used, which are in TOF units.

    // This will scale these default time-dependent IC parameters so the initial plot guess
    // is reasonable for non-TOF workspaces (e.g. d-spacing, wavelength).

    // Only applies to units with a simple power-law relationship with TOF;
    // units like DeltaE have a non-linear, energy-dependent conversion that
    // may be kinematically forbidden at the peak position.
    const auto wsUnit = workspace->getAxis(0)->unit();
    auto tof = Mantid::Kernel::UnitFactory::Instance().create("TOF");
    double factor, power;
    if (!wsUnit->quickConversion(*tof, factor, power))
      return;

    const auto peakCentre = getParameter("X0");
    if (peakCentre != 0.0) {
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
