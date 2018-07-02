#ifndef MANTID_GEOMETRY_SHAPEINFO_H_
#define MANTID_GEOMETRY_SHAPEINFO_H_

#include "MantidGeometry/DllConfig.h"
#include <vector>

namespace Mantid {
namespace Kernel {
class V3D;
}
namespace Geometry {
class RectangularDetector;
class StructuredDetector;
class IObjComponent;

/** ShapeInfo : Stores shape types and information relevant to drawing the
shape. For cylinders, spheres and cones, height and radius are stored. Points
are stored in the winding order shown in the IDF here
http://docs.mantidproject.org/nightly/concepts/HowToDefineGeometricShape.html.

Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
namespace detail {
class MANTID_GEOMETRY_DLL ShapeInfo {
public:
  enum class GeometryShape {
    NOSHAPE = 0,
    CUBOID,     ///< CUBOID
    HEXAHEDRON, ///< HEXAHEDRON
    SPHERE,     ///< SPHERE
    CYLINDER,   ///< CYLINDER
    CONE,       ///< CONE
  };

private:
  std::vector<Kernel::V3D> m_points;
  double m_radius; ///< Radius for the sphere, cone and cylinder
  double m_height; ///< height for cone and cylinder;
  GeometryShape m_shape;

public:
  ShapeInfo();
  ShapeInfo(const ShapeInfo &) = default;
  const std::vector<Kernel::V3D> &points() const;
  double radius() const;
  double height() const;
  GeometryShape shape() const;

  void getObjectGeometry(GeometryShape &myshape,
                         std::vector<Kernel::V3D> &points, double &myradius,
                         double &myheight) const;
  /// sets the geometry handler for a cuboid
  void setCuboid(const Kernel::V3D &, const Kernel::V3D &, const Kernel::V3D &,
                 const Kernel::V3D &);
  /// sets the geometry handler for a hexahedron
  void setHexahedron(const Kernel::V3D &, const Kernel::V3D &,
                     const Kernel::V3D &, const Kernel::V3D &,
                     const Kernel::V3D &, const Kernel::V3D &,
                     const Kernel::V3D &, const Kernel::V3D &);
  /// sets the geometry handler for a sphere
  void setSphere(const Kernel::V3D &center, double radius);
  /// sets the geometry handler for a cylinder
  void setCylinder(const Kernel::V3D &center, const Kernel::V3D &axis,
                   double radius, double height);
  /// sets the geometry handler for a cone
  void setCone(const Kernel::V3D &center, const Kernel::V3D &axis,
               double radius, double height);

  bool operator==(const ShapeInfo &other);
};
} // namespace detail
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SHAPEINFO_H_ */