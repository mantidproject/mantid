#ifndef MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_
#define MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_

#include "MantidDataObjects/CalculateReflectometryP.h"
#include "MantidDataObjects/ReflectometryTransform.h"

namespace Mantid {
namespace MDAlgorithms {

/** ReflectometryTransformP : Calculates workspace(s) of Pi and Pf based on the
  input workspace and incident theta angle.

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
class DLLExport ReflectometryTransformP
    : public DataObjects::ReflectometryTransform {
public:
  ReflectometryTransformP(double pSumMin, double pSumMax, double pDiffMin,
                          double pDiffMax, double incidentTheta,
                          int numberOfBinsQx = 100, int numberOfBinsQz = 100);
};

} // namespace MDAlgorithms
} // namespace Mantid

#endif /* MANTID_MDALGORITHMS_REFLECTOMETRYTRANSFORMP_H_ */
