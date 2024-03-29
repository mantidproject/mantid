// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/CalculateReflectometry.h"

namespace Mantid {
namespace DataObjects {

/**
 Converts from inputs of wavelength, incident theta and final theta to Qx and Qz
 for reflectometry experiments
 */
class CalculateReflectometryQxQz : public CalculateReflectometry {
private:
  double m_cos_theta_i;
  double m_sin_theta_i;
  double m_dirQx;
  double m_dirQz;

public:
  /**
   Constructor
   */
  CalculateReflectometryQxQz(int version)
      : CalculateReflectometry(version), m_cos_theta_i(0.0), m_sin_theta_i(0.0), m_dirQx(0.0), m_dirQz(0.0) {}

  /**
   Setter for the incident theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaIncident: incident theta value in degrees
   */
  void updateThetaIncident(double thetaIncident) override {
    m_cos_theta_i = cos(thetaIncident * to_radians_factor);
    m_sin_theta_i = sin(thetaIncident * to_radians_factor);
  }

  /**
   Setter for the final theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaFinal: final theta value in degrees
   */
  void setThetaFinal(double thetaFinal) override {
    const double c_cos_theta_f = cos(thetaFinal * to_radians_factor);
    m_dirQx = (c_cos_theta_f - m_cos_theta_i);
    const double c_sin_theta_f = sin(thetaFinal * to_radians_factor);
    m_dirQz = (c_sin_theta_f + m_sin_theta_i);
  }

  /**
   Set the final theta value from the detector twoTheta value.
   @param twoTheta: detector twoTheta value in degrees
   */
  void setTwoTheta(double twoTheta) override {
    if (m_version == 1)
      setThetaFinal(twoTheta);
    else
      setThetaFinal(twoTheta - m_theta_i);
  }

  /**
   Executes the calculation to determine Qx
   @param wavelength : wavelenght in Anstroms
   */
  double calculateDim0(double wavelength) const override {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQx;
  }

  /**
   Executes the calculation to determine Qz
   @param wavelength : wavelenght in Anstroms
   */
  double calculateDim1(double wavelength) const override {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQz;
  }

  Mantid::Geometry::Quadrilateral createQuad(double lamUpper, double lamLower, double thetaUpper,
                                             double thetaLower) override {
    setTwoTheta(thetaLower);
    const Mantid::Kernel::V2D firstVertex(calculateDim0(lamLower), // highest qx
                                          calculateDim1(lamLower));
    const Mantid::Kernel::V2D secondVertex(calculateDim0(lamUpper),
                                           calculateDim1(lamUpper)); // lowest qz
    setTwoTheta(thetaUpper);
    const Mantid::Kernel::V2D thirdVertex(calculateDim0(lamLower),
                                          calculateDim1(lamLower)); // highest qz
    const Mantid::Kernel::V2D fourthVertex(calculateDim0(lamUpper), // lowest qx
                                           calculateDim1(lamUpper));
    Mantid::Geometry::Quadrilateral quad(fourthVertex, secondVertex, firstVertex, thirdVertex);
    // Our lower-left vertex may not be in the right position
    // we keep shifting the vertices around in a clock-wise fashion
    // until the lower-left vertex is in the correct place.
    while ((quad.at(0).X() > quad.at(3).X()) || (quad.at(0).Y() > quad.at(1).Y())) {
      quad.shiftVertexesClockwise();
    }

    return quad;
  }
};
} // namespace DataObjects
} // namespace Mantid
