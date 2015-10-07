#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_

#include <cmath>
#include "MantidDataObjects/CalculateReflectometry.h"

namespace Mantid {
namespace DataObjects {
/**
class CalculateReflectometryP: p-type transformation calculator
*/
class CalculateReflectometryP : public DataObjects::CalculateReflectometry {
private:
  double m_sin_theta_i;
  double m_sin_theta_f;

public:
  /**
   * Constructor
   */
  CalculateReflectometryP() : m_sin_theta_i(0.0), m_sin_theta_f(0.0) {}

  /**
   * Destructor
   */
  ~CalculateReflectometryP(){};

  /**
   Setter for the incident theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaIncident: incident theta value in degrees
   */
  void setThetaIncident(double thetaIncident) {
    m_sin_theta_i = sin(to_radians_factor * thetaIncident);
  }

  /**
   Setter for the final theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaFinal: final theta value in degrees
   */
  void setThetaFinal(double thetaFinal) {
    m_sin_theta_f = sin(to_radians_factor * thetaFinal);
  }

  /**
   Executes the calculation to determine PSum
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim0(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    double ki = wavenumber * m_sin_theta_i;
    double kf = wavenumber * m_sin_theta_f;
    return ki + kf;
  }

  /**
   Executes the calculation to determine PDiff
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim1(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    double ki = wavenumber * m_sin_theta_i;
    double kf = wavenumber * m_sin_theta_f;
    return ki - kf;
  }
  Mantid::Geometry::Quadrilateral createQuad(double lamUpper, double lamLower,
                                             double thetaUpper,
                                             double thetaLower) {
    setThetaFinal(thetaLower);
    auto dim1UpperRightVertex = calculateDim1(lamLower);
    auto dim0LowerLeftVertex = calculateDim0(lamUpper);
    // UPPER LEFT VERTEX
    const Mantid::Kernel::V2D ul(calculateDim0(lamUpper), // highest qx
                                 calculateDim1(lamLower));

    setThetaFinal(thetaUpper);
    const Mantid::Kernel::V2D ll(dim0LowerLeftVertex,
                                 calculateDim1(lamUpper)); // lowest qz

    const Mantid::Kernel::V2D ur(calculateDim0(lamLower),
                                 dim1UpperRightVertex); // highest qz
    // LOWER RIGHT VERTEX
    const Mantid::Kernel::V2D lr(calculateDim0(lamLower), // lowest qx
                                 calculateDim1(lamUpper));

    Mantid::Geometry::Quadrilateral quad(ll, lr, ur, ul);
    return quad;
  }
};
}
}
#endif // MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_