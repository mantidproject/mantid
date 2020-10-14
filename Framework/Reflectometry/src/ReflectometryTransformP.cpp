// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidReflectometry/ReflectometryTransformP.h"
using namespace Mantid::DataObjects;

namespace Mantid {
namespace Reflectometry {
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

} // namespace Reflectometry
} // namespace Mantid
