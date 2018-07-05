#ifndef MANTID_GEOMETRY_RENDERINGMESH_H_
#define MANTID_GEOMETRY_RENDERINGMESH_H_

#include "MantidGeometry/DllConfig.h"

namespace Mantid {
namespace Geometry {

/** RenderingMesh : Mesh abstraction required for rendering

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
class MANTID_GEOMETRY_DLL RenderingMesh {

public:
  virtual size_t numberOfVertices() const = 0;
  virtual size_t numberOfTriangles() const = 0;
  virtual std::vector<double> getVertices() const = 0;
  virtual std::vector<uint32_t> getTriangles() const = 0;
  virtual ~RenderingMesh() {}
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_RENDERINGMESH_H_ */
