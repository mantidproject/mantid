#ifndef MANTID_GEOMETRY_GROUPTRANSFORMATION_H_
#define MANTID_GEOMETRY_GROUPTRANSFORMATION_H_

#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SymmetryOperation.h"
#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/** GroupTransformation

  This class transforms Group-objects, using a SymmetryOperation. It can
  be used to transform a point- or space group into a different setting.

  The following example shows how to transform the Group P 1 2/m 1 to
  P 1 1 2/m, with unique c-axis:

    SpaceGroup_const_sptr sg =
        SpaceGroupFactory::Instance().createSpaceGroup("P 1 2/m 1");

    Group transformed = GroupTransformation("y,z,x")(*sg);

  Using the getInverse-method, the transformation can be reversed.

      @author Michael Wedel, ESS
      @date 01/11/2015

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_GEOMETRY_DLL GroupTransformation {
public:
  GroupTransformation(const MatrixVectorPair<double, V3R> &operation);
  GroupTransformation(const std::string &operationString);

  virtual ~GroupTransformation() = default;

  Group operator()(const Group &other) const;

  GroupTransformation getInverse() const;

private:
  SymmetryOperation
  transformOperation(const SymmetryOperation &operation) const;

  void setInverseFromPair();

  MatrixVectorPair<double, V3R> m_matrixVectorPair;
  MatrixVectorPair<double, V3R> m_inversePair;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_GROUPTRANSFORMATION_H_ */
