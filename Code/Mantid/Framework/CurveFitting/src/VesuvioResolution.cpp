//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/VesuvioResolution.h"
#include "MantidCurveFitting/ConvertToYSpace.h"
#include "MantidAPI/FunctionFactory.h"
#include <gsl/gsl_poly.h>

namespace Mantid
{
namespace CurveFitting
{
  namespace
  {
    ///@cond
    const char * MASS_NAME = "Mass";

    const double STDDEV_TO_HWHM = std::sqrt(std::log(4.0));
    ///@endcond
  }

  // Register into factory
  DECLARE_FUNCTION(VesuvioResolution);

  //---------------------------------------------------------------------------
  // Static functions
  //---------------------------------------------------------------------------

  /**
  * @param ws The workspace with attached instrument
  * @param index Index of the spectrum
  * @return DetectorParams structure containing the relevant parameters
  */
  ResolutionParams VesuvioResolution::getResolutionParameters(const API::MatrixWorkspace_const_sptr & ws,
                                                              const size_t index)
  {
    Geometry::IDetector_const_sptr detector;
    try
    {
      detector = ws->getDetector(index);
    }
    catch (Kernel::Exception::NotFoundError &)
    {
      throw std::invalid_argument("VesuvioResolution - Workspace has no detector attached to histogram at index " + \
        boost::lexical_cast<std::string>(index));
    }

    ResolutionParams respar;
    const auto & pmap = ws->constInstrumentParameters();
    respar.dl1 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l1");
    respar.dl2 = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_l2");
    respar.dtof = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_tof");
    respar.dthe = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_theta"); //radians
    respar.dEnLorentz = ConvertToYSpace::getComponentParameter(detector, pmap, "hwhm_lorentz");
    respar.dEnGauss = ConvertToYSpace::getComponentParameter(detector, pmap, "sigma_gauss");
    return respar;
  }
  
  //---------------------------------------------------------------------------
  // Member functions
  //---------------------------------------------------------------------------
  
  VesuvioResolution::VesuvioResolution() : API::ParamFunction(), API::IFunction1D(),
      m_log("VesuvioResolution"),
      m_wsIndex(0), m_mass(0.0), m_voigt(),
      m_resolutionSigma(0.0), m_lorentzFWHM(0.0)
  {}

  /**
   * @returns A string containing the name of the function
   */
  std::string VesuvioResolution::name() const
  {
    return "VesuvioResolution";
  }


  //-------------------------------------- Function evaluation -----------------------------------------

  /*
   * Creates the internal caches
   */
  void VesuvioResolution::setUpForFit()
  {
    // Voigt
    using namespace Mantid::API;
    m_voigt = boost::dynamic_pointer_cast<IPeakFunction>(FunctionFactory::Instance().createFunction("Voigt"));
  }

  /**
   * Also caches parameters from the instrument
   * @param workspace The workspace set as input
   * @param wsIndex A workspace index
   * @param startX Starting x-vaue (unused).
   * @param endX Ending x-vaue (unused).
   */
  void VesuvioResolution::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wsIndex,double startX, double endX)
  {
    UNUSED_ARG(startX);
    UNUSED_ARG(endX);

    m_wsIndex = wsIndex;
    DetectorParams detpar = ConvertToYSpace::getDetectorParameters(workspace, m_wsIndex);
    ResolutionParams respar = getResolutionParameters(workspace, m_wsIndex);
    this->cacheResolutionComponents(detpar, respar);
  }

  /**
   * @param detpar Structure containing detector parameters
   * @param respar Structure containing resolution parameters
   */
  void VesuvioResolution::cacheResolutionComponents(const DetectorParams & detpar, const ResolutionParams & respar)
  {
    // geometry
    double theta = detpar.theta; //cache for frequent access
    double hwhmLorentzE = respar.dEnLorentz;
    double hwhmGaussE = STDDEV_TO_HWHM*respar.dEnGauss;

    // ------ Fixed coefficients related to resolution & Y-space transforms ------------------
    const double mn = PhysicalConstants::NeutronMassAMU;
    const double mevToK = PhysicalConstants::E_mev_toNeutronWavenumberSq;

    const double k1 = std::sqrt(detpar.efixed/mevToK);
    const double l2l1 = detpar.l2/detpar.l1;

    // Resolution dependence

    // Find K0/K1 at y=0 by taking the largest root of (M-1)s^2 + 2cos(theta)s - (M+1) = 0
    // Quadratic if M != 1 but simple linear if it does
    double k0k1(0.0);
    if((m_mass-1.0) > DBL_EPSILON)
    {
      double x0(0.0),x1(0.0);
      gsl_poly_solve_quadratic(m_mass-1.0, 2.0*std::cos(theta), -(m_mass+1.0), &x0, &x1);
      k0k1 = std::max(x0,x1); // K0/K1 at y=0
    }
    else
    {
      // solution is simply s = 1/cos(theta)
      k0k1 = 1.0/std::cos(theta);
    }
    double qy0(0.0), wgauss(0.0);

    if(m_mass > 1.0)
    {
      qy0 = std::sqrt(k1*k1*m_mass*(k0k1*k0k1 - 1));
      double k0k1p3 = std::pow(k0k1,3);
      double r1 = -(1.0 + l2l1*k0k1p3);
      double r2 = 1.0 - l2l1*k0k1p3 + l2l1*std::pow(k0k1,2)*std::cos(theta) - k0k1*std::cos(theta);

      double factor = (0.2413/qy0)*((m_mass/mn)*r1 - r2);
      m_lorentzFWHM = std::abs(factor*hwhmLorentzE*2);
      wgauss = std::abs(factor*hwhmGaussE*2);
    }
    else
    {
      qy0 = k1*std::tan(theta);
      double factor = (0.2413*2.0/k1)*std::abs((std::cos(theta) + l2l1)/std::sin(theta));
      m_lorentzFWHM = hwhmLorentzE*factor;
      wgauss = hwhmGaussE*factor;
    }

    double k0y0 = k1*k0k1;                     // k0_y0 =  k0 value at y=0
    double wtheta = 2.0*STDDEV_TO_HWHM*std::abs(k0y0*k1*std::sin(theta)/qy0)*respar.dthe;
    double common = (m_mass/mn) - 1 + k1*std::cos(theta)/k0y0;
    double wl1 = 2.0*STDDEV_TO_HWHM*std::abs((std::pow(k0y0,2)/(qy0*detpar.l1))*common)*respar.dl1;
    double wl2 = 2.0*STDDEV_TO_HWHM*std::abs((std::pow(k0y0,3)/(k1*qy0*detpar.l1))*common)*respar.dl2;

    m_resolutionSigma = std::sqrt(std::pow(wgauss,2) + std::pow(wtheta,2) + std::pow(wl1,2) + std::pow(wl2,2));

    m_log.notice() << "--------------------- Mass=" << m_mass << " -----------------------" << std::endl;
    m_log.notice() << "w_l1 (FWHM)=" << wl2 << std::endl;
    m_log.notice() << "w_l0 (FWHM)=" << wl1 << std::endl;
    m_log.notice() << "w_theta (FWHM)=" << wtheta << std::endl;
    m_log.notice() << "w_foil_lorentz (FWHM)=" << m_lorentzFWHM << std::endl;
    m_log.notice() << "w_foil_gauss (FWHM)=" << wgauss << std::endl;

  }

  void VesuvioResolution::function1D(double* out, const double* xValues, const size_t nData) const
  {
    std::vector<double> outVec(static_cast<int>(nData),0);
    const std::vector<double> xValuesVec(xValues, xValues + nData);
    voigtApprox(outVec, xValuesVec, 0, 1, m_lorentzFWHM, m_resolutionSigma);
    std::copy(outVec.begin(), outVec.end(), out);
  }

  /**
   */
  void VesuvioResolution::declareAttributes()
  {
    declareAttribute(MASS_NAME, IFunction::Attribute(m_mass));
  }

  /**
   * @param name The name of the attribute
   * @param value The attribute's value
   */
  void VesuvioResolution::setAttribute(const std::string& name,const Attribute& value)
  {
    IFunction::setAttribute(name,value); // Make sure the base-class stores it
    if(name == MASS_NAME) m_mass = value.asDouble();
  }

  /**
   * Convenience wrapper for voigtApprox. This version uses the cached values of the widths
   * @param voigt [Out] Output values (vector is expected to be of the correct size
   * @param xValues Input coordinates
   * @param lorentzPos LorentzPos parameter
   * @param lorentzAmp LorentzAmp parameter
   */
  void VesuvioResolution::voigtApprox(std::vector<double> & voigt, const std::vector<double> & xValues, const double lorentzPos,
                                 const double lorentzAmp) const
  {
    voigtApprox(voigt, xValues, lorentzPos, lorentzAmp, m_lorentzFWHM, m_resolutionSigma);
  }

  /**
   * Transforms the input y coordinates using the Voigt function approximation. The area is normalized to lorentzAmp
   * @param voigt [Out] Output values (vector is expected to be of the correct size
   * @param xValues Input coordinates
   * @param lorentzPos LorentzPos parameter
   * @param lorentzAmp LorentzAmp parameter
   * @param lorentzWidth LorentzFWHM parameter
   * @param gaussWidth GaussianFWHM parameter
   */
  void VesuvioResolution::voigtApprox(std::vector<double> & voigt, const std::vector<double> & xValues, const double lorentzPos,
                                 const double lorentzAmp, const double lorentzWidth, const double gaussWidth) const
  {
    m_voigt->setParameter(0,lorentzAmp);
    m_voigt->setParameter(1,lorentzPos);
    m_voigt->setParameter(2,lorentzWidth);
    m_voigt->setParameter(3,gaussWidth);
    assert(voigt.size() == xValues.size());
    m_voigt->functionLocal(voigt.data(), xValues.data(), xValues.size());

    // Normalize so that integral of V=lorentzAmp
    const double norm = 1.0/(0.5*M_PI*lorentzWidth);
    std::transform(voigt.begin(), voigt.end(), voigt.begin(), std::bind2nd(std::multiplies<double>(), norm));
  }

} // namespace CurveFitting
} // namespace Mantid
