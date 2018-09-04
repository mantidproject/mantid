#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRY_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRY_H_

#include "MantidGeometry/Math/Quadrilateral.h"
#include <cmath>

namespace Mantid {
namespace DataObjects {

/**
 * Provides a common interface to Reflectometry Transform calculators.
 */
class CalculateReflectometry {

protected:
  const double to_radians_factor = M_PI / 180.0;

public:
  virtual ~CalculateReflectometry() = default;

  /**
   Setter for the final theta value require for the calculation.
   @param thetaIncident: final theta value in degrees
   */
  virtual void setThetaIncident(double thetaIncident) = 0;

  /**
   Setter for the final theta value require for the calculation.
   @param thetaFinal: final theta value in degrees
   */
  virtual void setThetaFinal(double thetaFinal) = 0;

  /**
   Executes the calculation on dimension 0
   @param wavelength : wavelength in Angstroms
   */
  virtual double calculateDim0(double wavelength) const = 0;

  /**
   Executes the calculation on dimension 1
   @param wavelength : wavelength in Angstroms
   */
  virtual double calculateDim1(double wavelength) const = 0;

  virtual Mantid::Geometry::Quadrilateral createQuad(double lamLower,
                                                     double lamUpper,
                                                     double thetaLower,
                                                     double thetaUpper) = 0;
};
} // namespace DataObjects
} // namespace Mantid

#endif
