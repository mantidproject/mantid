#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYQXQZ_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYQXQZ_H_

#include "MantidDataObjects/CalculateReflectometry.h"
#include <cmath>

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
  CalculateReflectometryQxQz() : m_dirQx(0.0), m_dirQz(0.0) {}

  /**
   Setter for the incident theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaIncident: incident theta value in degrees
   */
  void setThetaIncident(double thetaIncident) {
    m_cos_theta_i = cos(thetaIncident * to_radians_factor);
    m_sin_theta_i = sin(thetaIncident * to_radians_factor);
  }

  /**
   Setter for the final theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaFinal: final theta value in degrees
   */
  void setThetaFinal(double thetaFinal) {
    const double c_cos_theta_f = cos(thetaFinal * to_radians_factor);
    m_dirQx = (c_cos_theta_f - m_cos_theta_i);
    const double c_sin_theta_f = sin(thetaFinal * to_radians_factor);
    m_dirQz = (c_sin_theta_f + m_sin_theta_i);
  }

  /**
   Executes the calculation to determine Qx
   @param wavelength : wavelenght in Anstroms
   */
  double calculateDim0(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQx;
  }

  /**
   Executes the calculation to determine Qz
   @param wavelength : wavelenght in Anstroms
   */
  double calculateDim1(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQz;
  }

  Mantid::Geometry::Quadrilateral createQuad(double lamUpper, double lamLower,
                                             double thetaUpper,
                                             double thetaLower) {
    setThetaFinal(thetaLower);
    const Mantid::Kernel::V2D ur(calculateDim0(lamLower), // highest qx
                                 calculateDim1(lamLower));
    const Mantid::Kernel::V2D lr(calculateDim0(lamUpper),
                                 calculateDim1(lamUpper)); // lowest qz
    setThetaFinal(thetaUpper);
    const Mantid::Kernel::V2D ul(calculateDim0(lamLower),
                                 calculateDim1(lamLower)); // highest qz
    const Mantid::Kernel::V2D ll(calculateDim0(lamUpper),  // lowest qx
                                 calculateDim1(lamUpper));

    Mantid::Geometry::Quadrilateral quad(ll, lr, ur, ul);
    return quad;
  }
};
}
}

#endif
