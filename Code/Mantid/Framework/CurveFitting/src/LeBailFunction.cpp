#include "MantidCurveFitting/LeBailFunction.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"

#define DEFAULTPEAKWIDTHFACTOR 8.0

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid
{
namespace CurveFitting
{

  DECLARE_FUNCTION(LeBailFunction)

  // Get a reference to the logger
  Mantid::Kernel::Logger& LeBailFunction::g_log = Kernel::Logger::get("LeBailFunction");

  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  LeBailFunction::LeBailFunction()
  {
    mL1 = 1.0;
    mL2 = 0.0;

    g_log.warning() << "LeBailFunction.function1D(): Need a good algorithm to calculate a proper range for each peak. " << std::endl;

    return;
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  LeBailFunction::~LeBailFunction()
  {
  }
  
  std::string LeBailFunction::name() const
  {
    return "LeBailFunction";
  }

  void LeBailFunction::init()
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

    declareParameter("LatticeConstant", 10.0);

    return;
  }

  /*
   * Calculate peak parameters for a peak at d (d-spacing value)
   * Output will be written to parameter map too.
   */
  void LeBailFunction::calPeakParametersForD(double dh, double& alpha, double& beta, double &Tof_h,
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

    // 3. Add to parameter map
    parmap.insert(std::make_pair("Alpha", alpha));
    parmap.insert(std::make_pair("Beta", beta));
    parmap.insert(std::make_pair("Sigma2", sigma_g2));
    parmap.insert(std::make_pair("Gamma", gamma_l));
    parmap.insert(std::make_pair("TOF_h", Tof_h));

    g_log.debug() << "DB1214 D = " << dh << ", TOF = " << Tof_h << std::endl;

    return;
  }

  /*
   * Calculate all peaks' parameters
   */
  void LeBailFunction::calPeaksParameters()
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

    // 2. Calcualte peak parameters for all peaks
    for (size_t id = 0; id < dvalues.size(); ++id)
    {
      double dh = dvalues[id];
      // a) Calculate all the parameters
      double alpha, beta, tof_h, sigma2, gamma;
      calPeakParametersForD(dh, alpha, beta, tof_h, sigma2, gamma, mPeakParameters[id]);

      // b) Set peak parameters
      mPeaks[id]->setParameter("TOF_h", tof_h);
      mPeaks[id]->setParameter("height", heights[id]);
      mPeaks[id]->setParameter("Alpha", alpha);
      mPeaks[id]->setParameter("Beta", beta);
      mPeaks[id]->setParameter("Sigma2", sigma2);
      mPeaks[id]->setParameter("Gamma", gamma);
    }

    return;
  }

  void LeBailFunction::function1D(double *out, const double *xValues, size_t nData) const
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
    double latticeconstant = getParameter("LatticeConstant");

    /*
    std::cout << " \n-------------------------  being visited -----------------------\n" << std::endl;
    std::cout << "Alph0  = " << Alph0 << std::endl;
    std::cout << "Alph1  = " << Alph1 << std::endl;
    std::cout << "Alph0t = " << Alph0t << std::endl;
    std::cout << "Alph1t = " << Alph1t << std::endl;
    std::cout << "Zero   = " << Zero << std::endl;
    std::cout << "Zerot  = " << Zerot << std::endl;
    std::cout << "Lattice= " << latticeconstant << " Number of Peaks = " << mPeakHKLs.size() << std::endl;
    */

    // 2.
    double *tempout = new double[nData];
    for (size_t iy = 0; iy < nData; ++iy)
    {
      out[iy] = 0.0;
    }

    for (size_t id = 0; id < mPeakHKLs.size(); ++id)
    {
      int h = mPeakHKLs[id][0];
      int k = mPeakHKLs[id][1];
      int l = mPeakHKLs[id][2];
      double dh = calCubicDSpace(latticeconstant, h, k, l);
      dvalues[id] = dh;

      // a) Calculate all the parameters
      double alpha, beta, tof_h, sigma2, gamma;
      calPeakParametersForD(dh, alpha, beta, tof_h, sigma2, gamma, mPeakParameters[id]);

      // b) Set peak parameters
      g_log.debug() << "DB546 Peak @ d = " << dh << " Set Height = " << dh << std::endl;
      mPeaks[id]->setParameter("TOF_h", tof_h);
      mPeaks[id]->setParameter("height", heights[id]);
      mPeaks[id]->setParameter("Alpha", alpha);
      mPeaks[id]->setParameter("Beta", beta);
      mPeaks[id]->setParameter("Sigma2", sigma2);
      mPeaks[id]->setParameter("Gamma", gamma);

      // c) FIXME Implement "Calculate individual peak range"
      double fwhm = mPeaks[id]->fwhm();
      double tof_low = tof_h - DEFAULTPEAKWIDTHFACTOR*fwhm;
      double tof_upper = tof_h + DEFAULTPEAKWIDTHFACTOR*fwhm;
      mPeaks[id]->setCalculationRange(tof_low, tof_upper);

      // d) Calculate peak
      mPeaks[id]->function1D(tempout, xValues, nData);
      for (size_t iy = 0; iy < nData; ++iy)
      {
        out[iy] += tempout[iy];
      }
    } // END-FOR D-values

    for (size_t n = 0; n < nData; ++n)
      g_log.debug() << "DB327 " << xValues[n] << "\t\t" << out[n] << std::endl;

    // 3. Clean
    delete tempout;

    return;
  }

  /*
   * Using numerical derivative
   */
  void LeBailFunction::functionDeriv(const API::FunctionDomain &domain, API::Jacobian &jacobian)
  {
    calNumericalDeriv(domain, jacobian);
    return;
  }

  /*
   * Analytical
   */
  void LeBailFunction::functionDeriv1D(API::Jacobian *out, const double* xValues, const size_t nData)
  {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);

    throw std::runtime_error("LeBailFunction does not support analytical derivative. ");
  }

  /*
   * Add a peak with its d-value
   */
  void LeBailFunction::addPeak(double dh, double height)
  {
    dvalues.push_back(dh);
    heights.push_back(height);

    // API::IPeakFunction* tpeak = new CurveFitting::ThermoNeutronBackToBackExpPV();
    CurveFitting::ThermoNeutronBackToBackExpPV* peakptr = new CurveFitting::ThermoNeutronBackToBackExpPV();
    CurveFitting::ThermoNeutronBackToBackExpPV_sptr tpeak(peakptr);
    tpeak->setPeakRadius(8);

    tpeak->initialize();
    mPeaks.push_back(tpeak);

    std::map<std::string, double> parmap;
    mPeakParameters.push_back(parmap);

    return;
  }

  /*
   * Add a peak (HKL)
   */
  void LeBailFunction::addPeaks(std::vector<std::vector<int> > peakhkls, std::vector<double> peakheights)
  {
    // 1. Check
    if (peakhkls.size() != peakheights.size())
    {
      g_log.error() << "SetPeaks().  Input number of (HKL) is not equal to peak heights. " << std::endl;
      throw std::invalid_argument("Peak's HKL and height do not match. ");
    }

    // 2. Calculate peak positions
    double lattice = getParameter("LatticeConstant");
    for (size_t ipk = 0; ipk < peakhkls.size(); ++ ipk)
    {
      if (peakhkls[ipk].size() != 3)
      {
        throw std::invalid_argument("Vector for (HKL) must have three and only three integers.");
      }
      int h = peakhkls[ipk][0];
      int k = peakhkls[ipk][1];
      int l = peakhkls[ipk][2];
      double peak_d = calCubicDSpace(lattice, h, k, l);

      this->addPeak(peak_d, peakheights[ipk]);
      mPeakHKLs.push_back(peakhkls[ipk]);
    }

    return;
  } // END Function

  /*
   * Reset all peaks' height
   */
  void LeBailFunction::setPeakHeights(std::vector<double> inheights)
  {
    if (inheights.size() != heights.size())
    {
      g_log.error() << "Input number of peaks (height) is not same as peaks. " << std::endl;
      throw std::logic_error("Input number of peaks (height) is not same as peaks. ");
    }

    for (size_t ih = 0; ih < inheights.size(); ++ih)
      heights[ih] = inheights[ih];

    return;
  }


  CurveFitting::ThermoNeutronBackToBackExpPV_sptr LeBailFunction::getPeak(size_t peakindex)
  {
    if (peakindex >= mPeaks.size())
    {
      g_log.error() << "Try to access peak " << peakindex << " out of range [0, " << mPeaks.size() << ")." << std::endl;
      throw std::invalid_argument("getPeak() out of boundary");
    }

    CurveFitting::ThermoNeutronBackToBackExpPV_sptr rpeak = mPeaks[peakindex];

    return rpeak;
  }


  /*
   * Calculate d = a/sqrt(h**2+k**2+l**2)
   */
  double LeBailFunction::calCubicDSpace(double a, int h, int k, int l) const
  {
    double hklfactor = sqrt(double(h*h)+double(k*k)+double(l*l));
    double d = a/hklfactor;
    g_log.debug() << "DB143 a = " << a << " (HKL) = " << h << ", " << k << ", " << l << ": d = " << d << std::endl;

    return d;
  }

  /*
   * A public function API for function1D
   */
  void LeBailFunction::calPeaks(double* out, const double* xValues, const size_t nData)
  {
    this->function1D(out, xValues, nData);

    return;
  }

  /*
   * Return peak parameters
   */
  double LeBailFunction::getPeakParameter(size_t index, std::string parname) const
  {
    if (index >= mPeakParameters.size())
    {
      g_log.error() << "getParameter() Index out of range" << std::endl;
      throw std::runtime_error("Index out of range");
    }

    CurveFitting::ThermoNeutronBackToBackExpPV_sptr peak = mPeaks[index];

    double value = peak->getParameter(parname);

    /*
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
    */

    return value;
  }


} // namespace Mantid
} // namespace CurveFitting
