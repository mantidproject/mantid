//
// Includes
//
#include "MantidAPI/IkedaCarpenterModerator.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/MultiThreaded.h"
#include <gsl/gsl_roots.h>
#include <gsl/gsl_errno.h>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

#include <cmath>
#include <sstream>
#include <cassert>

namespace Mantid {
namespace API {
namespace {
/// Minima finding tolerance
const double MinimaFindingTolerance = 1e-10;

/// Parameters
const char *TILT_ANGLE_NAME = "TiltAngle";
const char *TAU_F_NAME = "TauF";
const char *TAU_S_NAME = "TauS";
const char *R_NAME = "R";
}

/// Default constructor required by the factory
IkedaCarpenterModerator::IkedaCarpenterModerator()
    : ModeratorModel(), m_tau_f(0.0), m_tau_s(0.0), m_r(0.0),
      m_lookupSize(1000), m_areaToTimeLookup(), m_offset(0.0) {}

/// Returns a clone of the current object
boost::shared_ptr<ModeratorModel> IkedaCarpenterModerator::clone() const {
  return boost::make_shared<IkedaCarpenterModerator>(*this);
}

/**
 * Sets the value of the \f$\alpha\f$ parameter
 * @param value :: A value for the fast decay coefficient in microseconds
 */
void IkedaCarpenterModerator::setFastDecayCoefficent(const double value) {
  m_tau_f = value;
}

/**
 * @returns Returns the value of the \f$\alpha\f$ parameter in microseconds
 */
double IkedaCarpenterModerator::getFastDecayCoefficent() const {
  return m_tau_f;
}

/**
 * Sets the value of the \f$\beta\f$ parameter
 * @param value :: A value for the slow decay coefficient in microseconds
 */
void IkedaCarpenterModerator::setSlowDecayCoefficent(const double value) {
  m_tau_s = value;
}

/**
 * @returns Returns the value of the \f$\beta\f$ parameter in microseconds
 */
double IkedaCarpenterModerator::getSlowDecayCoefficent() const {
  return m_tau_s;
}

/**
 * Sets the value of the \f$R\f$ parameter
 * @param value :: A value for the mixing coefficient
 */
void IkedaCarpenterModerator::setMixingCoefficient(const double value) {
  m_r = value;
}

/**
 * @returns Returns the value of the \f$\beta\f$ parameter in microseconds
 */
double IkedaCarpenterModerator::getMixingCoefficient() const { return m_r; }

/**
 * Mean emission time: \f$\tau_mean=3*\tau_f + R\tau_s\f$
 *  @returns The mean time for emission in microseconds
 */
double IkedaCarpenterModerator::emissionTimeMean() const {
  return 3.0 * m_tau_f + m_r * m_tau_s;
}

/**
 * Variance of emission time: \f$\tau_sig=\sqrt{3*\tau_f^2 + R(2-R)*\tau_s^2}\f$
 *  @returns The spread of emission times in microseconds^2
 */
double IkedaCarpenterModerator::emissionTimeVariance() const {
  return 3.0 * m_tau_f * m_tau_f + m_r * (2.0 - m_r) * m_tau_s * m_tau_s;
}

/**
 * Returns a time, in seconds, sampled from the distibution given a flat random
 * number
 * @param flatRandomNo :: A random number in the range [0,1]
 * @return A time, in seconds, sampled from the distribution
 */
double IkedaCarpenterModerator::sampleTimeDistribution(
    const double flatRandomNo) const {
  if (flatRandomNo >= 0.0 && flatRandomNo <= 1.0) {
    const double mean = emissionTimeMean();
    const double x = std::min(0.999, interpolateAreaTable(flatRandomNo));
    return mean * (2.0 * x - 1.0) / (1.0 - x);
  } else {
    std::ostringstream os;
    os << "IkedaCarpenterModerator::sampleTimeDistribution - Random number "
          "must be flat between [0,1]. Current value=" << flatRandomNo;
    throw std::invalid_argument(os.str());
  }
}

//----------------------------------------------------------------------------------------
// Private members
//----------------------------------------------------------------------------------------

/**
 * Custom initialize function, called after parameters have been set.
 * Initializes the lookup table
 */
void IkedaCarpenterModerator::init() { initLookupTable(); }

/**
 * Sets a parameter from a name & string value
 * @param name :: The name of the parameter
 * @param value :: The value as a string
 */
void IkedaCarpenterModerator::setParameterValue(const std::string &name,
                                                const std::string &value) {
  const double valueAsDbl = boost::lexical_cast<double>(value);
  if (name == TILT_ANGLE_NAME) {
    setTiltAngleInDegrees(valueAsDbl);
  } else if (name == TAU_F_NAME) {
    setFastDecayCoefficent(valueAsDbl);
  } else if (name == TAU_S_NAME) {
    setSlowDecayCoefficent(valueAsDbl);
  } else if (name == R_NAME) {
    setMixingCoefficient(valueAsDbl);
  } else {
    throw std::invalid_argument(
        "IkedaCarpenterModerator::setParameterValue - Unknown parameter: " +
        name);
  }
}

/**
 * Initialize the area-to-time lookup table
 */
void IkedaCarpenterModerator::initLookupTable() {
  m_areaToTimeLookup.resize(m_lookupSize);
  for (unsigned int i = 0; i < m_lookupSize; ++i) {
    const double area = static_cast<double>(i) / (m_lookupSize - 1);
    const double fraction = areaToTime(area);
    m_areaToTimeLookup[i] = fraction;
  }
}

/**
 * Return the value of time interpolated from the given area using the cached
 * table
 * @param area The value of the area to interpolate from
 * @return The time corresponding to this area linearly interpolated from the
 * cached table
 */
double IkedaCarpenterModerator::interpolateAreaTable(const double area) const {
  if (m_areaToTimeLookup.empty()) {
    PARALLEL_CRITICAL(IkedaCarpenterModerator_interpolateAreaTable) {
      if (m_areaToTimeLookup.empty()) {
        const_cast<IkedaCarpenterModerator *>(this)->initLookupTable();
      }
    }
  }
  const unsigned int nsteps = (m_lookupSize - 1);
  const unsigned int indexBelow =
      static_cast<unsigned int>(std::floor(area * nsteps));
  const double step = area * nsteps - indexBelow;
  if (indexBelow < nsteps)
    return m_areaToTimeLookup[indexBelow] * (1.0 - step) +
           m_areaToTimeLookup[indexBelow + 1] * step;
  else
    return m_areaToTimeLookup[indexBelow]; // last edge value
}

/**
 * Returns the value of T such that the integral of a normalised Ikeda-Carpenter
 * function, M(T), from 0 to T = area
 * @param area :: The required area value
 * @return The value of T such that the integral of M(T) equals the given area
 */
double IkedaCarpenterModerator::areaToTime(const double area) const {
  m_offset = area;
  const double rangeMin(0.0), rangeMax(1.0);
  if (area <= rangeMin) {
    return rangeMin;
  } else if (area >= rangeMax) {
    return rangeMax;
  } else {
    return findMinumum(rangeMin, rangeMax, MinimaFindingTolerance);
  }
}

/**
 * Find the minimum of the areaToTimeFunction between the given interval with
 * the given tolerance
 * @param rangeMin :: The start of the range where the function changes sign
 * @param rangeMax :: The end of the range where the function changes sign
 * @param tolerance :: The required tolerance
 * @return The location of the minimum
 */
double IkedaCarpenterModerator::findMinumum(const double rangeMin,
                                            const double rangeMax,
                                            const double tolerance) const {
  return zeroBrent(rangeMin, rangeMax, tolerance);
}

/**
 * Find the minimum of the areaToTimeFunction between the given interval with
 * the given tolerance
 * @param a :: The start of the range where the function changes sign
 * @param b :: The end of the range where the function changes sign
 * @param t :: The required tolerance
 * @return The location of the minimum
 */
double IkedaCarpenterModerator::zeroBrent(const double a, const double b,
                                          const double t) const {
  //****************************************************************************
  //
  //  Purpose:
  //
  //    zeroBrent seeks the root of a function F(X) in an interval [A,B].
  //
  //  Discussion:
  //
  //    The interval [A,B] must be a change of sign interval for F.
  //    That is, F(A) and F(B) must be of opposite signs.  Then
  //    assuming that F is continuous implies the existence of at least
  //    one value C between A and B for which F(C) = 0.
  //
  //    The location of the zero is determined to within an accuracy
  //    of 6 * MACHEPS * fabs ( C ) + 2 * T.
  //
  //  Licensing:
  //
  //    This code is distributed under the GNU LGPL license.
  //
  //  Modified:
  //
  //    13 April 2008
  //
  //  Author:
  //
  //    Original FORTRAN77 version by Richard Brent.
  //    C++ version by John Burkardt.
  //
  //  Reference:
  //
  //    Richard Brent,
  //    Algorithms for Minimization Without Derivatives,
  //    Dover, 2002,
  //    ISBN: 0-486-41998-3,
  //    LC: QA402.5.B74.
  //
  //  Parameters:
  //
  //    Input, double A, B, the endpoints of the change of sign interval.
  //
  //    Input, double T, a positive error tolerance.
  //
  //    Input, double F ( double X ), the name of a user-supplied function
  //    which evaluates the function whose zero is being sought.
  //
  //    Output, double ZERO, the estimated value of a zero of
  //    the function F.
  //

  //
  //  Make local copies of A and B.
  //
  double sa = a;
  double sb = b;

  double fa = areaToTimeFunction(sa);
  double fb = areaToTimeFunction(sb);
  double fc = fa;

  double c = sa;

  double e = sb - sa;
  double d = e;

  double macheps = 1e-14; // more than sufficient for Tobyfit

  for (int i = 0; true; i++) {
    if (fabs(fc) < fabs(fb)) {
      sa = sb;
      sb = c;
      c = sa;
      fa = fb;
      fb = fc;
      fc = fa;
    }

    double tol = 2.0 * macheps * fabs(sb) + t;
    double m = 0.5 * (c - sb);

    if (fabs(m) <= tol || fb == 0.0) {
      break;
    }

    if (fabs(e) < tol || fabs(fa) <= fabs(fb)) {
      e = m;
      d = e;
    } else {
      double p;
      double q;
      double s = fb / fa;

      if (sa == c) {
        p = 2.0 * m * s;
        q = 1.0 - s;
      } else {
        double r;
        q = fa / fc;
        r = fb / fc;
        p = s * (2.0 * m * a * (q - r) - (sb - sa) * (r - 1.0));
        q = (q - 1.0) * (r - 1.0) * (s - 1.0);
      }

      if (0.0 < p) {
        q = -q;
      } else {
        p = -p;
      }

      s = e;
      e = d;

      if (2.0 * p < 3.0 * m * q - fabs(tol * q) && p < fabs(0.5 * s * q)) {
        d = p / q;
      } else {
        e = m;
        d = e;
      }
    }
    sa = sb;
    fa = fb;

    if (tol < fabs(d)) {
      sb = sb + d;
    } else if (0.0 < m) {
      sb = sb + tol;
    } else {
      sb = sb - tol;
    }

    fb = areaToTimeFunction(sb);

    if ((0.0 < fb && 0.0 < fc) || (fb <= 0.0 && fc <= 0.0)) {
      c = sa;
      fc = fa;
      e = sb - sa;
      d = e;
    }
  }
  return sb;
}

/**
 * Function to pass to root-finder to find the value of the area for the given
 * fraction of the range
 * @param fraction ::
 * @return The value of the area for the given fraction with in the range
 */
double
IkedaCarpenterModerator::areaToTimeFunction(const double fraction) const {
  if (fraction <= 0.0) {
    return -m_offset;
  } else if (fraction >= 1.0) {
    return 1.0 - m_offset;
  } else {
    const double time = this->emissionTimeMean() * fraction / (1.0 - fraction);
    return area(time) - m_offset;
  }
}

/**
 * Calculates the area of the Ikeda-Carpenter moderator lineshape integrated
 * from 0 to x.
 * @param x :: The end-point of the integration
 * @return The value of the area
 */
double IkedaCarpenterModerator::area(const double x) const {
  if (x >= 0.0) {
    if (m_tau_f != 0.0) {
      const double c3 = 1.6666666666666666667e-01,
                   c4 = -1.2500000000000000000e-01,
                   c5 = 5.0000000000000000000e-02,
                   c6 = -1.3888888888888888889e-02,
                   c7 = 2.9761904761904761905e-03,
                   c8 = -5.2083333333333333333e-04,
                   c9 = 7.7160493827160493827e-05,
                   c10 = -9.9206349206349206349e-06,
                   c11 = 1.1273448773448773449e-06,
                   c12 = -1.1482216343327454439e-07,
                   c13 = 1.0598968932302265636e-08;

      const double ax = x / m_tau_f;
      double funAx;
      if (std::fabs(ax) <= 0.1) {
        funAx =
            c3 +
            ax *
                (c4 +
                 ax *
                     (c5 +
                      ax *
                          (c6 +
                           ax *
                               (c7 +
                                ax * (c8 +
                                      ax * (c9 +
                                            ax * (c10 +
                                                  ax * (c11 +
                                                        ax * (c12 +
                                                              ax * c13)))))))));
      } else
        funAx =
            (1.0 - exp(-(ax)) * (1.0 + (ax)+0.5 * (ax * ax))) / (ax * ax * ax);
      if (m_tau_s != 0. && m_r != 0.) {
        double funGx;
        const double gx = x * (1.0 / m_tau_f - 1.0 / m_tau_s);
        if (gx < 0.1)
          funGx =
              c3 +
              gx *
                  (c4 +
                   gx *
                       (c5 +
                        gx *
                            (c6 +
                             gx *
                                 (c7 +
                                  gx *
                                      (c8 +
                                       gx *
                                           (c9 +
                                            gx * (c10 +
                                                  gx * (c11 +
                                                        gx * (c12 +
                                                              gx * c13)))))))));
        else
          funGx = (1.0 - exp(-(gx)) * (1.0 + (gx)+0.5 * (gx * gx))) /
                  (gx * gx * gx);
        return ((ax * ax * ax) *
                (funAx - m_r * funGx * std::exp(-(x / m_tau_s))));
      } else
        return ((ax * ax * ax) * funAx);
    }
    if (m_tau_s != 0. && m_r != 0.)
      return ((1.0 - m_r) + m_r * (1.0 - std::exp(-(x / m_tau_s))));
    else
      return (1.);
  }
  return (0.);
}
}
}
