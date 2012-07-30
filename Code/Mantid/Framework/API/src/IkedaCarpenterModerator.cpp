//
// Includes
//
#include "MantidAPI/IkedaCarpenterModerator.h"

#include "MantidKernel/Exception.h"
#include <gsl/gsl_roots.h>
#include <gsl/gsl_errno.h>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <cmath>
#include <sstream>
#include <cassert>

namespace Mantid
{
  namespace API
  {
    namespace
    {
      /// Minima finding tolerance
      const double MinimaFindingTolerance = 1e-10;

      /// Parameters
      const char * TILT_ANGLE_NAME = "TiltAngle";
      const char * TAU_F_NAME = "TauF";
      const char * TAU_S_NAME = "TauS";
      const char * R_NAME = "R";
    }

    /// Default constructor required by the factory
    IkedaCarpenterModerator::IkedaCarpenterModerator()
      : ModeratorModel(), m_tau_f(0.0), m_tau_s(0.0), m_r(0.0), m_lookupSize(1000),
        m_areaToTimeLookup(), m_offset(0.0)
    {
    }

    /// Returns a clone of the current object
    boost::shared_ptr<ModeratorModel> IkedaCarpenterModerator::clone() const
    {
      return boost::make_shared<IkedaCarpenterModerator>(*this);
    }

    /**
     * Sets the value of the \f$\alpha\f$ parameter
     * @param value :: A value for the fast decay coefficient in seconds
     */
    void IkedaCarpenterModerator::setFastDecayCoefficent(const double value)
    {
      m_tau_f = value;
    }

    /**
     * @returns Returns the value of the \f$\alpha\f$ parameter in seconds
     */
    double IkedaCarpenterModerator::getFastDecayCoefficent() const
    {
      return m_tau_f;
    }

    /**
     * Sets the value of the \f$\beta\f$ parameter
     * @param value :: A value for the slow decay coefficient in seconds
     */
    void IkedaCarpenterModerator::setSlowDecayCoefficent(const double value)
    {
      m_tau_s = value;
    }

    /**
     * @returns Returns the value of the \f$\beta\f$ parameter in seconds
     */
    double IkedaCarpenterModerator::getSlowDecayCoefficent() const
    {
      return m_tau_s;
    }

    /**
     * Sets the value of the \f$R\f$ parameter
     * @param value :: A value for the mixing coefficient
     */
    void IkedaCarpenterModerator::setMixingCoefficient(const double value)
    {
      m_r = value;
    }

    /**
     * @returns Returns the value of the \f$\beta\f$ parameter in seconds
     */
    double IkedaCarpenterModerator::getMixingCoefficient() const
    {
      return m_r;
    }


    /**
     * Mean emission time: \f$\tau_mean=3*\tau_f + R\tau_s\f$
     *  @returns The mean time for emission in seconds
     */
    double IkedaCarpenterModerator::emissionTimeMean() const
    {
      return 3.0*m_tau_f + m_r*m_tau_s;
    }

    /**
     * Variance of emission time: \f$\tau_sig=\sqrt{3*\tau_f^2 + R(2-R)*\tau_s^2}\f$
     *  @returns The spread of emission times in seconds^2
     */
    double IkedaCarpenterModerator::emissionTimeVariance() const
    {
      return 3.0*m_tau_f*m_tau_f + m_r*(2.0-m_r)*m_tau_s*m_tau_s;
    }

    /**
     * Returns a time, in seconds, sampled from the distibution given a flat random number
     * @param flatRandomNo :: A random number in the range [0,1]
     * @return A time, in seconds, sampled from the distribution
     */
    double IkedaCarpenterModerator::sampleTimeDistribution(const double flatRandomNo) const
    {
      if(flatRandomNo >= 0.0 && flatRandomNo <= 1.0)
      {
        const double mean = emissionTimeMean();
        const double x = std::min(0.999, interpolateAreaTable(flatRandomNo));
        return mean * (2.0*x - 1.0)/(1.0 - x);
      }
      else
      {
        std::ostringstream os;
        os << "IkedaCarpenterModerator::sampleTimeDistribution - Random number must be flat between [0,1]. Current value=" << flatRandomNo;
        throw std::invalid_argument(os.str());
      }
    }

    //----------------------------------------------------------------------------------------
    // Private members
    //----------------------------------------------------------------------------------------
    /**
     * Sets a parameter from a name & string value
     * @param name :: The name of the parameter
     * @param value :: The value as a string
     */
    void IkedaCarpenterModerator::setParameterValue(const std::string & name, const std::string & value)
    {
      const double valueAsDbl = boost::lexical_cast<double>(value);
      if(name == TILT_ANGLE_NAME)
      {
        setTiltAngleInDegrees(valueAsDbl);
      }
      else if(name == TAU_F_NAME)
      {
        setFastDecayCoefficent(valueAsDbl);
      }
      else if(name == TAU_S_NAME)
      {
        setSlowDecayCoefficent(valueAsDbl);
      }
      else if(name == R_NAME)
      {
        setMixingCoefficient(valueAsDbl);
      }
      else
      {
        throw std::invalid_argument("IkedaCarpenterModerator::setParameterValue - Unknown parameter: " + name);
      }
    }

    /**
     * Initialize the area-to-time lookup table
     */
    void IkedaCarpenterModerator::initLookupTable()
    {
      m_areaToTimeLookup.resize(m_lookupSize);
      for(unsigned int i = 0; i < m_lookupSize; ++i)
      {
        const double area = static_cast<double>(i)/(m_lookupSize-1);
        const double fraction = areaToTime(area);
        m_areaToTimeLookup[i] = fraction;
      }
    }

    /**
     * Return the value of time interpolated from the given time using the cached table
     * @param area The value of the area to interpolate from
     * @return The time corresponding to this area linearly interpolated from the cached table
     */
    double IkedaCarpenterModerator::interpolateAreaTable(const double area) const
    {
      if(m_areaToTimeLookup.empty())
      {
        const_cast<IkedaCarpenterModerator*>(this)->initLookupTable();
      }
      const unsigned int nsteps = (m_lookupSize - 1);
      const unsigned int indexBelow = static_cast<unsigned int>(std::floor(area*nsteps));
      const double step = area*nsteps - indexBelow;
      return m_areaToTimeLookup[indexBelow]*(1.0 - step) + m_areaToTimeLookup[indexBelow + 1]*step;
    }


    /**
     * Returns the value of T such that the integral of a normalised Ikeda-Carpenter function, M(T), from 0 to T = area
     * @param area :: The required area value
     * @return The value of T such that the integral of M(T) equals the given area
     */
    double IkedaCarpenterModerator::areaToTime(const double area) const
    {
      m_offset = area;
      const double rangeMin(0.0), rangeMax(1.0);
      if(area <= rangeMin)
      {
        return rangeMin;
      }
      else if(area >= rangeMax)
      {
        return rangeMax;
      }
      else
      {
        return findMinumum(rangeMin, rangeMax, MinimaFindingTolerance);
      }
    }


    /**
     * Find the minimum of the areaToTimeFunction between the given interval with the given tolerance
     * @param rangeMin :: The start of the range where the function changes sign
     * @param rangeMax :: The end of the range where the function changes sign
     * @param tolerance :: The required tolerance
     * @return The location of the minimum
     */
    double IkedaCarpenterModerator::findMinumum(const double rangeMin, const double rangeMax, const double tolerance) const
    {
      // Define helper structs for GSL root finding.
      // GSL requires a params struct + a function pointer to minize. Our function
      // is a method so we create a params object with a pointer to this object
      // that can call the necessary areaToTimeFunction from a static method
      // whose address can be passed to the GSL solver
      struct FunctionParams
      {
        const IkedaCarpenterModerator & moderator;
      };
      struct FunctionCallback
      {
        static double function(double x, void *params)
        {
          struct FunctionParams *p = (struct FunctionParams*)params;
          return p->moderator.areaToTimeFunction(x);
        }
      };

      gsl_function rootFindingFunction;
      rootFindingFunction.function = &FunctionCallback::function;
      struct FunctionParams params = { *this };
      rootFindingFunction.params = &params;

      const gsl_root_fsolver_type *solverType = gsl_root_fsolver_brent;
      gsl_root_fsolver *solver = gsl_root_fsolver_alloc(solverType);
      gsl_root_fsolver_set (solver, &rootFindingFunction, rangeMin, rangeMax);

      int status;
      int iter = 0, max_iter = 100;
      double root(0.0);
      do
      {
        iter++;
        status = gsl_root_fsolver_iterate (solver);
        root = gsl_root_fsolver_root (solver);
        const double xlo = gsl_root_fsolver_x_lower (solver);
        const double xhi = gsl_root_fsolver_x_upper (solver);
        status = gsl_root_test_interval (xlo, xhi, 0, tolerance);
      }
      while (status == GSL_CONTINUE && iter < max_iter);

      gsl_root_fsolver_free(solver);
      return root;
    }

    /**
     * Function to pass to root-finder to find the value of the area for the given fraction of the range
     * @param fraction ::
     * @return The value of the area for the given fraction with in the range
     */
    double IkedaCarpenterModerator::areaToTimeFunction(const double fraction) const
    {
      if(fraction <= 0.0)
      {
        return -m_offset;
      }
      else if(fraction >= 1.0)
      {
        return 1.0 - m_offset;
      }
      else
      {
        const double time = this->emissionTimeMean()*fraction/(1.0 - fraction);
        return area(time) - m_offset;
      }
    }

    /**
     * Calculates the area of the Ikeda-Carpenter moderator lineshape integrated from 0 to x.
     * @param x :: The end-point of the integration
     * @return The value of the area
     */
    double IkedaCarpenterModerator::area(const double x) const
    {
      static const double c3 = 1.6666666666666666667e-01, c4 =-1.2500000000000000000e-01, c5 = 5.0000000000000000000e-02,
                          c6 =-1.3888888888888888889e-02, c7 = 2.9761904761904761905e-03, c8 =-5.2083333333333333333e-04,
                          c9 = 7.7160493827160493827e-05, c10 =-9.9206349206349206349e-06, c11= 1.1273448773448773449e-06,
                          c12 =-1.1482216343327454439e-07, c13 = 1.0598968932302265636e-08;

      if(x >= 0.0)
      {
        if(m_tau_f != 0.0)
        {
          const double ax=x/m_tau_f;
          double funAx, funGx;
          if(std::fabs(ax)<=0.1)
          {
            funAx = c3+ax*(c4+ax*(c5+ax*(c6+ax*(c7+ax*(c8+ax*(c9+ax*(c10+ax*(c11+ax*(c12+ax*c13)))))))));
          }
          else
            funAx = (1.0 - exp(-(ax))*(1.0+(ax)+0.5*(ax*ax))) / (ax*ax*ax);
          if(m_tau_s!=0. && m_r!=0.)
          {
            const double gx = x*(1.0/m_tau_f - 1.0/m_tau_s);
            if(gx<0.1)
              funGx = c3+gx*(c4+gx*(c5+gx*(c6+gx*(c7+gx*(c8+gx*(c9+gx*(c10+gx*(c11+gx*(c12+gx*c13)))))))));
            else
              funGx = (1.0 - exp(-(gx))*(1.0+(gx)+0.5*(gx*gx))) / (gx*gx*gx);
            return((ax*ax*ax)*(funAx - m_r*funGx*std::exp(-(x/m_tau_s))));
          }
          else
            return((ax*ax*ax)*funAx);
        }
        if(m_tau_s != 0. && m_r != 0.)
          return((1.0-m_r) + m_r*(1.0-std::exp(-(x/m_tau_s))));
        else
          return(1.);
      }
      return(0.);
    }


  }
}
