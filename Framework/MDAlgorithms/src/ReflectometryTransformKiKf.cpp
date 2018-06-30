#include "MantidMDAlgorithms/ReflectometryTransformKiKf.h"

using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {

/*
  Constructor
  @param kiMin: min ki value (extent)
  @param kiMax: max ki value (extent)
  @param kfMin: min kf value (extent)
  @param kfMax; max kf value (extent)
  @param incidentTheta: Predetermined incident theta value
  @param numberOfBinsQx: Number of bins in the qx axis
  @param numberOfBinsQz: Number of bins in the qz axis
*/
ReflectometryTransformKiKf::ReflectometryTransformKiKf(
    double kiMin, double kiMax, double kfMin, double kfMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform("Ki", "ki", kiMin, kiMax, "Kf", "kf", kfMin, kfMax,
                             numberOfBinsQx, numberOfBinsQz,
                             new CalculateReflectometryKiKf()) {
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
  m_calculator->setThetaIncident(incidentTheta);
}

} // namespace MDAlgorithms
} // namespace Mantid
