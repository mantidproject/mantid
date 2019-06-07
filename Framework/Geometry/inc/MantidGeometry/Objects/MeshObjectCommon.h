#ifndef MANTID_GEOMETRY_MESHOBJECTCOMMON_H_
#define MANTID_GEOMETRY_MESHOBJECTCOMMON_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/V3D.h"
#include <vector>

namespace Mantid {
namespace Geometry {
class BoundingBox;
/** MeshObjectCommon : Performs functions common to 3D and 2D closed meshes

  Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace MeshObjectCommon {

MANTID_GEOMETRY_DLL std::vector<double>
getVertices(const std::vector<Kernel::V3D> &vertices);

MANTID_GEOMETRY_DLL bool isOnTriangle(const Kernel::V3D &point,
                                      const Kernel::V3D &v1,
                                      const Kernel::V3D &v2,
                                      const Kernel::V3D &v3);
MANTID_GEOMETRY_DLL bool
rayIntersectsTriangle(const Kernel::V3D &start, const Kernel::V3D &direction,
                      const Kernel::V3D &v1, const Kernel::V3D &v2,
                      const Kernel::V3D &v3, Kernel::V3D &intersection,
                      TrackDirection &entryExit);

MANTID_GEOMETRY_DLL void checkVertexLimit(size_t nVertices);
MANTID_GEOMETRY_DLL const BoundingBox &
getBoundingBox(const std::vector<Kernel::V3D> &vertices, BoundingBox &cacheBB);
MANTID_GEOMETRY_DLL void
getBoundingBox(const std::vector<Kernel::V3D> &vertices, BoundingBox &cacheBB,
               double &xmax, double &ymax, double &zmax, double &xmin,
               double &ymin, double &zmin);
MANTID_GEOMETRY_DLL double getTriangleSolidAngle(const Kernel::V3D &a,
                                                 const Kernel::V3D &b,
                                                 const Kernel::V3D &c,
                                                 const Kernel::V3D &observer);
} // namespace MeshObjectCommon

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MESHOBJECTCOMMON_H_ */
