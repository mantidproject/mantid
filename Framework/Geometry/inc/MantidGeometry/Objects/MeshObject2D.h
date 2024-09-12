// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include <memory>
#include <vector>

namespace Mantid {

namespace Geometry {
class Track;
class GeometryHandler;

/** MeshObject2D :

  Defines an IObject implemented as a 2D mesh composed of triangles. Avoids
  assumptions made in MeshObject to do with closed surfaces, non-zero volumes
  and assoicated additional runtime costs. The number of vertices is limited to
  2^16 based on index type.
*/
class MANTID_GEOMETRY_DLL MeshObject2D : public IObject {
public:
  /// Constructor
  MeshObject2D(std::vector<uint32_t> faces, std::vector<Kernel::V3D> vertices, const Kernel::Material &material);
  /// Constructor
  MeshObject2D(std::vector<uint32_t> &&faces, std::vector<Kernel::V3D> &&vertices, const Kernel::Material &&material);

  double volume() const override;

  static bool pointsCoplanar(const std::vector<Kernel::V3D> &vertices);

  bool hasValidShape() const override;
  double distanceToPlane(const Kernel::V3D &point) const;
  bool isValid(const Kernel::V3D &point) const override; ///< Check if a point is inside
  bool isOnSide(const Kernel::V3D &) const override;
  int interceptSurface(Geometry::Track &ut) const override;
  double distance(const Geometry::Track &ut) const override;
  MeshObject2D *clone() const override;
  MeshObject2D *cloneWithMaterial(const Kernel::Material &material) const override;
  int getName() const override;
  double solidAngle(const SolidAngleParams &params) const override;
  double solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const override;
  bool operator==(const MeshObject2D &other) const;
  const BoundingBox &getBoundingBox() const override;
  const static double MinThickness;

  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                      double &zmin) const override;
  int getPointInObject(Kernel::V3D &point) const override;

  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const size_t) const override;
  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const BoundingBox &activeRegion, const size_t) const override;
  detail::ShapeInfo::GeometryShape shape() const override;
  const detail::ShapeInfo &shapeInfo() const override;
  void GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors, double &innerRadius,
                     double &radius, double &height) const override;
  void draw() const override;
  void initDraw() const override;
  const Kernel::Material &material() const override;
  virtual void setMaterial(const Kernel::Material &material) override;
  const std::string &id() const override;
  void setID(const std::string &id) override { m_id = id; };
  std::shared_ptr<GeometryHandler> getGeometryHandler() const override;
  /// Id as static
  static const std::string Id;
  size_t numberOfVertices() const;
  size_t numberOfTriangles() const;
  std::vector<double> getVertices() const;
  std::vector<uint32_t> getTriangles() const;

private:
  struct PlaneParameters {
    double a;
    double b;
    double c;
    double k;
    Kernel::V3D normal;
    Kernel::V3D p0;
    double abs_normal;
  } m_planeParameters;

  void initialize();
  /// Triangles are specified by indices into a list of vertices. Offset is
  /// always 3.
  std::vector<uint32_t> m_triangles;
  /// Vertices
  std::vector<Kernel::V3D> m_vertices;
  /// optional string identifier
  std::string m_id;
  /// Material composition
  Kernel::Material m_material;
  /// Bounding box
  mutable BoundingBox m_boundingBox;
  /// Geometry Handle for rendering
  std::shared_ptr<GeometryHandler> m_handler;
};

} // namespace Geometry
} // namespace Mantid
