// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GEOMETRYHANDLER_H
#define GEOMETRYHANDLER_H

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Rendering/RenderingMesh.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/V3D.h"

#include <boost/shared_ptr.hpp>
#include <memory>
#include <vector>

namespace Mantid {

namespace Geometry {
class IObjComponent;
class CSGObject;
class MeshObject2D;
class MeshObject;
namespace detail {
class Renderer;
class GeometryTriangulator;

template <typename Adaptee>
std::unique_ptr<Geometry::RenderingMesh>
makeRenderingMesh(const Adaptee &adaptee) {

  // Local class adapter
  class Adapter : public Geometry::RenderingMesh {
  private:
    // Not owned but can be guraranteed not null
    const Adaptee &m_adaptee;

  public:
    Adapter(const Adaptee &adaptee) : m_adaptee(adaptee) {}
    size_t numberOfVertices() const override {
      return m_adaptee.numberOfVertices();
    }
    size_t numberOfTriangles() const override {
      return m_adaptee.numberOfTriangles();
    }
    std::vector<double> getVertices() const override {
      return m_adaptee.getVertices();
    }
    std::vector<uint32_t> getTriangles() const override {
      return m_adaptee.getTriangles();
    }
    virtual ~Adapter() {}
  };
  return std::make_unique<Adapter>(adaptee);
}
} // namespace detail

/**
\class GeometryHandler
\brief Handles rendering of all object Geometry.
\author Lamar Moore
\date December 2017

Handles the rendering of all geometry types in Mantid.
*/
class MANTID_GEOMETRY_DLL GeometryHandler {
private:
  static Kernel::Logger &PLog; ///< The official logger

protected:
  std::shared_ptr<detail::ShapeInfo> m_shapeInfo;
  std::unique_ptr<detail::GeometryTriangulator> m_triangulator;
  IObjComponent *m_objComp =
      nullptr; ///< ObjComponent that uses this geometry handler
  CSGObject *m_csgObj = nullptr; ///< Object that uses this geometry handler
public:
  GeometryHandler(IObjComponent *comp);              ///< Constructor
  GeometryHandler(boost::shared_ptr<CSGObject> obj); ///< Constructor
  GeometryHandler(CSGObject *obj);                   ///< Constructor
  GeometryHandler(const MeshObject &obj);
  GeometryHandler(const MeshObject2D &obj);
  GeometryHandler(const GeometryHandler &handler);
  boost::shared_ptr<GeometryHandler> clone() const;
  ~GeometryHandler();
  void render() const; ///< Render Object or ObjComponent
  void
  initialize() const; ///< Prepare/Initialize Object/ObjComponent to be rendered
  bool canTriangulate() const { return !(m_triangulator == nullptr); }
  /// get the number of triangles
  size_t numberOfTriangles() const;
  /// get the number of points or vertices
  size_t numberOfPoints() const;

  bool hasShapeInfo() const { return !(m_shapeInfo == nullptr); }
  const detail::ShapeInfo &shapeInfo() const { return *m_shapeInfo; }
  /// Extract the vertices of the triangles
  const std::vector<double> &getTriangleVertices() const;
  /// Extract the Faces of the triangles
  const std::vector<uint32_t> &getTriangleFaces() const;
  /// Sets the geometry cache using the triangulation information provided
  void setGeometryCache(size_t nPts, size_t nFaces, std::vector<double> &&pts,
                        std::vector<uint32_t> &&faces);
  /// return the actual type and points of one of the "standard" objects,
  /// cuboid/cone/cyl/sphere
  void GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                     std::vector<Kernel::V3D> &vector, double &innerRadius,
                     double &radius, double &height) const;
  void setShapeInfo(detail::ShapeInfo &&shapeInfo);
};

} // NAMESPACE Geometry
} // NAMESPACE Mantid

#endif
