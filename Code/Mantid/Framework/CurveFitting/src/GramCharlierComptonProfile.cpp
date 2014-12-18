//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/GramCharlierComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Math/Distributions/HermitePolynomials.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_sf_gamma.h> // for factorial
#include <gsl/gsl_spline.h>

#include <sstream>

namespace Mantid
{
  namespace CurveFitting
  {
    // Register into factory
    DECLARE_FUNCTION(GramCharlierComptonProfile);

    namespace
    {
      ///@cond
      const char * WIDTH_PARAM = "Width";
      const char * HERMITE_PREFIX = "C_";
      const char * KFSE_NAME = "FSECoeff";
      const char * HERMITE_C_NAME = "HermiteCoeffs";
      const int NFINE_Y = 1000;

      /**
       * Computes the area under the curve given a set of [X,Y] points using
       * the trapezoidal method of integration
       * @param xv Vector of x values
       * @param yv Vector of y values
       * @returns The value of the area
       */
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

      // Cannot put these inside massProfile definition as you can't use local types as template
      // arguments on some compilers

      // massProfile needs to sort 1 vector but keep another in order. Use a struct of combined points
      // and a custom comparator
      struct Point
      {
        Point(double yvalue = 0.0, double qvalue = 0.0) : y(yvalue), q(qvalue) {}
        double y;
        double q;
      };

      struct InY
      {
        /// Sort by the y field
        bool operator()(Point const &a, Point const &b) { return a.y < b.y; }
      };
      ///@endcond
    }

    /**
     */
    GramCharlierComptonProfile::GramCharlierComptonProfile()
      : ComptonProfile(), m_hermite(), m_yfine(), m_qfine(), m_voigt(), m_voigtProfile(), m_userFixedFSE(false)
    {
    }

    /**
     * @returns A string containing the name of the function
     */
    std::string GramCharlierComptonProfile::name() const
    {
      return "GramCharlierComptonProfile";
    }

    /**
     */
    void GramCharlierComptonProfile::declareParameters()
    {
      // Base class ones
      ComptonProfile::declareParameters();
      declareParameter(WIDTH_PARAM, 1.0, "Gaussian width parameter");
      declareParameter(KFSE_NAME, 1.0, "FSE coefficient k");
      // Other parameters depend on the Hermite attribute...
    }

    /**
     */
    void GramCharlierComptonProfile::declareAttributes()
    {
      // Base class ones
      ComptonProfile::declareAttributes();
      declareAttribute(HERMITE_C_NAME, IFunction::Attribute("")); // space-separated string 1/0 indicating which coefficients are active
    }

    /**
     * @param name The name of the attribute
     * @param value The attribute's value
     */
    void GramCharlierComptonProfile::setAttribute(const std::string& name,const Attribute& value)
    {
      if(name == HERMITE_C_NAME) setHermiteCoefficients(value.asString());
      ComptonProfile::setAttribute(name, value);
    }

    /**
     * Throws if the string is empty and contains something other than numbers
     * @param coeffs A string of space separated 1/0 values indicating which polynomial coefficients to include in the fitting
     */
    void GramCharlierComptonProfile::setHermiteCoefficients(const std::string & coeffs)
    {
      if(coeffs.empty())
      {
        throw std::invalid_argument("GramCharlierComptonProfile - Hermite polynomial string is empty!");
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
    void GramCharlierComptonProfile::declareGramCharlierParameters()
    {
      // Gram-Chelier parameters are the even coefficents of the Hermite polynomials, i.e
      // setting hermite coefficients to "1 0 1" uses coefficents C_0,C_4 and C_2 is skipped.
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

    /*
     */
    std::vector<size_t> GramCharlierComptonProfile::intensityParameterIndices() const
    {
      assert(!m_hermite.empty());

      std::vector<size_t> indices;
      indices.reserve(m_hermite.size()+1);
      for(size_t i = 0; i < m_hermite.size(); ++i)
      {
        if(m_hermite[i] > 0)
        {
          std::ostringstream os;
          os << HERMITE_PREFIX << 2*i; //refactor to have method that produces the name
          indices.push_back(this->parameterIndex(os.str()));
        }
      }
      // Include Kfse if it is not fixed
      const size_t kIndex = this->parameterIndex(KFSE_NAME);
      if(!isFixed(kIndex))
      {
        indices.push_back(kIndex);
      }

      return indices;
    }

    /**
     * Fills in a column for each active hermite polynomial, starting at the given index
     * @param cmatrix InOut matrix whose columns should be set to the mass profile for each active hermite polynomial
     * @param start Index of the column to start on
     * @param errors Data errors array
     * @returns The number of columns filled
     */
    size_t GramCharlierComptonProfile::fillConstraintMatrix(Kernel::DblMatrix & cmatrix, const size_t start,
                                                            const std::vector<double>& errors) const
    {
      std::vector<double> profile(NFINE_Y, 0.0);
      const size_t nData(ySpace().size());
      std::vector<double> result(nData, 0.0);

      // If the FSE term is fixed by user then it's contribution is convoluted with Voigt and summed with the first column
      // otherwise it gets a column of it's own at the end.
      // Either way it needs to be computed, so do this first

      std::vector<double> fse(NFINE_Y, 0.0);
      std::vector<double> convolvedFSE(nData,0.0);
      addFSETerm(fse);
      convoluteVoigt(convolvedFSE.data(), nData, fse);

      size_t col(0);
      for(unsigned int i = 0; i < m_hermite.size(); ++i)
      {
        if(m_hermite[i] == 0) continue;

        const unsigned int npoly = 2*i;
        addMassProfile(profile.data(), npoly);
        convoluteVoigt(result.data(), nData, profile);
        if(i == 0 && m_userFixedFSE)
        {
          std::transform(result.begin(), result.end(), convolvedFSE.begin(), result.begin(), std::plus<double>());
        }
        std::transform(result.begin(), result.end(), errors.begin(), result.begin(), std::divides<double>());
        cmatrix.setColumn(start + col, result);

        std::fill_n(profile.begin(), NFINE_Y, 0.0);
        std::fill_n(result.begin(), nData, 0.0);
        ++col;
      }

      if(!m_userFixedFSE) // Extra column for He3
      {
        std::transform(convolvedFSE.begin(), convolvedFSE.end(), errors.begin(), convolvedFSE.begin(), std::divides<double>());
        cmatrix.setColumn(start + col, convolvedFSE);
        ++col;
      }
      return col;
    }

    /**
     * Uses a Gram-Charlier series approximation for the mass and convolutes it with the Voigt
     * instrument resolution function. Also multiplies by the mass*e_i^0.1/q
     * @param result An pre-sized output array that should be filled with the results
     * @param nData The length of the array
     */
    void GramCharlierComptonProfile::massProfile(double * result, const size_t nData) const
    {
      UNUSED_ARG(nData);
      using namespace Mantid::Kernel;

      // Hermite expansion (only even terms) + FSE term
      const size_t nhermite(m_hermite.size());

      // Sum over polynomials for each y
      std::vector<double> sumH(NFINE_Y);
      for(unsigned int i = 0; i < nhermite; ++i)
      {
        if(m_hermite[i] == 0) continue;
        const unsigned int npoly = 2*i; // Only even ones
        addMassProfile(sumH.data(), npoly);
      }
      addFSETerm(sumH);
      convoluteVoigt(result, nData, sumH);
    }

    /**
     * Uses a Gram-Charlier series approximation for the mass and convolutes it with the Voigt
     * instrument resolution function. Also multiplies by the mass*e_i^0.1/q. Sums it with the given result
     * @param result An pre-sized output array that should be filled with the results. Size is fixed at NFINE_Y
     * @param npoly An integer denoting the polynomial to calculate
     */
    void GramCharlierComptonProfile::addMassProfile(double * result, const unsigned int npoly) const
    {
      using namespace Mantid::Kernel;

      const double amp(1.0), wg(getParameter(WIDTH_PARAM));
      const double ampNorm = amp/(std::sqrt(2.0*M_PI)*wg);

      std::ostringstream os;
      os << HERMITE_PREFIX << npoly;
      const double hermiteCoeff = getParameter(os.str());
      const double factorial = gsl_sf_fact(npoly/2);
      // Intel compiler doesn't overload pow for unsigned types
      const double denom = ((std::pow(2.0, static_cast<int>(npoly)))*factorial);

      for(int j = 0; j < NFINE_Y; ++j)
      {
        const double y = m_yfine[j]/std::sqrt(2.)/wg;
        const double hermiteI = Math::hermitePoly(npoly,y);
        result[j] += ampNorm*std::exp(-y*y)*hermiteI*hermiteCoeff/denom;
      }
    }

    /**
     * Adds the FSE term to the result in the vector given
     * @param lhs Existing vector that the result should be added to
     */
    void GramCharlierComptonProfile::addFSETerm(std::vector<double> & lhs) const
    {
      assert(static_cast<size_t>(NFINE_Y) == lhs.size());
      using namespace Mantid::Kernel;

      const double amp(1.0), wg(getParameter(WIDTH_PARAM));
      const double ampNorm = amp/(std::sqrt(2.0*M_PI)*wg);

      double kfse(getParameter(KFSE_NAME));
      if(m_userFixedFSE) kfse *= getParameter("C_0");

      for(int j = 0; j < NFINE_Y; ++j)
      {
        const double y = m_yfine[j]/std::sqrt(2.)/wg;
        const double he3 = Math::hermitePoly(3,y);
        lhs[j] += ampNorm*std::exp(-y*y)*he3*(kfse/m_qfine[j]);
      }
    }

    /**
     * Convolute with resolution and multiply by the final ei^0.1*mass/q prefactor
     * @param result Output array that holds the result of the convolution
     * @param nData The length of the array
     * @param profile The input mass profile
     */
    void GramCharlierComptonProfile::convoluteVoigt(double * result, const size_t nData, const std::vector<double> & profile) const
    {
      const auto & modq = modQ();
      const auto & ei = e0();

      // Now convolute with the Voigt function (pre-calculated in setWorkspace as its expensive)
      for(size_t i = 0; i < nData; ++i)
      {
        const std::vector<double> & voigt = m_voigt[i];
        // Multiply voigt with polynomial sum and put result in voigt to save using another vector
        std::transform(voigt.begin(), voigt.end(), profile.begin(), m_voigtProfile.begin(), std::multiplies<double>());
        const double prefactor = std::pow(ei[i],0.1)*mass()/modq[i];
        result[i] = prefactor*trapzf(m_yfine, m_voigtProfile);
      }

    }

    /**
     * Used to cache some values when the workspace has been set
     * @param workspace A pointer to the workspace
     * @param wi A workspace index
     * @param startX Starting x-vaue (unused).
     * @param endX Ending x-vaue (unused).
     */
    void GramCharlierComptonProfile::setMatrixWorkspace(boost::shared_ptr<const API::MatrixWorkspace> workspace,size_t wi,double startX, double endX)
    {
      ComptonProfile::setMatrixWorkspace(workspace,wi,startX,endX); // Do base-class calculation first
    }

    /**
     * @param tseconds A vector containing the time-of-flight values in seconds
     * @param isHistogram True if histogram tof values have been passed in
     * @param detpar Structure containing detector parameters
     */
    void GramCharlierComptonProfile::cacheYSpaceValues(const std::vector<double> & tseconds, const bool isHistogram,
                                                       const DetectorParams & detpar)
    {
      ComptonProfile::cacheYSpaceValues(tseconds,isHistogram,detpar); // base-class calculations

      // Is FSE fixed at the moment?
      // The ComptonScatteringCountRate fixes it but we still need to know if the user wanted it fixed
      m_userFixedFSE = this->isFixed(this->parameterIndex(KFSE_NAME));

      const auto & yspace = ySpace();
      const auto & modq = modQ();

      // massProfile is calculated over a large range of Y, constructed by interpolation
      // This is done over an interpolated range between ymin & ymax and y and hence q must be sorted
      const size_t ncoarseY(yspace.size());
      std::vector<Point> points(ncoarseY);
      for(size_t i = 0; i < ncoarseY; ++i)
      {
        points[i] = Point(yspace[i], modq[i]);
      }
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
      m_yfine.resize(NFINE_Y);
      m_qfine.resize(NFINE_Y);
      const double miny(sortedy.front()), maxy(sortedy.back());
      const double step = (maxy-miny)/static_cast<double>((NFINE_Y-1));

      // Set up GSL interpolater
      gsl_interp_accel *acc = gsl_interp_accel_alloc ();
      gsl_spline *spline = gsl_spline_alloc(gsl_interp_linear, ncoarseY); // Actually a linear interpolater
      gsl_spline_init(spline, sortedy.data(), sortedq.data(), ncoarseY);
      for(int i = 0; i < NFINE_Y - 1; ++i)
      {
        const double xi = miny + step*i;
        m_yfine[i] = xi;
        m_qfine[i] = gsl_spline_eval(spline, xi, acc);
      }
      // Final value to ensure it ends at maxy
      m_yfine.back() = maxy;
      m_qfine.back() = gsl_spline_eval(spline, maxy, acc);
      gsl_spline_free(spline);
      gsl_interp_accel_free(acc);

      // Cache voigt function over yfine
      std::vector<double> minusYFine(NFINE_Y);
      std::transform(m_yfine.begin(), m_yfine.end(), minusYFine.begin(), std::bind2nd(std::multiplies<double>(), -1.0));
      std::vector<double> ym(NFINE_Y); // Holds result of (y[i] - yfine) for each original y
      m_voigt.resize(ncoarseY);

      for(size_t i = 0; i < ncoarseY; ++i)
      {
        std::vector<double> & voigt = m_voigt[i];
        voigt.resize(NFINE_Y);

        const double yi = yspace[i];
        std::transform(minusYFine.begin(), minusYFine.end(), ym.begin(), std::bind2nd(std::plus<double>(), yi)); //yfine is actually -yfine
        m_resolutionFunction->voigtApprox(voigt,ym,0,1.0);
      }

      m_voigtProfile.resize(NFINE_Y); // Value holder for later to avoid repeated memory allocations when creating a new vector
    }

  } // namespace CurveFitting
} // namespace Mantid
