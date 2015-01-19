#include "MantidCurveFitting/NeutronBk2BkExpConvPVoigt.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MultiThreaded.h"

#include "MantidKernel/ConfigService.h"

#include <gsl/gsl_sf_erf.h>
#include <boost/lexical_cast.hpp>

const double PI = 3.14159265358979323846264338327950288419716939937510582;
const double PEAKRANGE = 5.0;
const double TWO_OVER_PI = 2. / PI;
const double NEG_DBL_MAX = -1. * DBL_MAX;

using namespace std;

namespace Mantid {
namespace CurveFitting {
namespace {
/// static logger
Kernel::Logger g_log("NeutronBk2BkExpConvPV");
}

DECLARE_FUNCTION(NeutronBk2BkExpConvPVoigt)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
NeutronBk2BkExpConvPVoigt::NeutronBk2BkExpConvPVoigt() { mHKLSet = false; }

//----------------------------------------------------------------------------------------------
/** Destructor
 */
NeutronBk2BkExpConvPVoigt::~NeutronBk2BkExpConvPVoigt() {}

/// Default value for the peak radius
int NeutronBk2BkExpConvPVoigt::s_peakRadius = 5;

//----------------------------------------------------------------------------------------------
/** Define the fittable parameters
 * Notice that Sig0, Sig1 and Sig2 are NOT the squared value recorded in
 * Fullprof
 */
void NeutronBk2BkExpConvPVoigt::init() {
  // Peak height (0)
  declareParameter("Height", 1.0, "Intensity of peak");

  // Instrument geometry related (1 ~ 3)
  declareParameter(
      "Dtt1", 1.0,
      "coefficient 1 for d-spacing calculation for epithermal neutron part");
  declareParameter(
      "Dtt2", 1.0,
      "coefficient 2 for d-spacing calculation for epithermal neutron part");
  declareParameter("Zero", 0.0, "Zero shift for epithermal neutron");

  // Peak profile related (4 ~ 7) Back to back Expoential
  declareParameter(
      "Alph0", 1.6,
      "exponential constant for rising part of epithermal neutron pulse");
  declareParameter(
      "Alph1", 1.5,
      "exponential constant for rising part of expithermal neutron pulse");
  declareParameter(
      "Beta0", 1.6,
      "exponential constant of decaying part of epithermal neutron pulse");
  declareParameter(
      "Beta1", 1.5,
      "exponential constant of decaying part of epithermal neutron pulse");

  // Pseudo-Voigt (8 ~ 13)
  declareParameter("Sig0", 1.0, "variance parameter 1 of the Gaussian "
                                "component of the psuedovoigt function");
  declareParameter("Sig1", 1.0, "variance parameter 2 of the Gaussian "
                                "component of the psuedovoigt function");
  declareParameter("Sig2", 1.0, "variance parameter 3 of the Gaussian "
                                "component of the psuedovoigt function");

  declareParameter("Gam0", 0.0, "FWHM parameter 1 of the Lorentzian component "
                                "of the psuedovoigt function");
  declareParameter("Gam1", 0.0, "FWHM parameter 2 of the Lorentzian component "
                                "of the psuedovoigt function");
  declareParameter("Gam2", 0.0, "FWHM parameter 3 of the Lorentzian component "
                                "of the psuedovoigt function");

  // Lattice parameter (14)
  declareParameter("LatticeConstant", 10.0, "lattice constant for the sample");

  declareParameter("H", 0.0, "Miller index H. ");
  declareParameter("K", 0.0, "Miller index K. ");
  declareParameter("L", 0.0, "Miller index L. ");

  // Some build-in constant
  LATTICEINDEX = 14;
  HEIGHTINDEX = 0;

  // Unit cell
  m_unitCellSize = -DBL_MAX;

  // Set flag
  m_cellParamValueChanged = true;

  return;
}

//----------------------------------------------------------------------------------------------
/** Get peak parameters stored locally
 * Get some internal parameters values including
 * (a) Alpha, (b) Beta, (c) Gamma, (d) Sigma2
 * Exception: if the peak profile parameter is not in this peak, then
 *            return an Empty_DBL
 */
double NeutronBk2BkExpConvPVoigt::getPeakParameter(std::string paramname) {
  // Calculate peak parameters if required
  if (m_hasNewParameterValue) {
    calculateParameters(false);
  }

  // Get value
  double paramvalue(EMPTY_DBL());

  if (paramname.compare("Alpha") == 0)
    paramvalue = m_Alpha;
  else if (paramname.compare("Beta") == 0)
    paramvalue = m_Beta;
  else if (paramname.compare("Sigma2") == 0)
    paramvalue = m_Sigma2;
  else if (paramname.compare("Gamma") == 0)
    paramvalue = m_Gamma;
  else if (paramname.compare("d_h") == 0)
    paramvalue = m_dcentre;
  else if (paramname.compare("Eta") == 0)
    paramvalue = m_eta;
  else if (paramname.compare("TOF_h") == 0)
    paramvalue = m_centre;
  else if (paramname.compare("FWHM") == 0)
    paramvalue = m_fwhm;
  else {
    stringstream errss;
    errss << "Parameter " << paramname << " does not exist in peak function "
          << this->name() << "'s calculated parameters. "
          << "Candidates are Alpha, Beta, Sigma2, Gamma, d_h and FWHM. ";
    g_log.error(errss.str());
    throw runtime_error(errss.str());
  }

  return paramvalue;
}

//----------------------------------------------------------------------------------------------
/** Calculate peak parameters (fundamential Back-to-back PV),including
* alpha, beta, sigma^2, eta, H
*/
void NeutronBk2BkExpConvPVoigt::calculateParameters(bool explicitoutput) const {
  // Obtain parameters (class) with pre-set order
  double dtt1 = getParameter(1);
  double dtt2 = getParameter(2);
  double zero = getParameter(3);

  double alph0 = getParameter(4);
  double alph1 = getParameter(5);
  double beta0 = getParameter(6);
  double beta1 = getParameter(7);

  double sig0 = getParameter(8);
  double sig1 = getParameter(9);
  double sig2 = getParameter(10);
  double gam0 = getParameter(11);
  double gam1 = getParameter(12);
  double gam2 = getParameter(13);

  double latticeconstant = getParameter(LATTICEINDEX);

  if (!mHKLSet) {
    // Miller index can only be set once.  Either via parameters or
    // client-called setMillerIndex
    double h = getParameter(15);
    double k = getParameter(16);
    double l = getParameter(17);
    mH = static_cast<int>(h);
    mK = static_cast<int>(k);
    mL = static_cast<int>(l);

    // Check value valid or not
    if (mH * mH + mK * mK + mL * mL < 1.0E-8) {
      stringstream errmsg;
      errmsg << "H = K = L = 0 is not allowed";
      g_log.error(errmsg.str());
      throw std::invalid_argument(errmsg.str());
    }

    g_log.debug() << "Set (HKL) from input parameter = " << mH << ", " << mK
                  << ", " << mL << "\n";

    mHKLSet = true;
  }

  double dh, tof_h, eta, alpha, beta, H, sigma2, gamma, N;

  // Calcualte Peak Position d-spacing if necessary
  if (m_cellParamValueChanged) {
    // TODO : make it more general to all type of lattice structure
    m_unitCell.set(latticeconstant, latticeconstant, latticeconstant, 90.0,
                   90.0, 90.0);
    dh = m_unitCell.d(mH, mK, mL);
    m_dcentre = dh;
    m_cellParamValueChanged = false;
  } else {
    dh = m_dcentre;
  }

  // Calculate all the parameters
  /*
    alpha(d) = alpha0 + alpha1/d_h
    beta(d)  = beta0 + beta1/d_h^4
    tof(d)   = zero + Dtt1*d_h + Dtt2*d_h^2
    */

  alpha = alph0 + alph1 / dh;
  beta = beta0 + beta1 / pow(dh, 4.);
  tof_h = zero + dtt1 * dh + dtt2 * dh * dh;

  sigma2 = sig0 * sig0 + sig1 * sig1 * std::pow(dh, 2) +
           sig2 * sig2 * std::pow(dh, 4);
  gamma = gam0 + gam1 * dh + gam2 * std::pow(dh, 2);

  g_log.debug() << "[F001] TOF_h = " << tof_h << ", Alpha = " << alpha
                << ", Beta = " << beta << ", Gamma = " << gamma
                << "(Gam-0 = " << gam0 << ", Gam-1 = " << gam1
                << ", Gam-2 = " << gam2 << ")."
                << "\n";

  // Calcualte H for the peak
  calHandEta(sigma2, gamma, H, eta);

  N = alpha * beta * 0.5 / (alpha + beta);

  // Record recent value
  m_Alpha = alpha;
  m_Beta = beta;
  m_Sigma2 = sigma2;
  m_Gamma = gamma;
  m_fwhm = H;
  m_centre = tof_h;
  m_N = N;
  m_eta = eta;

  // Check whether all the parameters are physical
  if (alpha != alpha || beta != beta || sigma2 != sigma2 || gamma != gamma ||
      H != H || H <= 0.) {
    m_parameterValid = false;
  } else {
    m_parameterValid = true;
  }

  // Debug output
  if (explicitoutput) {
    stringstream msgss;
    msgss << " dh = " << dh << "; TOF = " << tof_h << ", "
          << "alpha = " << alpha << ", beta = " << beta;
    g_log.information(msgss.str());
  }

  // Reset the flag
  m_hasNewParameterValue = false;

  return;
}

//----------------------------------------------------------------------------------------------
/** Override setting parameter by parameter index
  */
void NeutronBk2BkExpConvPVoigt::setParameter(size_t i, const double &value,
                                             bool explicitlySet) {
  if (i == LATTICEINDEX) {
    // Lattice parameter
    if (fabs(m_unitCellSize - value) > 1.0E-8) {
      // If change in value is non-trivial
      m_cellParamValueChanged = true;
      ParamFunction::setParameter(i, value, explicitlySet);
      m_hasNewParameterValue = true;
      m_unitCellSize = value;
    }
  } else {
    // Non lattice parameter
    ParamFunction::setParameter(i, value, explicitlySet);
    m_hasNewParameterValue = true;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Overriding setting parameter by parameter name
  */
void NeutronBk2BkExpConvPVoigt::setParameter(const std::string &name,
                                             const double &value,
                                             bool explicitlySet) {
  if (name.compare("LatticeConstant") == 0) {
    // Lattice parameter
    if (fabs(m_unitCellSize - value) > 1.0E-8) {
      // If change in value is non-trivial
      m_cellParamValueChanged = true;
      ParamFunction::setParameter(LATTICEINDEX, value, explicitlySet);
      m_hasNewParameterValue = true;
      m_unitCellSize = value;
    }
  } else {
    ParamFunction::setParameter(name, value, explicitlySet);
    m_hasNewParameterValue = true;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Function (local) of the vector version
  * @param out: The calculated peak intensities. This is assume to been
 * initialized to the correct length
  * with a value of zero everywhere.
  * @param xValues: The x-values to evaluate the peak at.
 */
void NeutronBk2BkExpConvPVoigt::function(vector<double> &out,
                                         const vector<double> &xValues) const {
  // Calculate peak parameters
  if (m_hasNewParameterValue)
    calculateParameters(false);
  else
    g_log.debug("Function() has no new parameters to calculate. ");

  // Get some other parmeters
  const double HEIGHT = getParameter(HEIGHTINDEX);
  const double INVERT_SQRT2SIGMA = 1.0 / sqrt(2.0 * m_Sigma2);

  // Calculate where to start and to end calculating
  const double RANGE = m_fwhm * PEAKRANGE;

  const double LEFT_VALUE = m_centre - RANGE;
  vector<double>::const_iterator iter =
      std::lower_bound(xValues.begin(), xValues.end(), LEFT_VALUE);

  const double RIGHT_VALUE = m_centre + RANGE;
  vector<double>::const_iterator iter_end =
      std::lower_bound(iter, xValues.end(), RIGHT_VALUE);

  // Calcualte
  std::size_t pos(std::distance(xValues.begin(), iter)); // second loop variable
  for (; iter != iter_end; ++iter) {
    out[pos] = HEIGHT * calOmega(*iter - m_centre, m_eta, m_N, m_Alpha, m_Beta,
                                 m_fwhm, m_Sigma2, INVERT_SQRT2SIGMA);
    pos++;
  } // ENDFOR data points

  return;
}

//----------------------------------------------------------------------------------------------
/** virtual function from IFunction
  */
void NeutronBk2BkExpConvPVoigt::function1D(double *out, const double *xValues,
                                           const size_t nData) const {
  // Calculate peak parameters
  if (m_hasNewParameterValue)
    calculateParameters(false);
  else
    g_log.debug("Function() has no new parameters to calculate. ");

  // Get some other parameters
  const double HEIGHT = getParameter(HEIGHTINDEX);
  const double INVERT_SQRT2SIGMA = 1.0 / sqrt(2.0 * m_Sigma2);

  // On each data point
  const double RANGE = m_fwhm * PEAKRANGE;
  g_log.debug() << "[F002] Peak centre = " << m_centre
                << "; Calcualtion Range = " << RANGE << ".\n";

  for (size_t i = 0; i < nData; ++i) {
    if (fabs(xValues[i] - m_centre) < RANGE) {
      // In peak range
      out[i] = HEIGHT * calOmega(xValues[i] - m_centre, m_eta, m_N, m_Alpha,
                                 m_Beta, m_fwhm, m_Sigma2, INVERT_SQRT2SIGMA);
      g_log.debug() << "TOF = " << xValues[i] << " = " << out[i] << "\n";
    } else {
      // Out of peak range
      out[i] = 0.0;
      g_log.debug() << "TOF = " << xValues[i] << " out of calculation range. "
                    << ".\n";
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Calcualte H and eta for the peak
 */
void NeutronBk2BkExpConvPVoigt::calHandEta(double sigma2, double gamma,
                                           double &H, double &eta) const {
  // 1. Calculate H
  double H_G = sqrt(8.0 * sigma2 * log(2.0));
  double H_L = gamma;

  double temp1 = std::pow(H_L, 5) + 0.07842 * H_G * std::pow(H_L, 4) +
                 4.47163 * std::pow(H_G, 2) * std::pow(H_L, 3) +
                 2.42843 * std::pow(H_G, 3) * std::pow(H_L, 2) +
                 2.69269 * std::pow(H_G, 4) * H_L + std::pow(H_G, 5);

  H = std::pow(temp1, 0.2);

  // 2. Calculate eta
  double gam_pv = H_L / H;
  eta = 1.36603 * gam_pv - 0.47719 * std::pow(gam_pv, 2) +
        0.11116 * std::pow(gam_pv, 3);

  if (eta > 1 || eta < 0) {
    g_log.warning() << "Calculated eta = " << eta
                    << " is out of range [0, 1].\n";
  } else {
    g_log.debug() << "[DBx121] Eta = " << eta << "; Gamma = " << gamma << ".\n";
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Calculate Omega(x) = ... ...
* This is the core component to calcualte peak profile
*/
double NeutronBk2BkExpConvPVoigt::calOmega(const double x, const double eta,
                                           const double N, const double alpha,
                                           const double beta, const double H,
                                           const double sigma2,
                                           const double invert_sqrt2sigma,
                                           const bool explicitoutput) const {
  // Transform to variable u, v, y, z
  const double u = 0.5 * alpha * (alpha * sigma2 + 2. * x);
  const double y = (alpha * sigma2 + x) * invert_sqrt2sigma;

  const double v = 0.5 * beta * (beta * sigma2 - 2. * x);
  const double z = (beta * sigma2 - x) * invert_sqrt2sigma;

  // Calculate Gaussian part
  const double erfcy = gsl_sf_erfc(y);
  double part1(0.);
  if (fabs(erfcy) > DBL_MIN)
    part1 = exp(u) * erfcy;

  const double erfcz = gsl_sf_erfc(z);
  double part2(0.);
  if (fabs(erfcz) > DBL_MIN)
    part2 = exp(v) * erfcz;

  const double omega1 = (1. - eta) * N * (part1 + part2);

  // Calculate Lorenzian part
  double omega2(0.);

  g_log.debug() << "Eta = " << eta << "; X = " << x << " N = " << N << ".\n";

  if (eta >= 1.0E-8) {
    const double SQRT_H_5 = sqrt(H) * .5;
    std::complex<double> p(alpha * x, alpha * SQRT_H_5);
    std::complex<double> q(-beta * x, beta * SQRT_H_5);
    double omega2a = imag(exp(p) * Mantid::API::E1(p));
    double omega2b = imag(exp(q) * Mantid::API::E1(q));
    omega2 = -1.0 * N * eta * (omega2a + omega2b) * TWO_OVER_PI;

    g_log.debug() << "Exp(p) = " << exp(p) << ", Exp(q) = " << exp(q) << ".\n";

    if (omega2 != omega2) {
      g_log.debug() << "Omega2 is not physical.  Omega2a = " << omega2a
                    << ", Omega2b = " << omega2b << ", p = " << p.real() << ", "
                    << p.imag() << ".\n";
    } else {
      g_log.debug() << "X = " << x << " is OK. Omega 2 = " << omega2
                    << ", Omega2A = " << omega2a << ", Omega2B = " << omega2b
                    << "\n";
    }
  }
  const double omega = omega1 + omega2;

  if (explicitoutput || omega != omega) {
    if (omega <= NEG_DBL_MAX || omega >= DBL_MAX || omega != omega) {
      stringstream errss;
      errss << "Peak (" << mH << mK << mL << "): TOF = " << m_centre
            << ", dX = " << x << ", (" << x / m_fwhm << " FWHM) ";
      errss << "Omega = " << omega << " is infinity! omega1 = " << omega1
            << ", omega2 = " << omega2 << "\n";
      errss << "  u = " << u << ", v = " << v
            << ", erfc(y) = " << gsl_sf_erfc(y)
            << ", erfc(z) = " << gsl_sf_erfc(z) << "\n";
      errss << "  alpha = " << alpha << ", beta = " << beta
            << " sigma2 = " << sigma2 << ", N = " << N << "\n";
      g_log.warning(errss.str());
    }
  }
  g_log.debug() << "[DB] Final Value of Omega = " << omega << ".\n";

  return omega;
}

} // namespace CurveFitting
} // namespace Mantid
