#include "MantidMDAlgorithms/ReflectometryTransformQxQz.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

/*
 Constructor
 @param qxMin: min qx value (extent)
 @param qxMax: max qx value (extent)
 @param qzMin: min qz value (extent)
 @param qzMax; max qz value (extent)
 @param incidentTheta: Predetermined incident theta value
 @param numberOfBinsQx : Number of bins along the qx axis
 @param numberOfBinsQz : Number of bins along the qz axis
 */
ReflectometryTransformQxQz::ReflectometryTransformQxQz(
    double qxMin, double qxMax, double qzMin, double qzMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform("Qx", "qx", qxMin, qxMax, "Qz", "qz", qzMin, qzMax,
                             numberOfBinsQx, numberOfBinsQz,
                             new CalculateReflectometryQxQz()) {
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
  m_calculator->setThetaIncident(incidentTheta);
}
} // namespace MDAlgorithms
} // namespace Mantid
