#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"
#include "MantidAPI/FunctionFactory.h"
#include <gsl/gsl_sf_erf.h>

#define PI 3.14159265358979323846264338327950288419716939937510582


namespace Mantid
{
namespace CurveFitting
{


DECLARE_FUNCTION(ThermalNeutronBk2BkExpConvPV)

// Get a reference to the logger
Mantid::Kernel::Logger& ThermalNeutronBk2BkExpConvPV::g_log = Kernel::Logger::get("ThermalNeutronBk2BkExpConvPV");

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ThermalNeutronBk2BkExpConvPV::ThermalNeutronBk2BkExpConvPV():mHKLSet(false)
{
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
  */
ThermalNeutronBk2BkExpConvPV::~ThermalNeutronBk2BkExpConvPV()
{
}
  
/*
 * Define the fittable parameters
 */
void ThermalNeutronBk2BkExpConvPV::init()
{
    /// Peak height
    declareParameter("Height", 1.0);

    /// Instrument geometry related
    declareParameter("Dtt1", 1.0);
    declareParameter("Dtt2", 1.0);
    declareParameter("Dtt1t", 1.0);
    declareParameter("Dtt2t", 1.0);
    declareParameter("Zero", 0.0);
    declareParameter("Zerot", 0.0);

    /// Peak profile related
    declareParameter("Width", 1.0);
    declareParameter("Tcross", 1.0);
    declareParameter("Alph0",1.6);
    declareParameter("Alph1",1.5);
    declareParameter("Beta0",1.6);
    declareParameter("Beta1",1.5);
    declareParameter("Alph0t",1.6);
    declareParameter("Alph1t",1.5);
    declareParameter("Beta0t",1.6);
    declareParameter("Beta1t",1.5);

    declareParameter("Sig0", 1.0);
    declareParameter("Sig1", 1.0);
    declareParameter("Sig2", 1.0);

    declareParameter("Gam0", 0.0);
    declareParameter("Gam1", 0.0);
    declareParameter("Gam2", 0.0);

    /// Lattice parameter
    declareParameter("LatticeConstant", 10.0);

    return;
}

/*
 * Set Miller Indices for this peak
 */
void ThermalNeutronBk2BkExpConvPV::setMillerIndex(int h, int k, int l)
{
    if (mHKLSet)
    {
        g_log.error() << "ThermalNeutronBk2BkExpConvPV Peak cannot have (HKL) reset.";
        throw std::logic_error("ThermalNeutronBk2BkExpConvPV Peak cannot have (HKL) reset.");
    }
    else
    {
        mHKLSet = true;
    }

    mH = h;
    mK = k;
    mL = l;

    if (mH*mH + mK*mK + mL*mL < 1.0E-8)
    {
        g_log.error() << "H = K = L = 0 is not allowed" << std::endl;
        throw std::invalid_argument("H=K=L=0 is not allowed for a peak.");
    }

    return;
}

/*
 * Override function1D
 */
void ThermalNeutronBk2BkExpConvPV::functionLocal(double* out, const double* xValues, size_t nData) const
{
    // 1. Get parameters (class)
    double alph0 = getParameter("Alph0");
    double alph1 = getParameter("Alph1");
    double beta0 = getParameter("Beta0");
    double beta1 = getParameter("Beta1");
    double alph0t = getParameter("Alph0t");
    double alph1t = getParameter("Alph1t");
    double beta0t = getParameter("Beta0t");
    double beta1t = getParameter("Beta1t");
    double dtt1 = getParameter("Dtt1");
    double dtt1t = getParameter("Dtt1t");
    double dtt2t = getParameter("Dtt2t");
    double zero = getParameter("Zero");
    double zerot = getParameter("Zerot");
    double sig0 = getParameter("Sig0");
    double sig1 = getParameter("Sig1");
    double sig2 = getParameter("Sig2");
    double gam0 = getParameter("Gam0");
    double gam1 = getParameter("Gam1");
    double gam2 = getParameter("Gam2");
    double wcross = getParameter("Width");
    double Tcross = getParameter("Tcross");
    double latticeconstant = getParameter("LatticeConstant");
    double height = getParameter("Height");

    // 2. Calcualte Peak Position d-spacing and TOF
    double dh = calCubicDSpace(latticeconstant, mH, mK, mL);

    // a) Calculate all the parameters
    double alpha, beta, tof_h, sigma2, gamma;

    // i. Start to calculate alpha, beta, sigma2, gamma,
    double n = 0.5*gsl_sf_erfc(wcross*(Tcross-1/dh));

    double alpha_e = alph0 + alph1*dh;
    double alpha_t = alph0t - alph1t/dh;
    alpha = 1/(n*alpha_e + (1-n)*alpha_t);

    double beta_e = beta0 + beta1*dh;
    double beta_t = beta0t - beta1t/dh;
    beta = 1/(n*beta_e + (1-n)*beta_t);

    double Th_e = zero + dtt1*dh;
    double Th_t = zerot + dtt1t*dh - dtt2t/dh;
    tof_h = n*Th_e + (1-n)*Th_t;

    sigma2 = sig0 + sig1*std::pow(dh, 2) + sig2*std::pow(dh, 4);
    gamma = gam0 + gam1*dh + gam2*std::pow(dh, 2);

    // 3. Calcualte H for the peak
    double H, eta;
    calHandEta(sigma2, gamma, H, eta);

    // 4. Calcualte peak value
    double invert_sqrt2sigma = 1.0/sqrt(2.0*sigma2);
    double N = alpha*beta*0.5/(alpha+beta);

    for (size_t id = 0; id < nData; ++id)
    {
      double dT = xValues[id]-tof_h;
      double omega = calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma);
      out[id] = height*omega;
    }

    return;
}

/*
 * Calculate d = a/sqrt(h**2+k**2+l**2)
 */
double ThermalNeutronBk2BkExpConvPV::calCubicDSpace(double a, int h, int k, int l) const
{
    // TODO This function will be refactored in future.
    double hklfactor = sqrt(double(h*h)+double(k*k)+double(l*l));
    double d = a/hklfactor;
    g_log.debug() << "DB143 a = " << a << " (HKL) = " << h << ", " << k << ", " << l << ": d = " << d << std::endl;

    return d;
}

/*
 * Calcualte H and eta for the peak
 */
void ThermalNeutronBk2BkExpConvPV::calHandEta(double sigma2, double gamma, double& H, double& eta) const
{
  // 1. Calculate H
  double H_G = sqrt(8.0 * sigma2 * log(2.0));
  double H_L = gamma;

  double temp1 = std::pow(H_L, 5) + 0.07842*H_G*std::pow(H_L, 4) + 4.47163*std::pow(H_G, 2)*std::pow(H_L, 3) +
      2.42843*std::pow(H_G, 3)*std::pow(H_L, 2) + 2.69269*std::pow(H_G, 4)*H_L + std::pow(H_G, 5);

  H = std::pow(temp1, 0.2);

  // 2. Calculate eta
  double gam_pv = H_L/H;
  eta = 1.36603 * gam_pv - 0.47719 * std::pow(gam_pv, 2) + 0.11116 * std::pow(gam_pv, 3);

  if (eta > 1 || eta < 0)
  {
    g_log.error() << "Calculated eta = " << eta << " is out of range [0, 1]." << std::endl;
  }

  return;
}

void ThermalNeutronBk2BkExpConvPV::functionDerivLocal(API::Jacobian* , const double* , const size_t )
{
  throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for IkedaCarpenterPV.");
}

/*
 * Calculate derivative of this peak function
 */
void ThermalNeutronBk2BkExpConvPV::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain, jacobian);
}

/*
 * Get the center of the peak
 */
double ThermalNeutronBk2BkExpConvPV::centre()const
{

    double tof_h = calPeakCenter();

    return tof_h;
}

/*
 * Set peak center.  Not allowed
 */
void ThermalNeutronBk2BkExpConvPV::setCentre(const double c)
{
    UNUSED_ARG(c);
    throw std::invalid_argument("ThermalNuetronBk2BkExpConvPV: do not allow to set peak's centre");
}

/*
 * Set peak height
 */
void ThermalNeutronBk2BkExpConvPV::setHeight(const double h)
{
  setParameter("Height", h);

  return;
}

/*
 * Get peak's height
 */
double ThermalNeutronBk2BkExpConvPV::height() const
{
  double height = this->getParameter("Height");
  return height;
}

/*
 * Get peak's FWHM
 */
double ThermalNeutronBk2BkExpConvPV::fwhm() const
{
    // 1. Get peak parameter
    double sig0 = getParameter("Sig0");
    double sig1 = getParameter("Sig1");
    double sig2 = getParameter("Sig2");
    double gam0 = getParameter("Gam0");
    double gam1 = getParameter("Gam1");
    double gam2 = getParameter("Gam2");
    double latticeconstant = getParameter("LatticeConstant");

    // 2. Calcualte d-space of the peak center
    double dh = calCubicDSpace(latticeconstant, mH, mK, mL);

    // 3. Calculate Sigma2 and Gamma and thus H & Eta
    double H, eta;
    double sigma2 = sig0 + sig1*std::pow(dh, 2) + sig2*std::pow(dh, 4);
    double gamma = gam0 + gam1*dh + gam2*std::pow(dh, 2);

    calHandEta(sigma2, gamma, H, eta);

    return H;
}

/*
 * Set peak's FWHM
 */
void ThermalNeutronBk2BkExpConvPV::setFwhm(const double w)
{
  UNUSED_ARG(w);
  throw std::invalid_argument("Unable to set FWHM");
}

/*
 * Calculate peak's center
 */
double ThermalNeutronBk2BkExpConvPV::calPeakCenter() const
{
    // 1. Get parameters
    double latticeconstant = getParameter("LatticeConstant");
    double wcross = getParameter("Width");
    double Tcross = getParameter("Tcross");
    double zero = getParameter("Zero");
    double dtt1 = getParameter("Dtt1");
    double zerot = getParameter("Zerot");
    double dtt1t = getParameter("Dtt1t");
    double dtt2t = getParameter("Dtt2t");

    // 2. Calcualte center of d-space
    double dh = calCubicDSpace(latticeconstant, mH, mK, mL);

    double n = 0.5*gsl_sf_erfc(wcross*(Tcross-1/dh));

    double Th_e = zero + dtt1*dh;
    double Th_t = zerot + dtt1t*dh - dtt2t/dh;
    double tof_h = n*Th_e + (1-n)*Th_t;

    return tof_h;
}


/*
 * Calculate Omega(x) = ... ...
 * This is the core component to calcualte peak profile
 */
double ThermalNeutronBk2BkExpConvPV::calOmega(double x, double eta, double N, double alpha, double beta, double H,
    double sigma2, double invert_sqrt2sigma) const
{
  // 1. Prepare
  std::complex<double> p(alpha*x, alpha*H*0.5);
  std::complex<double> q(-beta*x, beta*H*0.5);

  double u = 0.5*alpha*(alpha*sigma2+2*x);
  double y = (alpha*sigma2 + x)*invert_sqrt2sigma;

  double v = 0.5*beta*(beta*sigma2 - 2*x);
  double z = (beta*sigma2 - x)*invert_sqrt2sigma;

  // 2. Calculate
  double omega1 = (1-eta)*N*(exp(u)*gsl_sf_erfc(y) + std::exp(v)*gsl_sf_erfc(z));
  double omega2;
  if (eta < 1.0E-8)
  {
    omega2 = 0.0;
  }
  else
  {
    omega2 = 2*N*eta/PI*(imag(exp(p)*E1(p)) + imag(exp(q)*E1(q)));
  }
  double omega = omega1+omega2;

  return omega;
}

/*
 * Implementation of complex integral E_1
 */
std::complex<double> ThermalNeutronBk2BkExpConvPV::E1(std::complex<double> z) const
{
    throw std::runtime_error("E1() has not been implemented yet!");
    return z;
}

} // namespace CurveFitting
} // namespace Mantid
