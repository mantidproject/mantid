//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IkedaCarpenterPV.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/SpecialFunctionSupport.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/UnitFactory.h"
#include <cmath>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>
#include <limits>
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Component.h"
#include "MantidGeometry/Instrument/DetectorGroup.h"
#include "MantidGeometry/Instrument/FitParameter.h"
#include <limits>

namespace Mantid {
namespace CurveFitting {

namespace {
/// static logger
Kernel::Logger g_log("IkedaCarpenterPV");
}

using namespace Kernel;
using namespace SpecialFunctionSupport;
using namespace Geometry;

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
    g_log.debug()
        << "SigmaSquared NEGATIVE!.\n"
        << "Likely due to a fit not converging properly\n"
        << "If this is frequent problem please report to Mantid team.\n"
        << "For now to calculate width force SigmaSquared positive.\n";
    sigmaSquared = -sigmaSquared;
  }
  if (gamma < 0) {
    g_log.debug()
        << "Gamma NEGATIVE!.\n"
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
  declareParameter("I", 0.0, "The integrated intensity of the peak. I.e. "
                             "approximately equal to HWHM times height of "
                             "peak");
  declareParameter("Alpha0", 1.6, "Used to model fast decay constant");
  declareParameter("Alpha1", 1.5, "Used to model fast decay constant");
  declareParameter("Beta0", 31.9, "Inverse of slow decay constant");
  declareParameter("Kappa", 46.0, "Controls contribution of slow decay term");
  declareParameter("SigmaSquared", 1.0,
                   "standard deviation squared (Voigt Guassian broadening)");
  declareParameter("Gamma", 1.0, "Voigt Lorentzian broadening");
  declareParameter("X0", 0.0, "Peak position");
}

/** Method for updating m_waveLength.
 *  If size of m_waveLength is equal to number of data (for a new instance of
 *this
 *  class this vector is empty initially) then don't recalculate it.
 *
 *  @param xValues :: x values
 *  @param nData :: length of xValues
 */
void IkedaCarpenterPV::calWavelengthAtEachDataPoint(const double *xValues,
                                                    const size_t &nData) const {
  // if wavelength vector already have the right size no need for resizing it
  // further we make the assumption that no need to recalculate this vector if
  // it already has the right size

  if (m_waveLength.size() != nData) {
    m_waveLength.resize(nData);

    Mantid::Kernel::Unit_sptr wavelength =
        Mantid::Kernel::UnitFactory::Instance().create("Wavelength");
    for (size_t i = 0; i < nData; i++) {
      m_waveLength[i] = xValues[i];
    }

    // note if a version of convertValue was added which allows a double* as
    // first argument
    // then could avoid copying above plus only have to resize m_wavelength when
    // its size smaller than nData
    API::MatrixWorkspace_const_sptr mws = getMatrixWorkspace();
    if (mws) {
      API::MatrixWorkspace_const_sptr mws = getMatrixWorkspace();
      Instrument_const_sptr instrument = mws->getInstrument();
      Geometry::IComponent_const_sptr sample = instrument->getSample();
      if (sample != NULL) {
        convertValue(m_waveLength, wavelength, mws, m_workspaceIndex);
      } else {
        g_log.warning()
            << "No sample set for instrument in workspace.\n"
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
void IkedaCarpenterPV::convertVoigtToPseudo(const double &voigtSigmaSq,
                                            const double &voigtGamma, double &H,
                                            double &eta) const {
  double fwhmGsq = 8.0 * M_LN2 * voigtSigmaSq;
  double fwhmG = sqrt(fwhmGsq);
  double fwhmG4 = fwhmGsq * fwhmGsq;
  double fwhmL = voigtGamma;
  double fwhmLsq = voigtGamma * voigtGamma;
  double fwhmL4 = fwhmLsq * fwhmLsq;

  H = pow(fwhmG4 * fwhmG + 2.69269 * fwhmG4 * fwhmL +
              2.42843 * fwhmGsq * fwhmG * fwhmLsq +
              4.47163 * fwhmGsq * fwhmLsq * fwhmL + 0.07842 * fwhmG * fwhmL4 +
              fwhmL4 * fwhmL,
          0.2);

  if (H == 0.0)
    H = std::numeric_limits<double>::epsilon() * 1000.0;

  double tmp = fwhmL / H;

  eta = 1.36603 * tmp - 0.47719 * tmp * tmp + 0.11116 * tmp * tmp * tmp;
}

void IkedaCarpenterPV::constFunction(double *out, const double *xValues,
                                     const int &nData) const {
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

    std::complex<double> zs =
        std::complex<double>(-alpha * diff, 0.5 * alpha * gamma);
    std::complex<double> zu = (1 - k) * zs;
    std::complex<double> zv = (1 - k) * zs;
    std::complex<double> zr =
        std::complex<double>(-beta * diff, 0.5 * beta * gamma);

    double N = 0.25 * alpha * (1 - k * k) / (k * k);

    out[i] = I * N * ((1 - eta) * (Nu * exp(u + gsl_sf_log_erfc(yu)) +
                                   Nv * exp(v + gsl_sf_log_erfc(yv)) +
                                   Ns * exp(s + gsl_sf_log_erfc(ys)) +
                                   Nr * exp(r + gsl_sf_log_erfc(yr))) -
                      eta * 2.0 / M_PI * (Nu * exponentialIntegral(zu).imag() +
                                          Nv * exponentialIntegral(zv).imag() +
                                          Ns * exponentialIntegral(zs).imag() +
                                          Nr * exponentialIntegral(zr).imag()));
  }
}

void IkedaCarpenterPV::functionLocal(double *out, const double *xValues,
                                     const size_t nData) const {
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

    std::complex<double> zs =
        std::complex<double>(-alpha * diff, 0.5 * alpha * gamma);
    std::complex<double> zu = (1 - k) * zs;
    std::complex<double> zv = (1 - k) * zs;
    std::complex<double> zr =
        std::complex<double>(-beta * diff, 0.5 * beta * gamma);

    double N = 0.25 * alpha * (1 - k * k) / (k * k);

    out[i] = I * N * ((1 - eta) * (Nu * exp(u + gsl_sf_log_erfc(yu)) +
                                   Nv * exp(v + gsl_sf_log_erfc(yv)) +
                                   Ns * exp(s + gsl_sf_log_erfc(ys)) +
                                   Nr * exp(r + gsl_sf_log_erfc(yr))) -
                      eta * 2.0 / M_PI * (Nu * exponentialIntegral(zu).imag() +
                                          Nv * exponentialIntegral(zv).imag() +
                                          Ns * exponentialIntegral(zs).imag() +
                                          Nr * exponentialIntegral(zr).imag()));
  }
}

void IkedaCarpenterPV::functionDerivLocal(API::Jacobian *, const double *,
                                          const size_t) {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "functionDerivLocal is not implemented for IkedaCarpenterPV.");
}

void IkedaCarpenterPV::functionDeriv(const API::FunctionDomain &domain,
                                     API::Jacobian &jacobian) {
  calNumericalDeriv(domain, jacobian);
}

} // namespace CurveFitting
} // namespace Mantid
