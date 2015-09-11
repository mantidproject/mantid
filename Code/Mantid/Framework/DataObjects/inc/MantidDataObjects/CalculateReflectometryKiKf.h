#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYKIKF_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYKIKF_H_
#include "MantidDataObjects/CalculateReflectometry.h"
#include <cmath>

namespace Mantid {
namespace DataObjects{
/**
class CalculateReflectometryKiKf: Calculation type for converting to ki or kf given
a theta value (in degrees) and a wavelength
*/
class CalculateReflectometryKiKf : public CalculateReflectometry {
private:
  double m_sin_theta_i;
  double m_sin_theta_f;

public:
  /**
   Constructor
   */
  CalculateReflectometryKiKf() : m_sin_theta_i(0.0), m_sin_theta_f(0.0) {}

  /**
   Destructor
   */
  ~CalculateReflectometryKiKf(){};

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
   Executes the calculation to determine Ki
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim0(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_sin_theta_i;
  }

  /**
   Executes the calculation to determine Kf
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim1(double wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_sin_theta_f;
  }
  Mantid::Geometry::Quadrilateral createQuad(double lamUpper, double lamLower, double thetaUpper, double thetaLower){
    /**THIS IS NOT CORRECT FOR KiKf TRANSFORMATION**/
      setThetaFinal(thetaLower);
      const Mantid::Kernel::V2D ur(calculateDim0(lamLower), // highest qx
                   calculateDim1(lamLower));
      const Mantid::Kernel::V2D lr(calculateDim0(lamUpper),
                   calculateDim1(lamUpper)); // lowest qz

      setThetaFinal(thetaUpper);
      const Mantid::Kernel::V2D ul(calculateDim0(lamLower),
                   calculateDim1(lamLower)); // highest qz
      const Mantid::Kernel::V2D ll(calculateDim0(lamUpper), // lowest qx
                   calculateDim1(lamUpper));

      Mantid::Geometry::Quadrilateral quad(ll, lr, ur, ul);
      return quad;
  }
};
}
}
#endif // MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYKIKF_H_
