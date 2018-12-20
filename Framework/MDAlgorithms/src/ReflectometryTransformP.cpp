#include "MantidMDAlgorithms/ReflectometryTransformP.h"
using namespace Mantid::DataObjects;

namespace Mantid {
namespace MDAlgorithms {
/*
Constructor
@param pSumMin: p sum min value (extent)
@param pSumMax: p sum max value (extent)
@param pDiffMin: p diff min value (extent)
@param pDiffMax: p diff max value (extent)
@param incidentTheta: Predetermined incident theta value
@param numberOfBinsQx : Number of bins along the qx axis
@param numberOfBinsQz : Number of bins along the qz axis
*/
ReflectometryTransformP::ReflectometryTransformP(
    double pSumMin, double pSumMax, double pDiffMin, double pDiffMax,
    double incidentTheta, int numberOfBinsQx, int numberOfBinsQz)
    : ReflectometryTransform("Pz_i + Pz_f", "sum_pz", pSumMin, pSumMax,
                             "Pz_i - Pz_f", "diff_pz", pDiffMin, pDiffMax,
                             numberOfBinsQx, numberOfBinsQz,
                             new CalculateReflectometryP()) {
  if (incidentTheta < 0 || incidentTheta > 90) {
    throw std::out_of_range("incident theta angle must be > 0 and < 90");
  }
  m_calculator->setThetaIncident(incidentTheta);
}

} // namespace MDAlgorithms
} // namespace Mantid
