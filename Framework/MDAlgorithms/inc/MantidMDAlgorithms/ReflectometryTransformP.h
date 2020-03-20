// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataObjects/CalculateReflectometryP.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {
namespace MDAlgorithms {

/** ReflectometryTransformP : Calculates workspace(s) of Pi and Pf based on the
  input workspace and incident theta angle.

  @date 2012-06-06
*/
class DLLExport ReflectometryTransformP
    : public DataObjects::ReflectometryTransform {
public:
  ReflectometryTransformP(double pSumMin, double pSumMax, double pDiffMin,
                          double pDiffMax, double incidentTheta,
                          int numberOfBinsQx = 100, int numberOfBinsQz = 100);
};

} // namespace MDAlgorithms
} // namespace Mantid
