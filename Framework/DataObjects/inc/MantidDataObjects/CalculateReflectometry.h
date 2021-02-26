// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/Math/Quadrilateral.h"
#include <cmath>

namespace Mantid {
namespace DataObjects {

/**
 * Provides a common interface to Reflectometry Transform calculators.
 */
class CalculateReflectometry {

protected:
  double m_theta_i{0.0};
  const double to_radians_factor = M_PI / 180.0;

public:
  virtual ~CalculateReflectometry() = default;

  /**
   Setter for the incident theta value require for the calculation.
   @param thetaIncident: incident theta value in degrees
   */
  void setThetaIncident(double thetaIncident) {
    m_theta_i = thetaIncident;
    updateThetaIncident(thetaIncident);
  };

  /**
   Derived class setter for the final theta value require for the calculation.
   @param thetaIncident: incident theta value in degrees
   */
  virtual void updateThetaIncident(double thetaIncident) = 0;

  /**
   Set the final theta value from the detector twoTheta angle.
   @param twoTheta: detector twoTheta value in degrees
   */
  virtual void setTwoTheta(double twoTheta) = 0;

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
