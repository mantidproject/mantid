#include "MantidCurveFitting/ThermalNeutronBk2BkExpConvPV.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/MultiThreaded.h"
#include <gsl/gsl_sf_erf.h>

#define PI 3.14159265358979323846264338327950288419716939937510582

using namespace std;
using namespace Mantid;
using namespace Mantid::API;


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
ThermalNeutronBk2BkExpConvPV::ThermalNeutronBk2BkExpConvPV():mHKLSet(false),m_cancel(false),m_parallelException(false)
{

}
    
//----------------------------------------------------------------------------------------------
/** Destructor
  */
ThermalNeutronBk2BkExpConvPV::~ThermalNeutronBk2BkExpConvPV()
{
}
  
/** Define the fittable parameters
  * Notice that Sig0, Sig1 and Sig2 are NOT the squared value recorded in Fullprof
 */
void ThermalNeutronBk2BkExpConvPV::init()
{
  // Peak height
  declareParameter("Height", 1.0);

  // Instrument geometry related
  declareParameter("Dtt1", 1.0);
  declareParameter("Dtt2", 1.0);
  declareParameter("Dtt1t", 1.0);
  declareParameter("Dtt2t", 1.0);
  declareParameter("Zero", 0.0);
  declareParameter("Zerot", 0.0);

  // Peak profile related
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

  // Lattice parameter
  declareParameter("LatticeConstant", 10.0);

  // Initialize parameters
  mParameters.insert(std::make_pair("Alpha", 1.0));
  mParameters.insert(std::make_pair("Beta", 1.0));
  mParameters.insert(std::make_pair("Gamma", 1.0));
  mParameters.insert(std::make_pair("Sigma2", 1.0));
  mParameters.insert(std::make_pair("FWHM", 1.0));
  mParameters.insert(make_pair("d_h", 1.0));

  return;
}

//------------- Public Functions (Overwrite) Set/Get ---------------------------------------------



//------------- Public Functions (New) Set/Get ---------------------------------------------------
/** Set Miller Indices for this peak
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
    stringstream errmsg;
    errmsg << "H = K = L = 0 is not allowed";
    g_log.error(errmsg.str());
    throw std::invalid_argument(errmsg.str());
  }

  return;
}

/** Get Miller Index from this peak
 */
void ThermalNeutronBk2BkExpConvPV::getMillerIndex(int& h, int &k, int &l)
{
  h = mH;
  k = mK;
  l = mL;

  return;
}


/** Get peak parameters stored locally
 * Get some internal parameters values including
 * (a) Alpha, (b) Beta, (c) Gamma, (d) Sigma2
 * Exception: if the peak profile parameter is not in this peak, then
 *            return an Empty_DBL
 */
double ThermalNeutronBk2BkExpConvPV::getPeakParameter(std::string paramname)
{
  std::map<std::string, double>::iterator mit;
  mit = mParameters.find(paramname);
  double paramvalue;
  if (mit == mParameters.end())
  {
    g_log.warning() << "Parameter " << paramname << " does not exist in peak profile. " << std::endl;
    paramvalue = Mantid::EMPTY_DBL();
  }
  else
  {
    paramvalue = mit->second;
  }

  return paramvalue;
}

//------------- Public Functions (Overwrite) Calculation ---------------------------------------------
/** Calculate peak parameters (fundamential Back-to-back PV),including
  * alpha, beta, sigma^2, eta, H
  */
void ThermalNeutronBk2BkExpConvPV::calculateParameters(double& dh, double& tof_h, double& eta, double& alpha, double& beta,
                                                       double& H, double &sigma2, double &gamma, double& N, bool explicitoutput) const
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

  // 2. Calcualte Peak Position d-spacing and TOF
  dh = calCubicDSpace(latticeconstant, mH, mK, mL);

  // 3. Calculate all the parameters
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

  sigma2 = sig0*sig0 + sig1*sig1*std::pow(dh, 2) + sig2*sig2*std::pow(dh, 4);
  gamma = gam0 + gam1*dh + gam2*std::pow(dh, 2);

  // 3. Calcualte H for the peak
  calHandEta(sigma2, gamma, H, eta);

  N = alpha*beta*0.5/(alpha+beta);

  // 4. Record recent value
  mParameters["Alpha"] = alpha;
  mParameters["Beta"] = beta;
  mParameters["Sigma2"] = sigma2;
  mParameters["Gamma"] = gamma;
  mParameters["FWHM"] = H;
  mParameters["d_h"] = dh;

  // 5. Debug output
  if (explicitoutput)
  {
    std::cout << "alpha = " << alpha << ", beta = " << beta
              << ", N = " << N << std::endl;
    std::cout << "  n = " << n << ", alpha_e = " << alpha_e << ", alpha_t = " << alpha_t << std::endl;
    std::cout << " dh = " << dh << ", alph0t = " << alph0t << ", alph1t = " << alph1t
              << ", alph0 = " << alph0 << ", alph1 = " << alph1 << std::endl;
    std::cout << "  n = " << n << ", beta_e = " << beta_e << ", beta_t = " << beta_t << std::endl;
    std::cout << " dh = " << dh << ", beta0t = " << beta0t << ", beta1t = " << beta1t << std::endl;
  }

  return;
}

//------------- Private Functions (Overwrite) Calculation ---------------------------------------------

/** Override function1D
 */
void ThermalNeutronBk2BkExpConvPV::functionLocal(double* out, const double* xValues, size_t nData) const
{  
  // 1. Calculate peak parameters
  double height = getParameter("Height");

  double d_h, tof_h, alpha, beta, H, sigma2, eta, N, gamma;
  this->calculateParameters(d_h, tof_h, eta, alpha, beta, H, sigma2, gamma, N, false);

  // cout << "DBx212:  eta = " << eta << ", gamma = " << gamma << endl;

  double invert_sqrt2sigma = 1.0/sqrt(2.0*sigma2);

  // PRAGMA_OMP(parallel for schedule(dynamic, 10))

  // PARALLEL_SET_NUM_THREADS(8);
  // PARALLEL_FOR_NO_WSP_CHECK()
  for (size_t id = 0; id < nData; ++id)
  {
    // PARALLEL_START_INTERUPT_REGION

    // a) Caclualte peak intensity
    double dT = xValues[id]-tof_h;
    double omega = calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma);

    out[id] = height*omega;

    /*  Disabled for parallel checking
    if (!(omega > -DBL_MAX && omega < DBL_MAX))
    {
      // Output with error
      g_log.error() << "Calcuate Peak " << mH << ", " << mK << ", " << mL << " wrong!" << std::endl;
      bool explicitoutput = true;
      calculateParameters(d_h, tof_h, eta, alpha, beta, H, sigma2, gamma, N, explicitoutput);
      calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma, explicitoutput);

      out[id] = DBL_MAX;
    }
    else
    {
      out[id] = height*omega;
    }
    */

    // PARALLEL_END_INTERUPT_REGION
  } // ENDFOR data points
  // PARALLEL_CHECK_INTERUPT_REGION

  return;
}

/** Disabled derivative
  */
void ThermalNeutronBk2BkExpConvPV::functionDerivLocal(API::Jacobian* , const double* , const size_t )
{
  throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for IkedaCarpenterPV.");
}

/** Calculate derivative of this peak function
 */
void ThermalNeutronBk2BkExpConvPV::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain, jacobian);
}

/** Get the center of the peak
 */
double ThermalNeutronBk2BkExpConvPV::centre()const
{
  double dh, tof_h, eta, alpha, beta, H, sigma, gamma, N;
  calculateParameters(dh, tof_h, eta, alpha, beta, H, sigma, gamma, N, false);

  return tof_h;

  // double tof_h = calPeakCenter();
  // return tof_h;
}

/** Set peak center.  Not allowed
 */
void ThermalNeutronBk2BkExpConvPV::setCentre(const double c)
{
  UNUSED_ARG(c);
  throw std::invalid_argument("ThermalNuetronBk2BkExpConvPV: do not allow to set peak's centre");
}

/** Set peak height
 */
void ThermalNeutronBk2BkExpConvPV::setHeight(const double h)
{
  setParameter("Height", h);
  return;
}

/** Get peak's height
 */
double ThermalNeutronBk2BkExpConvPV::height() const
{
  double height = this->getParameter("Height");
  return height;
}

/** Get peak's FWHM
 */
double ThermalNeutronBk2BkExpConvPV::fwhm() const
{
  double dh, tof_h, eta, alpha, beta, H, sigma, gamma, N;

  calculateParameters(dh, tof_h, eta, alpha, beta, H, sigma, gamma, N, false);

  return H;

  /*
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
  double sigma2 = sig0*sig0 + sig1*sig1*std::pow(dh, 2) + sig2*sig2*std::pow(dh, 4);
  double gamma = gam0 + gam1*dh + gam2*std::pow(dh, 2);

  calHandEta(sigma2, gamma, H, eta);

  return H;
  */
}

/** Set peak's FWHM
 */
void ThermalNeutronBk2BkExpConvPV::setFwhm(const double w)
{
  UNUSED_ARG(w);
  throw std::invalid_argument("Unable to set FWHM");
}

/** Calculate peak's center
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

  // std::cout << "Lattice = " << latticeconstant << ", Dtt1 = " << dtt1 << ", Dtt1t = " << dtt1t << std::endl;

  // 2. Calcualte center of d-space
  double dh = calCubicDSpace(latticeconstant, mH, mK, mL);

  double n = 0.5*gsl_sf_erfc(wcross*(Tcross-1/dh));

  double Th_e = zero + dtt1*dh;
  double Th_t = zerot + dtt1t*dh - dtt2t/dh;
  double tof_h = n*Th_e + (1-n)*Th_t;

  if (tof_h < 0)
  {
    g_log.error() << "Peak " << mH << ", " << mH << ", " << mL << ": TOF-h cannot be negative! "
                  << "Lattice = " << latticeconstant << ", d-spacing = " << dh << std::endl
                  << "Zero    = " << zero << ", Zerot = " << zerot << std::endl
                  << "Dtt1    = " << dtt1 << ", Dtt1t = " << dtt1t << ", Dtt2t   = " << dtt2t << std::endl
                  << "Width   = " << wcross << ", Tcross = " << Tcross << ", n = " << n << std::endl;
  }

  return tof_h;
}
*/


//-------------  Private Function To Calculate Peak Profile --------------------------------------------
/** Calcualte H and eta for the peak
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
    g_log.warning() << "Calculated eta = " << eta << " is out of range [0, 1]." << std::endl;
  }

  return;
}


/** Calculate Omega(x) = ... ...
 *  This is the core component to calcualte peak profile
 */
double ThermalNeutronBk2BkExpConvPV::calOmega(double x, double eta, double N, double alpha, double beta, double H,
                                              double sigma2, double invert_sqrt2sigma, bool explicitoutput) const
{
  // 1. Prepare
  std::complex<double> p(alpha*x, alpha*sqrt(H)*0.5);
  std::complex<double> q(-beta*x, beta*sqrt(H)*0.5);

  double u = 0.5*alpha*(alpha*sigma2+2*x);
  double y = (alpha*sigma2 + x)*invert_sqrt2sigma;

  double v = 0.5*beta*(beta*sigma2 - 2*x);
  double z = (beta*sigma2 - x)*invert_sqrt2sigma;

  // 2. Calculate
  double part1, part2;

  double erfcy = gsl_sf_erfc(y);
  if (fabs(erfcy) > DBL_MIN)
    part1 = exp(u)*erfcy;
  else
    part1 = 0.0;

  double erfcz = gsl_sf_erfc(z);
  if (fabs(erfcz) > DBL_MIN)
    part2 = exp(v)*erfcz;
  else
    part2 = 0.0;

  double omega1 = (1-eta)*N*(part1 + part2);
  double omega2, omega2a, omega2b;
  if (eta < 1.0E-8)
  {
    omega2a = 0.0;
    omega2b = 0.0;
    omega2 = 0.0;
  }
  else
  {
    omega2a = imag(exp(p)*E1(p));
    omega2b = imag(exp(q)*E1(q));
    omega2 = 2*N*eta/PI*(omega2a + omega2b);
    // omega2 = 2*N*eta/PI*(imag(exp(p)*E1(p)) + imag(exp(q)*E1(q)));
  }
  double omega = omega1+omega2;

  if (explicitoutput && !(omega > -DBL_MAX && omega < DBL_MAX))
  {
    std::cout << "Find omega = " << omega << " is infinity! omega1 = " << omega1 << ", omega2 = " << omega2 << std::endl;
    std::cout << "  u = " << u << ", v = " << v << ", erfc(y) = " << gsl_sf_erfc(y)
                  << ", erfc(z) = " << gsl_sf_erfc(z) << std::endl;
    std::cout << "  alpha = " << alpha << ", x = " << x << " sigma2 = " << sigma2
                  << ", N = " << N << std::endl;
  }

  /* Debug output to understand E1()
  cout << setw(15) << p.real() << setw(15) << p.imag() << "\t\t" << omega2a << endl;
  cout << setw(15) << p.real() << setw(15) << p.imag() << setw(15) << exp(p).real() << setw(15) << exp(p).imag()
       << setw(15) << E1(p).real() << setw(15) << E1(p).imag() << endl;

  cout << q.real() << "\t\t" << q.imag() << "\t\t" << omega2b << endl;
  cout << setw(15) << q.real() << setw(15) << q.imag() << setw(15) << exp(q).real() << setw(15) << exp(q).imag()
       << setw(15) << E1(q).real() << setw(15) << E1(q).imag() << endl;
  cout << p.real() << "\t\t" << p.imag() << "\t\t" << E1(p).real() << "\t\t" << E1(p).imag() << endl;
  cout << exp(p) << "\t\t" << exp(q) << endl;
  cout << q.real() << "\t\t" << q.imag() << "\t\t" << E1(q).real() << "\t\t" << E1(q).imag() << endl;
  */

  return omega;
}

/** This is called during long-running operations,
 * and check if the algorithm has requested that it be cancelled.
 */
void ThermalNeutronBk2BkExpConvPV::interruption_point() const
{
  // only throw exceptions if the code is not multi threaded otherwise you contravene the OpenMP standard
  // that defines that all loops must complete, and no exception can leave an OpenMP section
  // openmp cancel handling is performed using the ??, ?? and ?? macros in each algrothim
  IF_NOT_PARALLEL
      if (m_cancel) throw Algorithm::CancelException();
}

//-------------------------  External Functions ---------------------------------------------------
/** Implementation of complex integral E_1
 */
std::complex<double> E1(std::complex<double> z)
{
  std::complex<double> e1;

  double rz = real(z);
  double az = abs(z);

  if (fabs(az) < 1.0E-8)
  {
    // If z = 0, then the result is infinity... diverge!
    complex<double> r(1.0E300, 0.0);
    e1 = r;
  }
  else if (az <= 10.0 || (rz < 0.0 && az < 20.0))
  {
    // Some interesting region, equal to integrate to infinity, converged
    complex<double> r(1.0, 0.0);
    e1 = r;
    complex<double> cr = r;

    for (size_t k = 1; k <= 150; ++k)
    {
      double dk = double(k);
      cr = -cr * dk * z / ( (dk+1.0)*(dk+1.0) );
      e1 += cr;
      if (abs(cr) < abs(e1)*1.0E-15)
      {
        // cr is converged to zero
        break;
      }
    } // ENDFOR k

    e1 = -e1 - log(z) + (z*e1);
  }
  else
  {
    // Rest of the region
    complex<double> ct0(0.0, 0.0);
    for (int k = 120; k > 0; --k)
    {
      complex<double> dk(double(k), 0.0);
      ct0 = dk / (10.0 + dk / (z + ct0));
    } // ENDFOR k

    e1 = 1.0 / (z + ct0);
    e1 = e1 * exp(-z);
    if (rz < 0.0 && fabs(imag(z)) < 1.0E-10 )
    {
      complex<double> u(0.0, 1.0);
      e1 = e1 - (PI * u);
    }
  }

  return e1;
}

} // namespace CurveFitting
} // namespace Mantid
