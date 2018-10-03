#ifndef MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_
#define MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_

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
   Setter for the incident theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaIncident: incident theta value in degrees
   */
  void setThetaIncident(double thetaIncident) override {
    m_sin_theta_i = sin(to_radians_factor * thetaIncident);
  }

  /**
   Setter for the final theta value require for the calculation. Internally
   pre-calculates and caches to cos theta for speed.
   @param thetaFinal: final theta value in degrees
   */
  void setThetaFinal(double thetaFinal) override {
    m_sin_theta_f = sin(to_radians_factor * thetaFinal);
  }

  /**
   Executes the calculation to determine PSum
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim0(double wavelength) const override {
    double wavenumber = 2.0 * M_PI / wavelength;
    double ki = wavenumber * m_sin_theta_i;
    double kf = wavenumber * m_sin_theta_f;
    return ki + kf;
  }

  /**
   Executes the calculation to determine PDiff
   @param wavelength : wavelength in Angstroms
   */
  double calculateDim1(double wavelength) const override {
    double wavenumber = 2.0 * M_PI / wavelength;
    double ki = wavenumber * m_sin_theta_i;
    double kf = wavenumber * m_sin_theta_f;
    return ki - kf;
  }
  Mantid::Geometry::Quadrilateral createQuad(double lamUpper, double lamLower,
                                             double thetaUpper,
                                             double thetaLower) override {
    setThetaFinal(thetaLower);
    auto dim1UpperRightVertex = calculateDim1(lamLower);
    auto dim0LowerLeftVertex = calculateDim0(lamUpper);
    // UPPER LEFT VERTEX
    const Mantid::Kernel::V2D secondVertex(
        calculateDim0(lamUpper), // highest qx
        calculateDim1(lamLower));

    setThetaFinal(thetaUpper);
    const Mantid::Kernel::V2D firstVertex(dim0LowerLeftVertex,
                                          calculateDim1(lamUpper)); // lowest qz

    const Mantid::Kernel::V2D thirdVertex(calculateDim0(lamLower),
                                          dim1UpperRightVertex); // highest qz
    // LOWER RIGHT VERTEX
    const Mantid::Kernel::V2D fourthVertex(calculateDim0(lamLower), // lowest qx
                                           calculateDim1(lamUpper));

    Mantid::Geometry::Quadrilateral quad(firstVertex, fourthVertex, thirdVertex,
                                         secondVertex);
    // Our lower-left vertex may not be in the right position
    // we keep shifting the vertices around in a clock-wise fashion
    // until the lower-left vertex is in the correct place.
    while ((quad.at(0).X() > quad.at(3).X()) ||
           (quad.at(0).Y() > quad.at(1).Y())) {
      quad.shiftVertexesClockwise();
    }
    return quad;
  }
};
} // namespace DataObjects
} // namespace Mantid
#endif // MANTID_DATAOBJECTS_CALCULATEREFLECTOMETRYP_H_