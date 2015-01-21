#include "MantidSINQ/PoldiUtilities/PoldiConversions.h"
#include "MantidKernel/PhysicalConstants.h"

#include <stdexcept>
#include <cmath>

namespace Mantid {

namespace Poldi {

namespace Conversions {

/* Unit conversion functions dtoTOF and TOFtod
 *
 * This way of converting units leads to values that differ slightly from the
 *ones produced
 * by the original fortran code. In that code there is a lot of "multiplying by
 *2PI and dividing by 2PI"
 * going on, which is not present here. These small deviations accumulate when a
 *lot of conversions
 * are performed, so the end results may differ numerically in those cases.
 *
 * The following two functions are exactly inverse, as demonstrated by the unit
 *tests.
 */
/** Converts TOF to d, given a distance and sin(theta). Throws std::domain_error
  * when distance or sin(theta) <= 0.
  *
  * @param tof :: Time of flight in microseconds.
  * @param distance :: Neutron flight path in mm.
  * @param sinTheta :: sin(theta).
  * @return d in Angstrom.
  */
double TOFtoD(double tof, double distance, double sinTheta) {
  if (distance <= 0 || sinTheta <= 0.0) {
    throw std::domain_error(
        "Distances and sin(theta) less or equal to 0 cannot be processed.");
  }

  return PhysicalConstants::h * 1e7 * tof /
         (2.0 * distance * sinTheta * PhysicalConstants::NeutronMass);
}

/** Converts d to TOF, given a distance and sin(theta)
  *
  * @param d :: d in Angstrom.
  * @param distance :: Neutron flight path in mm.
  * @param sinTheta :: sin(theta).
  * @return TOF in microseconds.
  */
double dtoTOF(double d, double distance, double sinTheta) {
  return 2.0 * distance * sinTheta * d * PhysicalConstants::NeutronMass /
         (PhysicalConstants::h * 1e7);
}

/** Converts d-spacing in Angstrom to momentum transfer Q (in reciprocal
  *Angstrom). Throws std::domain_error
  * when d <= 0 is supplied.
  *
  * @param d :: d-spacing in Angstrom
  * @return Q in reciprocal Angstrom
  */
double dToQ(double d) {
  if (d <= 0.0) {
    throw std::domain_error(
        "Can not convert d-spacings less or equal than zero.");
  }

  return (2.0 * M_PI) / d;
}

/** Converts momentum transfer Q (in reciprocal Angstrom) to d-spacing in
  *Angstrom. Throws std::domain_error
  * when Q <= 0 is supplied.
  *
  * @param q :: Q-value in reciprocal Angstrom
  * @return d in Angstrom
  */
double qToD(double q) {
  if (q <= 0.0) {
    throw std::domain_error(
        "Can not convert Q-values less or equal than zero.");
  }

  return (2.0 * M_PI) / q;
}

/** Converts degrees to radians
  * @param degree :: angle in degree
  * @return angle in radians
  */
double degToRad(double degree) { return degree / 180.0 * M_PI; }

/** Converts radians to degrees
  * @param radians :: angle in radians
  * @return angle in degrees
  */
double radToDeg(double radians) { return radians / M_PI * 180.0; }
}
}
}
