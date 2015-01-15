//----------------------------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------------------------
#include "MantidCurveFitting/Voigt.h"

#include "MantidAPI/FunctionFactory.h"

#include <cmath>

namespace Mantid {
namespace CurveFitting {
DECLARE_FUNCTION(Voigt);

namespace {
/// @cond
// Coefficients for approximation using 4 lorentzians
const size_t NLORENTZIANS = 4;

const double COEFFA[NLORENTZIANS] = {-1.2150, -1.3509, -1.2150, -1.3509};
const double COEFFB[NLORENTZIANS] = {1.2359, 0.3786, -1.2359, -0.3786};
const double COEFFC[NLORENTZIANS] = {-0.3085, 0.5906, -0.3085, 0.5906};
const double COEFFD[NLORENTZIANS] = {0.0210, -1.1858, -0.0210, 1.1858};

const char *LORENTZ_AMP = "LorentzAmp";
const char *LORENTZ_POS = "LorentzPos";
const char *LORENTZ_FWHM = "LorentzFWHM";
const char *GAUSSIAN_FWHM = "GaussianFWHM";

const double SQRTLN2 = std::sqrt(std::log(2.0));
const double SQRTPI = std::sqrt(M_PI);
///@endcond
}

/**
 * Declare the active parameters for the function
 */
void Voigt::declareParameters() {
  declareParameter(LORENTZ_AMP, 0.0, "Value of the Lorentzian amplitude");
  declareParameter(LORENTZ_POS, 0.0, "Position of the Lorentzian peak");
  declareParameter(LORENTZ_FWHM, 0.0,
                   "Value of the full-width half-maximum for the Lorentzian");
  declareParameter(GAUSSIAN_FWHM, 0.0,
                   "Value of the full-width half-maximum for the Gaussian");
}

/**
 * Calculate Voigt function for each x value
 * @param out :: The values of the function at each x point
 * @param xValues :: The X values
 * @param nData :: The number of X values to evaluate
 */
void Voigt::functionLocal(double *out, const double *xValues,
                          const size_t nData) const {
  calculateFunctionAndDerivative(xValues, nData, out, NULL);
}

/**
 * Derivatives of function with respect to active parameters
 * @param out :: The Jacobian matrix containing the partial derivatives for each
 * x value
 * @param xValues :: The X values
 * @param nData :: The number of X values to evaluate
 */
void Voigt::functionDerivLocal(API::Jacobian *out, const double *xValues,
                               const size_t nData) {
  calculateFunctionAndDerivative(xValues, nData, NULL, out);
}

/**
 * Calculates both function & derivative together
 * @param xValues :: The X values
 * @param nData :: The number of X values to evaluate
 * @param functionValues :: Calculated y values
 * @param derivatives :: The Jacobian matrix containing the partial derivatives
 * for each x value (allowed null)
 */
void Voigt::calculateFunctionAndDerivative(const double *xValues,
                                           const size_t nData,
                                           double *functionValues,
                                           API::Jacobian *derivatives) const {
  const double a_L = getParameter(LORENTZ_AMP);
  const double lorentzPos = getParameter(LORENTZ_POS);
  const double gamma_L = getParameter(LORENTZ_FWHM);
  const double gamma_G = getParameter(GAUSSIAN_FWHM);

  const double rtln2oGammaG = SQRTLN2 / gamma_G;
  const double prefactor = (a_L * SQRTPI * gamma_L * SQRTLN2 / gamma_G);

  for (size_t i = 0; i < nData; ++i) {
    const double xoffset = xValues[i] - lorentzPos;

    const double X = xoffset * 2.0 * rtln2oGammaG;
    const double Y = gamma_L * rtln2oGammaG;

    double fx(0.0), dFdx(0.0), dFdy(0.0);
    for (size_t j = 0; j < NLORENTZIANS; ++j) {
      const double ymA(Y - COEFFA[j]);
      const double xmB(X - COEFFB[j]);
      const double alpha = COEFFC[j] * ymA + COEFFD[j] * xmB;
      const double beta = ymA * ymA + xmB * xmB;
      const double ratioab = alpha / beta;
      fx += ratioab;
      dFdx += (COEFFD[j] / beta) - 2.0 * (X - COEFFB[j]) * ratioab / beta;
      dFdy += (COEFFC[j] / beta) - 2.0 * (Y - COEFFA[j]) * ratioab / beta;
    }
    if (functionValues) {
      functionValues[i] = prefactor * fx;
    }
    if (derivatives) {
      derivatives->set(i, 0, prefactor * fx / a_L);
      derivatives->set(i, 1, -prefactor * dFdx * 2.0 * rtln2oGammaG);
      derivatives->set(i, 2, prefactor * (fx / gamma_L + dFdy * rtln2oGammaG));
      derivatives->set(
          i, 3,
          -prefactor *
              (fx + (rtln2oGammaG) * (2.0 * xoffset * dFdx + gamma_L * dFdy)) /
              gamma_G);
    }
  }
}

/**
 * Returns the value of the "LorentzPos" parameter.
 * @return value of centre of peak
 */
double Voigt::centre() const { return getParameter(LORENTZ_POS); }

/**
 * Return the value of the "LorentzAmp" parameter
 * @return value of height of peak
 */
double Voigt::height() const { return 2.0 * getParameter(LORENTZ_AMP) / 3.0; }

/**
 * Gives the FWHM of the peak. This is estimated as
 * 0.5*(LorentzFWHM + GaussianFWHM)
 * @return value of FWHM of peak
 */
double Voigt::fwhm() const {
  return (getParameter(LORENTZ_FWHM) + getParameter(GAUSSIAN_FWHM));
}

/**
 * Set the centre of the peak, the LorentzPos parameter
 * @param value :: The new value for the centre of the peak
 */
void Voigt::setCentre(const double value) {
  this->setParameter(LORENTZ_POS, value);
}

/**
 * Set the height of the peak. Sets LorentzAmp parameter to 1.5*value
 * @param value :: The new value for the centre of the peak
 */
void Voigt::setHeight(const double value) {
  this->setParameter(LORENTZ_AMP, 1.5 * value);
}

/**
 * Set the FWHM of the peak
 * @param value :: The new value for the FWHM of the peak
 */
void Voigt::setFwhm(const double value) {
  this->setParameter(LORENTZ_FWHM, 0.5 * value);
  this->setParameter(GAUSSIAN_FWHM, 0.5 * value);
}

} // namespace CurveFitting
} // namespace Mantid
