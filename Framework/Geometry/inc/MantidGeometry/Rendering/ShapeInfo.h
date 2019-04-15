// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_SHAPEINFO_H_
#define MANTID_GEOMETRY_SHAPEINFO_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/V3D.h"
#include <iosfwd>
#include <vector>

namespace Mantid {
namespace Geometry {
class RectangularDetector;
class StructuredDetector;
class IObjComponent;

/** ShapeInfo : Stores shape types and information relevant to drawing the
shape. For cylinders, spheres and cones, height and radius are stored. Points
are stored in the winding order shown in the IDF here
http://docs.mantidproject.org/nightly/concepts/HowToDefineGeometricShape.html.
*/
namespace detail {
class MANTID_GEOMETRY_DLL ShapeInfo {
public:
  enum class GeometryShape {
    NOSHAPE = 0,
    CUBOID,         ///< CUBOID
    HEXAHEDRON,     ///< HEXAHEDRON
    SPHERE,         ///< SPHERE
    CYLINDER,       ///< CYLINDER
    CONE,           ///< CONE
    HOLLOWCYLINDER, ///< HOLLOW CYLINDER
  };

  struct CuboidGeometry {
    const Kernel::V3D &leftFrontBottom;
    const Kernel::V3D &leftFrontTop;
    const Kernel::V3D &leftBackBottom;
    const Kernel::V3D &rightFrontBottom;
  };
  struct HexahedronGeometry {
    const Kernel::V3D &leftBackBottom;
    const Kernel::V3D &leftFrontBottom;
    const Kernel::V3D &rightFrontBottom;
    const Kernel::V3D &rightBackBottom;
    const Kernel::V3D &leftBackTop;
    const Kernel::V3D &leftFrontTop;
    const Kernel::V3D &rightFrontTop;
    const Kernel::V3D &rightBackTop;
  };
  struct SphereGeometry {
    const Kernel::V3D &centre;
    double radius;
  };
  struct CylinderGeometry {
    const Kernel::V3D &centreOfBottomBase;
    const Kernel::V3D &axis;
    double radius;
    double height;
  };
  struct ConeGeometry {
    const Kernel::V3D &centre;
    const Kernel::V3D &axis;
    double radius;
    double height;
  };
  struct HollowCylinderGeometry {
    const Kernel::V3D &centreOfBottomBase;
    const Kernel::V3D &axis;
    double innerRadius;
    double radius;
    double height;
  };

private:
  std::vector<Kernel::V3D> m_points;
  double m_radius;      ///< Radius for the sphere, cone and cylinder;
                        ///< Also outer radius for hollow cylinder;
  double m_height;      ///< height for cone, cylinder and hollow cylinder;
  double m_innerRadius; ///< Inner radius for hollow cylinder
  GeometryShape m_shape;

public:
  ShapeInfo();
  ShapeInfo(const ShapeInfo &) = default;
  const std::vector<Kernel::V3D> &points() const;
  double radius() const;
  double innerRadius() const;
  double height() const;
  GeometryShape shape() const;

  void getObjectGeometry(GeometryShape &shape, std::vector<Kernel::V3D> &points,
                         double &innerRadius, double &radius,
                         double &height) const;

  CuboidGeometry cuboidGeometry() const;
  HexahedronGeometry hexahedronGeometry() const;
  SphereGeometry sphereGeometry() const;
  CylinderGeometry cylinderGeometry() const;
  ConeGeometry coneGeometry() const;
  HollowCylinderGeometry hollowCylinderGeometry() const;
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
  void setCylinder(const Kernel::V3D &centerBottomBase,
                   const Kernel::V3D &symmetryAxis, double radius,
                   double height);
  /// sets the geometry handler for a cone
  void setCone(const Kernel::V3D &center, const Kernel::V3D &symmetryAxis,
               double radius, double height);
  /// sets the geometry handler for a hollow cylinder
  void setHollowCylinder(const Kernel::V3D &centreBottomBase,
                         const Kernel::V3D &symmetryAxis, double innerRadius,
                         double outerRadius, double height);
  bool operator==(const ShapeInfo &other);
};

MANTID_GEOMETRY_DLL std::ostream &
operator<<(std::ostream &os, const ShapeInfo::GeometryShape shape);

} // namespace detail
} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_SHAPEINFO_H_ */
