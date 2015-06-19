#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYQXQZ_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYQXQZ_H_

#include <cmath>

namespace Mantid {
namespace DataObjects {

/**
 Converts from inputs of wavelength, incident theta and final theta to Qx and Qz
 for reflectometry experiments
 */
class CalculateReflectometryQxQz {
private:
  double m_cos_theta_i;
  double m_sin_theta_i;
  double m_dirQx;
  double m_dirQz;
  const double to_radians_factor;

public:
  /**
   Constructor
   @param thetaIncident: incident theta value in degrees
   */
  CalculateReflectometryQxQz(const double &thetaIncident)
    : m_cos_theta_i(cos(thetaIncident * to_radians_factor)),
      m_sin_theta_i(sin(thetaIncident * to_radians_factor)),
      m_dirQx(0.0), m_dirQz(0.0), to_radians_factor(M_PI / 180) {}

  /**
   Setter for the final theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaFinal: final theta value in degrees
   */
  void setThetaFinal(const double &thetaFinal) {
    const double c_cos_theta_f = cos(thetaFinal * to_radians_factor);
    m_dirQx = (c_cos_theta_f - m_cos_theta_i);
    const double c_sin_theta_f = sin(thetaFinal * to_radians_factor);
    m_dirQz = (c_sin_theta_f + m_sin_theta_i);
  }

  /**
   Executes the calculation to determine Qx
   @param wavelength : wavelenght in Anstroms
   */
  double calculateX(const double &wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQx;

  }

  /**
   Executes the calculation to determine Qz
   @param wavelength : wavelenght in Anstroms
   */
  double calculateZ(const double &wavelength) const {
    double wavenumber = 2 * M_PI / wavelength;
    return wavenumber * m_dirQz;
  }
};

}
}

#endif
