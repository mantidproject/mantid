// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_

#include "MantidDataObjects/CalculateReflectometryQxQz.h"
#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {

namespace MDAlgorithms {

/** ReflectometryTranformQxQz : Type of ReflectometyTransform. Used to convert
 from an input R vs Wavelength workspace to a 2D MDEvent workspace with
 dimensions of QxQy.
 Transformation is specific for reflectometry purposes.

 @date 2012-05-29
 */
class DLLExport ReflectometryTransformQxQz
    : public DataObjects::ReflectometryTransform {
public:
  /// Constructor
  ReflectometryTransformQxQz(double qxMin, double qxMax, double qzMin,
                             double qzMax, double incidentTheta,
                             int numberOfBinsQx = 100,
                             int numberOfBinsQz = 100);

  /// Disable default constructor
  ReflectometryTransformQxQz() = delete;

  /// Disable copy operator
  ReflectometryTransformQxQz(const ReflectometryTransformQxQz &) = delete;

  /// Disable assignment operator
  ReflectometryTransformQxQz &
  operator=(const ReflectometryTransformQxQz &) = delete;
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_ */
