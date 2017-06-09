#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANFORMQXQZ_H_

#include "MantidDataObjects/RebinnedOutput.h"
#include "MantidDataObjects/ReflectometryTransform.h"
#include "MantidDataObjects/CalculateReflectometryQxQz.h"

namespace Mantid {

namespace MDAlgorithms {

/** ReflectometryTranformQxQz : Type of ReflectometyTransform. Used to convert
 from an input R vs Wavelength workspace to a 2D MDEvent workspace with
 dimensions of QxQy.
 Transformation is specific for reflectometry purposes.

 @date 2012-05-29

 Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
 National Laboratory & European Spallation Source

 This file is part of Mantid.

 Mantid is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 Mantid is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 File change history is stored at: <https://github.com/mantidproject/mantid>
 Code Documentation is available at: <http://doxygen.mantidproject.org>
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
