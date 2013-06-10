//------------------------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------------------------
#include "MantidCurveFitting/GramCharlierComptonProfile.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Math/Distributions/HermitePolynomials.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_sf_gamma.h> // for factorial
#include <gsl/gsl_spline.h>

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
      ///@endcond
    }

    /**
     */
    GramCharlierComptonProfile::GramCharlierComptonProfile()
      : ComptonProfile(), m_hermite()
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

    namespace
    {
      ///@cond
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
     * Uses a Gram-Charlier series approximation for the mass and convolutes it with the Voigt
     * instrument resolution function
     * @param result An pre-sized output vector that should be filled with the results
     */
    void GramCharlierComptonProfile::massProfile(std::vector<double> & result) const
    {
      using namespace Mantid::Kernel;
      const auto & yspace = ySpace();
      const auto & modq = modQ();

      // First compute product of gaussian momentum distribution with Hermite polynomials.
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

      const double amp(1.0), wg(getParameter(WIDTH_PARAM));
      const double ampNorm = amp/(std::sqrt(2.0*M_PI)*wg);
      // Sum over polynomials for each y
      std::vector<double> sumH(nfineY);
      for(unsigned int i = 0; i < nhermite; ++i)
      {
        if(m_hermite[i] == 0) continue;

        const int npoly = 2*i; // Only even ones
        std::ostringstream os;
        os << HERMITE_PREFIX << npoly;
        const double hermiteCoeff = getParameter(os.str());
        for(int j = 0; j < nfineY; ++j)
        {
          const double y = yfine[j]/std::sqrt(2.)/wg;
          const double hermiteI = Math::hermitePoly(npoly,y);
          const double factorial = gsl_sf_fact(i);
          sumH[j] += ampNorm*std::exp(-y*y)*hermiteI*hermiteCoeff/((std::pow(2.0,npoly))*factorial);
        }
      }

      // Plus the FSE term
      const double kfse = getParameter(KFSE_NAME);
      for(int j = 0; j < nfineY; ++j)
      {
        const double y = yfine[j]/std::sqrt(2.)/wg;
        const double he3 = Math::hermitePoly(3,y);
        sumH[j] += ampNorm*std::exp(-y*y)*he3*(kfse/qfine[j]);
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
        voigtApprox(voigt,ym,0,1.0,lorentzFWHM(),resolutionFWHM());
        // Multiply voigt with polynomial sum and put result in voigt to save using another vector
        std::transform(voigt.begin(), voigt.end(), sumH.begin(), voigt.begin(), std::multiplies<double>());
        result[i] = trapzf(yfine, voigt);
      }

    }

  } // namespace CurveFitting
} // namespace Mantid
