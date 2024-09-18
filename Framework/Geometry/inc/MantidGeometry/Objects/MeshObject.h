// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "BoundingBox.h"
#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Matrix.h"
#include <map>
#include <memory>

namespace Mantid {
//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
namespace Kernel {
class PseudoRandomNumberGenerator;
class V3D;
} // namespace Kernel

namespace Geometry {
class CompGrp;
class GeometryHandler;
class Track;
class vtkGeometryCacheReader;
class vtkGeometryCacheWriter;

/**
\class MeshObject
\brief Triangular Mesh Object
\version 1.0
\date Dec 2017
\author Karl Palmen

Mesh Object of Triangles assumed to form one or more
non-intersecting closed surfaces enclosing separate volumes.
The number of vertices is limited to 2^32 based on index type. For 2D Meshes see
Mesh2DObject
*/
class MANTID_GEOMETRY_DLL MeshObject : public IObject {
public:
  /// Constructor
  MeshObject(std::vector<uint32_t> faces, std::vector<Kernel::V3D> vertices, const Kernel::Material &material);
  /// Constructor
  MeshObject(std::vector<uint32_t> &&faces, std::vector<Kernel::V3D> &&vertices, const Kernel::Material &&material);

  /// Copy constructor
  MeshObject(const MeshObject &) = delete;
  /// Assignment operator
  MeshObject &operator=(const MeshObject &) = delete;
  /// Destructor
  virtual ~MeshObject() = default;
  /// Clone
  IObject *clone() const override { return new MeshObject(m_triangles, m_vertices, m_material); }
  IObject *cloneWithMaterial(const Kernel::Material &material) const override {
    return new MeshObject(m_triangles, m_vertices, material);
  }

  void setID(const std::string &id) override { m_id = id; }
  const std::string &id() const override { return m_id; }

  int getName() const override { return 0; }

  const Kernel::Material &material() const override;
  void setMaterial(const Kernel::Material &material) override;

  /// Return whether this object has a valid shape
  bool hasValidShape() const override;

  bool isValid(const Kernel::V3D &) const override; ///< Check if a point is inside
  bool isOnSide(const Kernel::V3D &) const override;
  int calcValidType(const Kernel::V3D &Pt, const Kernel::V3D &uVec) const;

  // INTERSECTION
  int interceptSurface(Geometry::Track &) const override;
  double distance(const Track &track) const override;

  // Solid angle - uses triangleSolidAngle unless many (>30000) triangles
  double solidAngle(const SolidAngleParams &params) const override;
  // Solid angle with a scaling of the object
  double solidAngle(const SolidAngleParams &params, const Kernel::V3D &scaleFactor) const override;

  /// Calculates the volume of this object.
  double volume() const override;

  /// Calculate (or return cached value of) Axis Aligned Bounding box
  /// (DEPRECATED)
  void getBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin,
                      double &zmin) const override;

  /// Return cached value of axis-aligned bounding box
  const BoundingBox &getBoundingBox() const override;

  // find internal point to object
  int getPointInObject(Kernel::V3D &point) const override;

  /// Select a random point within the object
  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const size_t) const override;
  std::optional<Kernel::V3D> generatePointInObject(Kernel::PseudoRandomNumberGenerator &rng,
                                                   const BoundingBox &activeRegion, const size_t) const override;

  // Rendering member functions
  void draw() const override;
  // Initialize Drawing
  void initDraw() const override;
  // Get Geometry Handler
  std::shared_ptr<GeometryHandler> getGeometryHandler() const override;
  /// Set Geometry Handler
  void setGeometryHandler(const std::shared_ptr<GeometryHandler> &h);

  detail::ShapeInfo::GeometryShape shape() const override;
  const detail::ShapeInfo &shapeInfo() const override;

  void GetObjectGeom(detail::ShapeInfo::GeometryShape &type, std::vector<Kernel::V3D> &vectors, double &innerRadius,
                     double &radius, double &height) const override;

  /// Read access to mesh object for rendering
  size_t numberOfVertices() const;
  std::vector<double> getVertices() const;
  const std::vector<Kernel::V3D> &getV3Ds() const;
  size_t numberOfTriangles() const;
  std::vector<uint32_t> getTriangles() const;

  void rotate(const Kernel::Matrix<double> &);
  void translate(const Kernel::V3D &);
  void multiply(const Kernel::Matrix<double> &);
  void scale(const double scaleFactor);
  void updateGeometryHandler();

private:
  void initialize();
  /// Get intersections
  void getIntersections(const Kernel::V3D &start, const Kernel::V3D &direction,
                        std::vector<Kernel::V3D> &intersectionPoints,
                        std::vector<Mantid::Geometry::TrackDirection> &entryExitFlags) const;

  /// Get triangle
  bool getTriangle(const size_t index, Kernel::V3D &v1, Kernel::V3D &v2, Kernel::V3D &v3) const;
  /// Search object for valid point
  bool searchForObject(Kernel::V3D &point) const;

  /// Cache for object's bounding box
  mutable BoundingBox m_boundingBox;

  /// Tolerence distance
  const double M_TOLERANCE = 0.000001;

  /// Geometry Handle for rendering
  std::shared_ptr<GeometryHandler> m_handler;

  // String from which object may be defined
  std::string m_string;

  /// string to return as ID
  std::string m_id;

  /// a pointer to a class for reading from the geometry cache
  std::shared_ptr<vtkGeometryCacheReader> m_vtk_cache_reader;
  /// a pointer to a class for writing to the geometry cache
  std::shared_ptr<vtkGeometryCacheWriter> m_vtk_cache_writer;

  /// Contents
  /// Triangles are specified by indices into a list of vertices.
  std::vector<uint32_t> m_triangles;
  std::vector<Kernel::V3D> m_vertices;
  /// material composition
  Kernel::Material m_material;
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid
