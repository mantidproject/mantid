//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/NCSCountRate.h"

#include <boost/lexical_cast.hpp>
#include <sstream>

namespace Mantid
{
  namespace CurveFitting
  {
    namespace
    {
      ///@cond
      const char * WSINDEX_NAME = "WorkspaceIndex";
      const char * MASSES_NAME = "Masses";
      const char * HERMITE_C_NAME = "HermiteCoeffs";
      const char * BKGD_DEGREE_NAME = "BackgroundPoly";
      ///@endcond
    }

    /**
     */
    NCSCountRate::NCSCountRate() : API::ParamFunction(),
        m_workspace(), m_wsIndex(0), m_masses(), m_hermite(), m_bkgdPoly(4),
        m_l1(0.0), m_sigmaL1(0.0), m_l2(0.0), m_sigmaL2(0.0), m_theta(0.0), m_sigmaTheta(0.0), m_e1(0.0),
        m_t0(0.0), m_hwhmGaussE(0.0), m_hwhmLorentzE(0.0)
    {}

    /**
     * @returns A string identifier for the function
     */
    std::string NCSCountRate::name() const
    {
      return "NCSCountRate";
    }

    /**
     */
    void NCSCountRate::declareAttributes()
    {
      // General workspace access parameters
      declareAttribute(WSINDEX_NAME, IFunction::Attribute(static_cast<int>(m_wsIndex)));

     /** Momentum distribution
      * Each mass, with the exception of the proton/deuterium mass, is fitted with a Gaussian
      * approximation for the momentum distribution. The proton/deuterium mass is fitted using
      * a Gram-Charlier approximation which is constructed using Hermite polynomials.
      * The number of fit parameters therefore depends on the number of masses & number of hermite
      * polynomials included for the proton expansion
      */
      declareAttribute(MASSES_NAME, IFunction::Attribute("")); // space-separated string of the mass inputs
      declareAttribute(HERMITE_C_NAME, IFunction::Attribute("")); // space-separated string 1/0 indicating which coefficients are active

      // Chebyshev polynomial background (0 turns it off)
      declareAttribute(BKGD_DEGREE_NAME, IFunction::Attribute(m_bkgdPoly));
    }

    /**
     * Adds the parameters that don't depend on the number of masses
     */
    void NCSCountRate::declareParameters()
    {
      this->declareParameter("FSECoeff", 0.0, "FSE coefficient");
    }

    /**
     * @param name The name of the attribute
     * @param value The attribute's value
     */
    void NCSCountRate::setAttribute(const std::string& name,const Attribute& value)
    {
      if(name == MASSES_NAME) setMasses(value.asString());
      else if(name == HERMITE_C_NAME) setHermiteCoefficients(value.asString());
    }

    /**
     * Also caches parameters from the instrument
     * Throws if it is not a MatrixWorkspace
     * @param ws The workspace set as input
     */
    void NCSCountRate::setWorkspace(boost::shared_ptr<const API::Workspace> ws)
    {
      m_workspace = boost::dynamic_pointer_cast<const API::MatrixWorkspace>(ws);
      if(!m_workspace)
      {
        throw std::invalid_argument("NCSCountRate expected an object of type MatrixWorkspace, type=" + ws->id());
      }
      auto inst = m_workspace->getInstrument();
      auto sample = inst->getSample();
      auto source = inst->getSource();
      if(!sample || !source)
      {
        throw std::invalid_argument("NCSCountRate - Workspace has no source/sample.");
      }
      Geometry::IDetector_const_sptr det;
      try
      {
       det = m_workspace->getDetector(m_wsIndex);
      }
      catch (Kernel::Exception::NotFoundError &)
      {
        throw std::invalid_argument("NCSCountRate - Workspace has not detector attached to histogram at index " + boost::lexical_cast<std::string>(m_wsIndex));
      }

      m_l1 = sample->getDistance(*source);
      m_l2 = det->getDistance(*sample);
      m_theta = m_workspace->detectorTwoTheta(det);

      // parameters
      m_sigmaL1 = getComponentParameter(*det, "sigma_l1");
      m_sigmaL2 = getComponentParameter(*det, "sigma_l2");
      m_sigmaTheta = getComponentParameter(*det, "sigma_theta");
      m_e1 = getComponentParameter(*det,"efixed");
      m_t0 = getComponentParameter(*det,"t0"); // Loaded from IP file
      m_hwhmLorentzE = getComponentParameter(*det, "hwhm_energy_lorentz");
      m_hwhmGaussE = std::sqrt(std::log(4))*getComponentParameter(*det, "sigma_energy_gauss");
    }

    /*
     * Creates the internal caches
     */
    void NCSCountRate::setUpForFit()
    {

    }


    //-------------------------------------- Function evaluation -----------------------------------------

    /**
     * Calculates the value of the function for each x value and stores in the given output array
     * @param out An array of size nData to store the results
     * @param xValues The input X data array of size nData
     * @param nData The length of the out & xValues arrays
     */
    void NCSCountRate::function1D(double* out, const double* xValues, const size_t nData) const
    {
      // First transform to Y space


    }

    //-------------------------------------- Attribute setters -------------------------------------------
    /**
     * Throws if the string is empty or the string contains something other than numbers
     * @param masses A string of space separated values for the masses
     */
    void NCSCountRate::setMasses(const std::string & masses)
    {
      if(masses.empty())
      {
        throw std::invalid_argument("NCSCountRate - Mass string cannot be empty!");
      }

      m_masses.clear();
      m_masses.reserve(6); //Guess at an upper limit to limit memory allocations
      std::istringstream is(masses);
      while(!is.eof())
      {
        double value(0.0);
        is >> value;
        if(!is)
        {
          throw std::invalid_argument("NCSCountRate - Error reading number from mass string: " + masses);
        }
        m_masses.push_back(value);
      }
      declareGaussianParameters();
    }

    /**
     * All masses but the first mass are fitted with gaussians.
     * Adds sigma & intensity parameters for each mass except the first
     */
    void NCSCountRate::declareGaussianParameters()
    {
      const char * WIDTH_PREFIX = "Sigma_";
      const char * INTENSITY_PREFIX = "Intens_";
      for(size_t i = 1; i < m_masses.size(); ++i)
      {
        std::ostringstream os;
        os << WIDTH_PREFIX << i;
        this->declareParameter(os.str(),1.0, "Gaussian width of ith mass");
        os.str("");// Clear stream
        os << INTENSITY_PREFIX << i;
        this->declareParameter(os.str(),1.0, "Scattering intensity for ith mass");
      }

    }


    /**
     * Throws if the string is empty and contains something other than numbers
     * @param coeffs A string of space separated 1/0 values indicating which polynomial coefficients to include in the fitting
     */
    void NCSCountRate::setHermiteCoefficients(const std::string & coeffs)
    {
      if(coeffs.empty())
      {
        throw std::invalid_argument("NCSCountRate - Hermite polynomial string is empty!");
      }
      m_hermite.clear();
      m_hermite.reserve(3); // Maximum guess
      std::istringstream is(coeffs);
      while(!is.eof())
      {
        short value;
        is >> value;
        if(!is)
        {
          throw std::invalid_argument("NCSCountRate - Error reading int from hermite coefficient string: " + coeffs);
        }
        m_hermite.push_back(value);
      }
      declareGramCharlierParameters();
    }

    /**
     * Currently the first mass is assumed to be fitted with the Gram-Charlier expansion.
     * The input string gives whether each even hermite polnomial is active or not
     */
    void NCSCountRate::declareGramCharlierParameters()
    {
      // Gram-Chelier parameters are the even coefficents of the Hermite polynomials, i.e
      // setting hermite coefficients to "1 0 1" uses coefficents C_0,C_4 and C_2 is skipped
      const char * HERMITE_PREFIX = "C_";
      for(size_t i = 0; i < m_hermite.size(); ++i)
      {
        if(m_hermite[i] > 0)
        {
          std::ostringstream os;
          os << HERMITE_PREFIX << 2*i;
          this->declareParameter(os.str(), 1.0, "Hermite polynomial coefficent");
        }
      }
    }


    /**
     * Throws if the value is negative.
     * @param npoly The degree of polynomial to use for the background
     */
    void NCSCountRate::setBackgroundPolyDegree(const int npoly)
    {
      if(npoly < 0)
      {
        throw std::invalid_argument("NCSCountRate - Background polynomial degree must be positive! BackgroundPoly="
            + boost::lexical_cast<std::string>(npoly));
      }
      m_bkgdPoly = npoly;
    }

    //--------------------------------------- Attribute query ---------------------------------------------
    /**
     * @returns True if the background should be included, false otherwise
     */
    bool NCSCountRate::backgroundRequsted() const
    {
      return (m_bkgdPoly > 0);
    }

    //------------------------------------------ Helpers ---------------------------------------------------
    /**
     * @param comp A reference to the component that should contain the parameter
     * @param name The name of the parameter
     * @returns The value of the parameter if it exists
     * @throws A std::invalid_argument error if the parameter does not exist
     */
    double NCSCountRate::getComponentParameter(const Geometry::IComponent & comp,const std::string &name) const
    {
      std::vector<double> pars = comp.getNumberParameter(name);
      if(!pars.empty())
      {
        return pars[0];
      }
      else
      {
        throw std::invalid_argument("NCSCountRate - Unable to find component parameter \"" + name + "\".");
      }
    }


  } // namespace CurveFitting
} // namespace Mantid
