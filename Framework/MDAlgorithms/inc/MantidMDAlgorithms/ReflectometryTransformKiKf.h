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
