//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidCurveFitting/NCSCountRate.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Math/Distributions/HermitePolynomials.h"
#include "MantidKernel/PhysicalConstants.h"

#include <boost/lexical_cast.hpp>

#include <gsl/gsl_poly.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_sf_gamma.h> // for factorial
#include <gsl/gsl_spline.h>
#include <algorithm>
#include <sstream>

namespace Mantid
{
  namespace CurveFitting
  {
    // Regsiter into factory
    DECLARE_FUNCTION(NCSCountRate);

    namespace
    {
      ///@cond
      const char * WSINDEX_NAME = "WorkspaceIndex";
      const char * MASSES_NAME = "Masses";
      const char * HERMITE_C_NAME = "HermiteCoeffs";
      const char * BKGD_DEGREE_NAME = "BackgroundPoly";
      const char * KFSE_NAME = "FSECoeff";

      const char * WIDTH_PREFIX = "Sigma_";
      const char * INTENSITY_PREFIX = "Intens_";
      const char * HERMITE_PREFIX = "C_";

      const double STDDEV_TO_HWHM = std::sqrt(std::log(4));
      ///@endcond
    }

    /**
     */
    NCSCountRate::NCSCountRate() : API::ParamFunction(),
        m_workspace(), m_wsIndex(0), m_masses(), m_hermite(), m_bkgdPoly(4),
        m_l1(0.0), m_sigmaL1(0.0), m_l2(0.0), m_sigmaL2(0.0), m_theta(0.0), m_sigmaTheta(0.0), m_e1(0.0),
        m_t0(0.0), m_hwhmGaussE(0.0), m_hwhmLorentzE(0.0), m_voigt()
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
      this->declareParameter(KFSE_NAME, 0.0, "FSE coefficient k");
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
      m_t0 = getComponentParameter(*det,"t0")*1e-6;
      m_hwhmLorentzE = getComponentParameter(*det, "hwhm_energy_lorentz");
      m_hwhmGaussE = STDDEV_TO_HWHM*getComponentParameter(*det, "sigma_energy_gauss");
    }

    /*
     * Creates the internal caches
     */
    void NCSCountRate::setUpForFit()
    {
      using namespace Mantid::API;
      m_voigt = boost::dynamic_pointer_cast<API::IFunction1D>(FunctionFactory::Instance().createFunction("Voigt"));
      assert(m_voigt);
    }


    //-------------------------------------- Function evaluation -----------------------------------------

    /**
     * Calculates the value of the function for each x value and stores in the given output array
     * @param out An array of size nData to store the results
     * @param xValues The input X data array of size nData. It is assumed to be times in microseconds
     * @param nData The length of the out & xValues arrays
     */
    void NCSCountRate::function1D(double* out, const double* xValues, const size_t nData) const
    {
      std::vector<double> tInSecs(xValues, xValues + nData);
      std::transform(tInSecs.begin(), tInSecs.end(), tInSecs.begin(), std::bind2nd(std::multiplies<double>(), 1e-6)); // Convert to seconds

      const double mn = PhysicalConstants::NeutronMassAMU;
      const double mevToK = PhysicalConstants::E_mev_toNeutronWavenumberSq;
      const double massToMeV = 0.5*PhysicalConstants::NeutronMass/PhysicalConstants::meV; // Includes factor of 1/2

      const double v1 = std::sqrt(m_e1/massToMeV);
      const double k1 = std::sqrt(m_e1/mevToK);
      const double l2l1 = m_l2/m_l1;

      // Calculate energy dependent factors
      std::vector<double> e0(nData,0.0), omega(nData, 0.0), modQ(nData, 0.0);
      for(size_t i = 0; i < nData; ++i)
      {
        const double v0 = m_l1/(tInSecs[i] - m_t0 - (m_l2/v1));
        const double ei = massToMeV*v0*v0;
        e0[i] = ei;
        omega[i] = ei - m_e1;
        const double k0 = std::sqrt(ei/PhysicalConstants::E_mev_toNeutronWavenumberSq);
        modQ[i] = std::sqrt(k0*k0 + k1*k1 - 2.0*k0*k1*std::cos(m_theta));
      }

      //---------------------------- Transform to y-space --------------------------------------------------------
      const size_t nmasses = m_masses.size();
      std::vector<std::vector<double>> yspace(nmasses, std::vector<double>(nData));
      std::vector<double> lorentzW(nmasses), sigmaRes(nmasses);

      for(size_t i = 0; i < nmasses; ++i)
      {
        const double mi = m_masses[i];
        auto & ym = yspace[i]; // reference to vector
        for(size_t j = 0; j < nData; ++j)
        {
          const double q = modQ[j];
          const double w = omega[j];
          ym[j] = 0.2393*(mi/q)*(w - mevToK*q*q/mi);
        }

        double x0(0.0),x1(0.0);
        gsl_poly_solve_quadratic(mi-1, 2.0*std::cos(m_theta), -(mi+1), &x0,&x1);
        const double k0k1 = std::max(x0,x1); // K0/K1 at y=0

        double qy0(0.0), wl(0.0), wgauss(0.0);
        if(mi > 1.0)
        {
          qy0 = std::sqrt(mi*std::pow(k1*k0k1,2) - 1);
          double k0k1p3 = std::pow(k0k1,3);
          double r1 = -(1.0 + l2l1*k0k1p3);
          double r2 = 1.0 - l2l1*k0k1p3 + l2l1*std::pow(k0k1,2)*std::cos(m_theta) - k0k1*std::cos(m_theta);

          double factor = (0.2413/qy0)*((mi/mn)*r1 - r2);
          wl = std::abs(factor*m_hwhmLorentzE*2);
          wgauss = std::abs(factor*m_hwhmGaussE*2);
        }
        else
        {
          qy0 = k1*std::tan(m_theta);
          double factor = (0.2413*2.0/k1)*std::abs((std::cos(m_theta) + l2l1)/std::sin(m_theta));
          wl = m_hwhmLorentzE*factor;
          wgauss = m_hwhmGaussE*factor;
        }
        double k0y0 = k1*k0k1;                     // k0_y0 =  k0 value at y=0
        double wtheta = 2.0*STDDEV_TO_HWHM*std::abs(k0y0*k1*std::sin(m_theta)/qy0)*m_sigmaTheta;
        double common = (mi/mn) - 1 + k1*std::cos(m_theta)/k0y0;
        double wl1 = 2.0*STDDEV_TO_HWHM*std::abs((std::pow(k0y0,2)/(qy0*m_l1))*common)*m_sigmaL1;
        double wl2 = 2.0*STDDEV_TO_HWHM*std::abs((std::pow(k0y0,3)/(k1*qy0*m_l1))*common)*m_sigmaL2;

        sigmaRes[i] = std::sqrt(std::pow(wgauss,2) + std::pow(wtheta,2) + std::pow(wl1,2) + std::pow(wl2,2));
        lorentzW[i] = wl;
      }

      // -------------------------------- J1 --------------------------------------------------------------
      std::vector<std::vector<double>> j1(nmasses, std::vector<double>(nData, 0.0));

      const double kfse = getParameter(KFSE_NAME);
      std::vector<double> voigtDiffResult(nData); // Avoid repeated memory allocations for each loop
      for(size_t i = 0; i < nmasses; ++i)
      {
        const auto & yi = yspace[i];
        auto & j1i = j1[i];
        std::ostringstream os;
        os << WIDTH_PREFIX << i;
        const double gaussWidth(getParameter(os.str()));
        const double lorentzWidth(lorentzW[i]);
        const double gaussRes = sigmaRes[i];
        if(i == 0)
        {
          const double amp(1.0);
          firstMassJ(j1i, yi, modQ, amp, kfse, gaussWidth, lorentzWidth, gaussRes);
        }
        else
        {
          const double lorentzPos(0.0), lorentzAmp(1.0), lorentzFWHM(lorentzWidth);
          const double gaussFWHM = std::sqrt(std::pow(gaussRes,2) + std::pow(2.0*STDDEV_TO_HWHM*gaussWidth,2));
          voigtApprox(j1i, yi,lorentzPos, lorentzAmp, lorentzFWHM, gaussFWHM); // Answer goes into j1i
          voigtApproxDiff(voigtDiffResult, yi, lorentzPos, lorentzAmp, lorentzFWHM, gaussFWHM);
          for(size_t j = 0; j < nData; ++j)
          {
            const double factor = std::pow(gaussWidth,4.0)/(3.0*modQ[j]);
            j1i[j] -= factor*voigtDiffResult[j];
          }
        }
        // Multiply by mass
        std::transform(j1i.begin(), j1i.end(), j1i.begin(), std::bind2nd(std::multiplies<double>(), m_masses[i]));
      }

      // Sum over each mass and scale by prefactor to get answer
      for(size_t j = 0; j < nData; ++j)
      {
        for(size_t i = 0; i < nmasses; ++i)
        {
          out[j] += j1[i][j];
        }
        out[j] *= std::pow(e0[j],0.1)/modQ[j];
      }
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
     * The input string gives whether each even Hermite polnomial is active or not
     */
    void NCSCountRate::declareGramCharlierParameters()
    {
      // Gram-Chelier parameters are the even coefficents of the Hermite polynomials, i.e
      // setting hermite coefficients to "1 0 1" uses coefficents C_0,C_4 and C_2 is skipped.
      // Still require width parameter
      this->declareParameter(std::string(WIDTH_PREFIX) + "0",1.0, "Width of zeroth mass");
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

    namespace
    {
      ///@cond
      double trapzf(const std::vector<double> & xv, const std::vector<double> & yv)
      {
        const double stepsize = xv[1] - xv[0];
        const size_t endpoint = xv.size() - 1;
        double area(0.0);
        for(size_t i = 1; i < endpoint; i++)
        {
          area += yv[i];
        }
        area = stepsize/2*(yv[0] + 2*area + yv[endpoint]); // final step
        return area;
      }
      ///@endcond
    }

    /**
     * Includes the hermite expansion within the gaussian momentum distribution
     * @param j1 Output vector for the final values
     * @param yspace Y-space input values
     * @param modQ Values of modQ for each y value
     */
    void NCSCountRate::firstMassJ(std::vector<double> & j1, const std::vector<double> & yspace,
                                  const std::vector<double> & modQ, const double amp, const double kfse,
                                  const double wg, const double wl, const double wgRes) const
    {
      using namespace Mantid::Kernel;
      // First compute product of gaussian momentum distribution with Hermite polynomials.
      // This is done over an interpolated range between ymin & ymax and y and hence q must be sorted
      // Sort needs to sort y and put q in the right order based on the new order. Use a struct that will keep the y/q together
      // and sort on y
      struct Point
      {
        Point(double yvalue = 0.0, double qvalue = 0.0) : y(yvalue), q(qvalue) {}
        double y;
        double q;
      };
      const size_t ncoarseY(yspace.size());
      std::vector<Point> points(ncoarseY);
      for(size_t i = 0; i < ncoarseY; ++i)
      {
        points[i] = Point(yspace[i], modQ[i]);
      }
      struct InY
      {
        /// Sort by the y field
        bool operator()(Point const &a, Point const &b) { return a.y < b.y; }
      };
      std::sort(points.begin(), points.end(), InY());
      // Separate back into vectors as GSL requires them separate
      std::vector<double> sortedy(ncoarseY), sortedq(ncoarseY);
      for(size_t i = 0; i < ncoarseY; ++i)
      {
        const auto & p = points[i];
        sortedy[i] = p.y;
        sortedq[i] = p.q;
      }

      // Generate a more-finely grained y axis and interpolate Q values
      const int nfineY(1000);
      const double miny(sortedy.front()), maxy(sortedy.back());
      const double step = (maxy-miny)/static_cast<double>((nfineY-1));
      std::vector<double> yfine(nfineY),qfine(nfineY);
      // Set up GSL interpolater
      gsl_interp_accel *acc = gsl_interp_accel_alloc ();
      gsl_spline *spline = gsl_spline_alloc(gsl_interp_linear, ncoarseY); // Actually a linear interpolater
      gsl_spline_init(spline, sortedy.data(), sortedq.data(), ncoarseY);
      for(int i = 0; i < nfineY - 1; ++i)
      {
        const double xi = miny + step*i;
        yfine[i] = xi;
        qfine[i] = gsl_spline_eval(spline, xi, acc);
      }
      // Final value to ensure it ends at maxy
      yfine.back() = maxy;
      qfine.back() = gsl_spline_eval(spline, maxy, acc);
      gsl_spline_free(spline);
      gsl_interp_accel_free(acc);

      // Hermite expansion (only even terms) + FSE term
      const size_t nhermite(m_hermite.size());
      const size_t njm(nhermite + 1);
      std::vector<std::vector<double>> jMd(njm, std::vector<double>(nfineY, 0.0));

      const double ampNorm = amp/(std::sqrt(2.0*M_PI)*wg);
      // Sum over polynomials for each y
      std::vector<double> sumJM(nfineY);
      for(unsigned int i = 0; i < nhermite; ++i)
      {
        if(m_hermite[i] == 0) continue;
        auto & jMdi = jMd[i];
        const unsigned int npoly = 2*i; // Only even ones
        std::ostringstream os;
        os << HERMITE_PREFIX << npoly;
        const double hermiteCoeff = getParameter(os.str());
        for(int j = 0; j < nfineY; ++j)
        {
          const double y = yfine[j]/std::sqrt(2.)/wg;
          const double hermiteI = Math::hermitePoly(npoly,y);
          const double factorial = gsl_sf_fact(i);
          jMdi[j] = ampNorm*std::exp(-y*y)*hermiteI*hermiteCoeff/((std::pow(2.0,npoly))*factorial);
          sumJM[j] += jMdi[j];
        }
      }
      // Plus the FSE term
      auto & jMdfse = jMd.back();
      for(int j = 0; j < nfineY; ++j)
      {
        const double y = yfine[j]/std::sqrt(2.)/wg;
        const double he3 = Math::hermitePoly(3,y);
        jMdfse[j] = ampNorm*std::exp(-y*y)*he3*(kfse/qfine[j]);
      }

      // Now convolute with the Voigt function
      std::vector<double> minusYFine(nfineY);
      std::transform(yfine.begin(), yfine.end(), minusYFine.begin(), std::bind2nd(std::multiplies<double>(), -1.0));
      std::vector<double> ym(nfineY); // Holds result of (y[i] - yfine) for each original y
      std::vector<double> voigt(nfineY); // Holds results of voigt calculation
      for(size_t i = 0; i < ncoarseY; ++i)
      {
        const double yi = yspace[i];
        std::transform(minusYFine.begin(), minusYFine.end(), ym.begin(), std::bind2nd(std::plus<double>(), yi)); //yfine is actually -yfine
        voigtApprox(voigt,yfine,0,1,wl,wgRes);
        // Multipy voigt with polynomial sum and put result in voigt to save using another vector
        std::transform(voigt.begin(), voigt.end(), sumJM.begin(), voigt.begin(), std::multiplies<double>());
        j1[i] = trapzf(yfine, voigt);
      }
    }

    /**
     * Transforms the input y coordinates using a difference if Voigt functions across the whole range
     * @param voigtDiff [Out] Output values (vector is expected to be of the correct size)
     * @param yspace Input y coordinates
     * @param lorentzPos LorentzPos parameter
     * @param lorentzAmp LorentzAmp parameter
     * @param lorentzWidth LorentzFWHM parameter
     * @param gaussWidth GaussianFWHM parameter
     */
    void NCSCountRate::voigtApproxDiff(std::vector<double> & voigtDiff, const std::vector<double> & yspace, const double lorentzPos, const double lorentzAmp,
                                       const double lorentzWidth, const double gaussWidth) const
    {
      double miny(DBL_MAX), maxy(-DBL_MAX);
      auto iend = yspace.end();
      for(auto itr = yspace.begin(); itr != iend; ++itr)
      {
        const double absy = std::abs(*itr);
        if(absy < miny) miny = absy;
        else if(absy > maxy) maxy = absy;
      }
      const double epsilon = (maxy - miny)/1000.0;

      // Compute: V = voigt(y+2eps,...) - voigt(y-2eps,...) - 2*voigt(y+eps,...) + 2*(voigt(y-eps,...)/(2eps^3))

      std::vector<double> ypmEps(yspace.size());
      // y+2eps
      std::transform(yspace.begin(), yspace.end(), ypmEps.begin(), std::bind2nd(std::plus<double>(), 2.0*epsilon)); // Add 2 epsilon
      voigtApprox(voigtDiff, ypmEps, lorentzPos, lorentzAmp, lorentzWidth, gaussWidth);
      // y-2eps
      std::transform(yspace.begin(), yspace.end(), ypmEps.begin(), std::bind2nd(std::minus<double>(), 2.0*epsilon)); // Subtract 2 epsilon
      std::vector<double> tmpResult(yspace.size());
      voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp, lorentzWidth, gaussWidth);
      // Difference of first two terms - result is put back in voigtDiff
      std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(), voigtDiff.begin(), std::minus<double>());

      // y+eps
      std::transform(yspace.begin(), yspace.end(), ypmEps.begin(), std::bind2nd(std::plus<double>(), epsilon)); // Add 2 epsilon
      voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp, lorentzWidth, gaussWidth);
      std::transform(tmpResult.begin(), tmpResult.end(), tmpResult.begin(), std::bind2nd(std::multiplies<double>(), 2.0)); // times 2
      // Difference - result is put back in voigtDiff
      std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(), voigtDiff.begin(), std::minus<double>());

      //y-eps
      std::transform(yspace.begin(), yspace.end(), ypmEps.begin(), std::bind2nd(std::minus<double>(), epsilon)); // Add 2 epsilon
      voigtApprox(tmpResult, ypmEps, lorentzPos, lorentzAmp, lorentzWidth, gaussWidth);
      std::transform(tmpResult.begin(), tmpResult.end(), tmpResult.begin(), std::bind2nd(std::divides<double>(), std::pow(epsilon,3))); // divided by (eps^3)
      // Sum for final answer
      std::transform(voigtDiff.begin(), voigtDiff.end(), tmpResult.begin(), voigtDiff.begin(), std::plus<double>());
    }

    /**
     * Transforms the input y coordinates using the Voigt function approximation
     * @param voigt [Out] Output values (vector is expected to be of the correct size
     * @param yspace Input y coordinates
     * @param lorentzPos LorentzPos parameter
     * @param lorentzAmp LorentzAmp parameter
     * @param lorentzWidth LorentzFWHM parameter
     * @param gaussWidth GaussianFWHM parameter
     */
    void NCSCountRate::voigtApprox(std::vector<double> & voigt, const std::vector<double> & yspace, const double lorentzPos,
                                   const double lorentzAmp, const double lorentzWidth, const double gaussWidth) const
    {
      m_voigt->setParameter("LorentzAmp",lorentzAmp);
      m_voigt->setParameter("LorentzPos",lorentzPos);
      m_voigt->setParameter("LorentzFWHM",lorentzWidth);
      m_voigt->setParameter("GaussianFWHM",gaussWidth);
      assert(voigt.size() == yspace.size());
      m_voigt->function1D(voigt.data(), yspace.data(), yspace.size());
    }

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
