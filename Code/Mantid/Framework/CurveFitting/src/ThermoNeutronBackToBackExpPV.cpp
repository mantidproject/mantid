#include "MantidCurveFitting/ThermoNeutronBackToBackExpPV.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include <gsl/gsl_sf_erf.h>

#define PI 3.14159265358979323846264338327950288419716939937510582

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_FUNCTION(ThermoNeutronBackToBackExpPV)

  // Get a reference to the logger
  Mantid::Kernel::Logger& ThermoNeutronBackToBackExpPV::g_log = Kernel::Logger::get("ThermoNeutronBackToBackExpPV");

  // ----------------------------
  /*
   * Constructor and Desctructor
   */
  ThermoNeutronBackToBackExpPV::ThermoNeutronBackToBackExpPV() : mFWHM(0.0)
  {
    mLowTOF = 0.0;
    mUpperTOF = 1.0E20;
  }

  ThermoNeutronBackToBackExpPV::~ThermoNeutronBackToBackExpPV()
  {

  }

  /*
   * Initialize:  declare paraemters
   */
  void ThermoNeutronBackToBackExpPV::init()
  {
    declareParameter("I", 1.0);
    declareParameter("TOF_h", -0.0);
    declareParameter("height", 1.0);
    declareParameter("Alpha",1.6);
    declareParameter("Beta",1.6);
    declareParameter("Sigma2", 1.0);
    declareParameter("Gamma", 0.0);

    return;
  }

  double ThermoNeutronBackToBackExpPV::centre()const
  {
    double tofh = getParameter("TOF_h");


    return tofh;
  }


  void ThermoNeutronBackToBackExpPV::setHeight(const double h)
  {
    setParameter("height", h);

    return;
  };

  double ThermoNeutronBackToBackExpPV::height() const
  {
    double height = this->getParameter("height");
    return height;
  };

  /*
   * Calculate FWHM
   * It will be calculated each time when it is called because
   * there is no record whether any peak parameter is changed since last caculation
   * of FWHM.
   */
  double ThermoNeutronBackToBackExpPV::fwhm() const
  {
    if (mFWHM < 1.0E-8)
    {
      double sigma2 = this->getParameter("Sigma2");
      double gamma = this->getParameter("Gamma");
      double H, eta;
      calHandEta(sigma2, gamma, H, eta);
    }

    return mFWHM;
  };

  void ThermoNeutronBackToBackExpPV::setFwhm(const double w)
  {
    UNUSED_ARG(w);
    throw std::invalid_argument("Unable to set FWHM");
  };

  void ThermoNeutronBackToBackExpPV::setCentre(const double c)
  {
    setParameter("TOF_h",c);
  };

  /*
   * Implement the peak calculating formula
   */
  void ThermoNeutronBackToBackExpPV::functionLocal(double* out, const double* xValues, const size_t nData) const
  {
    // 1. Prepare constants
    const double alpha = this->getParameter("Alpha");
    const double beta = this->getParameter("Beta");
    const double sigma2 = this->getParameter("Sigma2");
    const double gamma = this->getParameter("Gamma");
    const double height = this->getParameter("height");
    const double tof_h = this->getParameter("TOF_h");

    double invert_sqrt2sigma = 1.0/sqrt(2.0*sigma2);
    double N = alpha*beta*0.5/(alpha+beta);

    double H, eta;
    calHandEta(sigma2, gamma, H, eta);

    g_log.debug() << "DB1140: TOF_h = " << tof_h << " h = " << height << ", I = " << this->getParameter("I") << " alpha = "
        << alpha << " beta = " << beta << " H = " << H << " eta = " << eta << "  Peak Range = " << mLowTOF << ", "
        << mUpperTOF << std::endl;

    // 2. Do calculation
    g_log.debug() << "DB1143:  nData = " << nData << "  From " << xValues[0] << " To " << xValues[nData-1] << std::endl;
    for (size_t id = 0; id < nData; ++id)
    {
      if (xValues[id] >= mLowTOF &&  xValues[id] <= mUpperTOF)
      {
        double dT = xValues[id]-tof_h;
        double omega = calOmega(dT, eta, N, alpha, beta, H, sigma2, invert_sqrt2sigma);
        out[id] = height*omega;
      }
      else
      {
        out[id] = 0.0;
      }
    }

    return;
  }

  void ThermoNeutronBackToBackExpPV::functionDerivLocal(API::Jacobian* , const double* , const size_t )
  {
    throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for IkedaCarpenterPV.");
  }

  void ThermoNeutronBackToBackExpPV::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
  {
    calNumericalDeriv(domain, jacobian);
  }

  /*
   * Calculate Omega(x) = ... ...
   */
  double ThermoNeutronBackToBackExpPV::calOmega(double x, double eta, double N, double alpha, double beta, double H,
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
  std::complex<double> ThermoNeutronBackToBackExpPV::E1(std::complex<double> z) const
  {
    return z;
  }

  void ThermoNeutronBackToBackExpPV::geneatePeak(double* out, const double* xValues, const size_t nData)
  {
    this->functionLocal(out, xValues, nData);

    return;
  }

  void ThermoNeutronBackToBackExpPV::calHandEta(double sigma2, double gamma, double& H, double& eta) const
  {
    // 1. Calculate H
    double H_G = sqrt(8.0 * sigma2 * log(2.0));
    double H_L = gamma;

    double temp1 = std::pow(H_L, 5) + 0.07842*H_G*std::pow(H_L, 4) + 4.47163*std::pow(H_G, 2)*std::pow(H_L, 3) +
        2.42843*std::pow(H_G, 3)*std::pow(H_L, 2) + 2.69269*std::pow(H_G, 4)*H_L + std::pow(H_G, 5);

    H = std::pow(temp1, 0.2);

    mFWHM = H;

    // 2. Calculate eta
    double gam_pv = H_L/H;
    eta = 1.36603 * gam_pv - 0.47719 * std::pow(gam_pv, 2) + 0.11116 * std::pow(gam_pv, 3);

    if (eta > 1 || eta < 0)
    {
      g_log.error() << "Calculated eta = " << eta << " is out of range [0, 1]." << std::endl;
    }

    return;
  }

  /*
   * Set the range for a peak to be calculated
   */
  void ThermoNeutronBackToBackExpPV::setCalculationRange(double tof_low, double tof_upper)
  {
    mLowTOF = tof_low;
    mUpperTOF = tof_upper;

    return;
  }

  void ThermoNeutronBackToBackExpPV::resetFWHM()
  {
    mFWHM = 0.0;
  }

} // namespace Mantid
} // namespace CurveFitting
