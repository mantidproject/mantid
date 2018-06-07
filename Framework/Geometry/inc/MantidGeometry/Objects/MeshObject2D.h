#ifndef MANTID_GEOMETRY_MESHOBJECT2D_H_
#define MANTID_GEOMETRY_MESHOBJECT2D_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Material.h"
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;
}

namespace Geometry {

/** MeshObject2D :

  Defines an IObject implemented as a 2D mesh composed of triangles. Avoids
  assumptions made in MeshObject to do with closed surfaces, non-zero volumes
  and assoicated additional runtime costs. The number of vertices is limited to
  2^16 based on index type.

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
class MANTID_GEOMETRY_DLL MeshObject2D {
public:
  /// Constructor
  MeshObject2D(const std::vector<uint16_t> &faces,
               const std::vector<Mantid::Kernel::V3D> &vertices,
               const Kernel::Material &material);
  /// Constructor
  MeshObject2D(std::vector<uint16_t> &&faces,
               std::vector<Mantid::Kernel::V3D> &&vertices,
               const Kernel::Material &&material);

  double volume() const;

  static bool pointsCoplanar(const std::vector<Mantid::Kernel::V3D> &vertices);

  bool hasValidShape() const;

private:
  void initialize();
  /// Triangles are specified by indices into a list of vertices. Offset is
  /// always 3.
  std::vector<uint16_t> m_triangles;
  /// Vertices
  std::vector<Kernel::V3D> m_vertices;
  /// Material composition
  Kernel::Material m_material;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_MESHOBJECT2D_H_ */
