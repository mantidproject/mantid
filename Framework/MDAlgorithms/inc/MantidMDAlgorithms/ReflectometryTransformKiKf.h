// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_

#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidDataObjects/CalculateReflectometryKiKf.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {
namespace MDAlgorithms {
/** ReflectometryTransformKiKf : Type to transform from R vs Wavelength
  workspace to a 2D MDEW with dimensions of Ki and Kf.

  @date 2012-06-06
*/
class DLLExport ReflectometryTransformKiKf
    : public DataObjects::ReflectometryTransform {
public:
  ReflectometryTransformKiKf(double kiMin, double kiMax, double kfMin,
                             double kfMax, double incidentTheta,
                             int numberOfBinsQx = 100,
                             int numberOfBinsQz = 100);

  /// Disable default constructor
  ReflectometryTransformKiKf() = delete;

  /// Disable copy operator
  ReflectometryTransformKiKf(const ReflectometryTransformKiKf &) = delete;

  /// Disable assignment operator
  ReflectometryTransformKiKf &
  operator=(const ReflectometryTransformKiKf &) = delete;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMKIKF_H_ */
