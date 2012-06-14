#include "MantidCurveFitting/LeBailFit.h"
#include "MantidKernel/System.h"
#include "MantidCurveFitting/ThermoNeutronBackToBackExpPV.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_FUNCTION(LeBailFit)

  // Get a reference to the logger
  Mantid::Kernel::Logger& LeBailFit::g_log = Kernel::Logger::get("LeBailFit");

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LeBailFit::LeBailFit()
  {
    mL1 = 1.0;
    mL2 = 0.0;

    mPeak = new CurveFitting::ThermoNeutronBackToBackExpPV();
    mPeak->initialize();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LeBailFit::~LeBailFit()
  {
  }
  
  std::string LeBailFit::name() const
  {
    return "LeBailFit";
  }

  void LeBailFit::init()
  {
    declareParameter("Dtt1", 1.0);
    declareParameter("Dtt2", 1.0);
    declareParameter("Dtt1t", 1.0);
    declareParameter("Dtt2t", 1.0);
    declareParameter("Zero", 0.0);
    declareParameter("Zerot", 0.0);

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

    g_log.warning() << "In function1D(), it is wrong to use a user given height for peak function. " << std::endl;

    return;
  }

  void LeBailFit::calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h,
      double &sigma_g2, double &gamma_l, std::map<std::string, double>& parmap) const
  {
    // 1. Get some parameters
    double wcross = getParameter("Width");
    double Tcross = getParameter("Tcross");

    // 2. Start to calculate alpha, beta, sigma2, gamma,
    double n = 0.5*erfc(wcross*(Tcross-1/dh));

    double alpha_e = Alph0 + Alph1*dh;
    double alpha_t = Alph0t - Alph1t/dh;
    alpha = 1/(n*alpha_e + (1-n)*alpha_t);

    double beta_e = Beta0 + Beta1*dh;
    double beta_t = Beta0t - Beta1t/dh;
    beta = 1/(n*beta_e + (1-n)*beta_t);

    double Th_e = Zero + Dtt1*dh;
    double Th_t = Zerot + Dtt1t*dh - Dtt2t/dh;
    Tof_h = n*Th_e + (1-n)*Th_t;

    sigma_g2 = Sig0 + Sig1*std::pow(dh, 2) + Sig2*std::pow(dh, 4);
    gamma_l = Gam0 + Gam1*dh + Gam2*std::pow(dh, 2);

    // 2. Add ...
    parmap.insert(std::make_pair("Alpha", alpha));
    parmap.insert(std::make_pair("Beta", beta));
    parmap.insert(std::make_pair("Sigma2", sigma_g2));
    parmap.insert(std::make_pair("Gamma", gamma_l));
    parmap.insert(std::make_pair("TOF_h", Tof_h));

    std::cout << "DB1214 D = " << dh << ", TOF = " << Tof_h << std::endl;

    return;
  }

  void LeBailFit::function1D(double *out, const double *xValues, size_t nData) const
  {
    // 1. Get parameters (class)
    Alph0 = getParameter("Alph0");
    Alph1 = getParameter("Alph1");
    Beta0 = getParameter("Beta0");
    Beta1 = getParameter("Beta1");
    Alph0t = getParameter("Alph0t");
    Alph1t = getParameter("Alph1t");
    Beta0t = getParameter("Beta0t");
    Beta1t = getParameter("Beta1t");
    Dtt1 = getParameter("Dtt1");
    Dtt1t = getParameter("Dtt1t");
    Dtt2t = getParameter("Dtt2t");
    Zero = getParameter("Zero");
    Zerot = getParameter("Zerot");
    Sig0 = getParameter("Sig0");
    Sig1 = getParameter("Sig1");
    Sig2 = getParameter("Sig2");
    Gam0 = getParameter("Gam0");
    Gam1 = getParameter("Gam1");
    Gam2 = getParameter("Gam2");

    // 2.
    double *tempout = new double[nData];

    for (size_t id = 0; id < dvalues.size(); ++id)
    {
      double dh = dvalues[id];
      // a) Calculate all the parameters
      double alpha, beta, tof_h, sigma2, gamma;
      calPeakParametersForD(dh, alpha, beta, tof_h, sigma2, gamma, mPeakParameters[id]);

      // b) Set peak parameters
      mPeak->setParameter("TOF_h", tof_h);
      mPeak->setParameter("height", heights[id]);
      mPeak->setParameter("Alpha", alpha);
      mPeak->setParameter("Beta", beta);
      mPeak->setParameter("Sigma2", sigma2);
      mPeak->setParameter("Gamma", gamma);

      // c) Calculate range
      g_log.error() << "Need a good algorithm to calculate a proper range for each peak. " << std::endl;

      // d) Calculate peak
      mPeak->function1D(tempout, xValues, nData);
      for (size_t iy = 0; iy < nData; ++iy)
      {
        out[iy] += tempout[iy];
      }
    }

    // 3. Clean
    delete tempout;

    return;
  }

  void LeBailFit::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian)
  {
    calNumericalDeriv(domain, jacobian);
    return;
  }

  /*
   * Analytical
   */
  void LeBailFit::functionDeriv1D(API::Jacobian *out, const double* xValues, const size_t nData)
  {
    throw std::runtime_error("LeBailFit does not support analytical derivative. ");
  }

  /*
   * Add a peak with its d-value
   */
  void LeBailFit::setPeak(double dh, double height)
  {
    dvalues.push_back(dh);
    heights.push_back(height);
    std::map<std::string, double> parmap;
    mPeakParameters.push_back(parmap);

    return;
  }

  /*
   * A public function API for function1D
   */
  void LeBailFit::calPeaks(double* out, const double* xValues, const size_t nData)
  {
    this->function1D(out, xValues, nData);

    return;
  }

  double LeBailFit::getPeakParameter(size_t index, std::string parname) const
  {
    if (index >= mPeakParameters.size())
    {
      g_log.error() << "getParameter() Index out of range" << std::endl;
      throw std::runtime_error("Index out of range");
    }

    std::map<std::string, double>::iterator mit;
    mit = mPeakParameters[index].find(parname);
    double value = 0.0;
    if (mit != mPeakParameters[index].end())
    {
      value = mit->second;
    }
    else
    {
      g_log.error() << "Unable to find parameter " << parname << " in PeakParameters[" << index << "]" << std::endl;
      throw std::invalid_argument("Non-existing parameter name.");
    }

    return value;
  }




} // namespace Mantid
} // namespace CurveFitting
