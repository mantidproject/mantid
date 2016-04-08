#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/GluGeometryHandler.h"
#include "MantidGeometry/Rendering/GluGeometryRenderer.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;

GluGeometryHandler::GluGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(CUBOID) {}

GluGeometryHandler::GluGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(std::move(obj)),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(CUBOID) {}

GluGeometryHandler::GluGeometryHandler(Object *obj)
    : GeometryHandler(obj),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(CUBOID) {}

boost::shared_ptr<GeometryHandler> GluGeometryHandler::clone() const {
  auto clone = boost::make_shared<GluGeometryHandler>(*this);
  clone->Renderer =
      Kernel::make_unique<GluGeometryRenderer>(); // overwrite the renderer
  return clone;
}

GluGeometryHandler::~GluGeometryHandler() = default;

GeometryHandler *GluGeometryHandler::createInstance(IObjComponent *comp) {
  return new GluGeometryHandler(comp);
}

GeometryHandler *
GluGeometryHandler::createInstance(boost::shared_ptr<Object> obj) {
  return new GluGeometryHandler(obj);
}

GeometryHandler *GluGeometryHandler::createInstance(Object *obj) {
  return new GluGeometryHandler(obj);
}

void GluGeometryHandler::Triangulate() {
  // Check whether Object is triangulated otherwise triangulate
  // Doesn't have to do anything because we are not going to triangulate
  // anything
}

void GluGeometryHandler::Render() {
  if (Obj != nullptr) {
    switch (type) {
    case CUBOID:
      Renderer->RenderCube(m_points[0], m_points[1], m_points[2], m_points[3]);
      break;
    case HEXAHEDRON:
      Renderer->RenderHexahedron(m_points[0], m_points[1], m_points[2],
                                 m_points[3], m_points[4], m_points[5],
                                 m_points[6], m_points[7]);
      break;
    case SPHERE:
      Renderer->RenderSphere(m_points[0], radius);
      break;
    case CYLINDER:
      Renderer->RenderCylinder(m_points[0], m_points[1], radius, height);
      break;
    case CONE:
      Renderer->RenderCone(m_points[0], m_points[1], radius, height);
      break;
    case SEGMENTED_CYLINDER:
      Renderer->RenderSegmentedCylinder(m_points[0], m_points[1], radius,
                                        height);
      break;
    }
  } else if (ObjComp != nullptr) {
    Renderer->Render(ObjComp);
  }
}

ObjectGeometry GluGeometryHandler::GetObjectGeom() {
  ObjectGeometry result;
  result.type = static_cast<GEOMETRY_TYPE>(0);
  if (Obj != nullptr) {
    result.type = type;
    result.vectors = m_points;
    switch (type) {
    case CUBOID:
      result.type = 1;
      break;
    case HEXAHEDRON:
      break;
    case SPHERE:
      myradius = radius;
      break;
    default:
      myradius = radius;
      myheight = height;
      break;
    }
  }
}

void GluGeometryHandler::Initialize() {
  if (Obj != nullptr) {
    // There is no initialization or probably call render
    Render();
  }
}

void GluGeometryHandler::setCuboid(const V3D &p1, const V3D &p2, const V3D &p3,
                                   const V3D &p4) {
  type = CUBOID;
  m_points.assign({p1, p2, p3, p4});
}

void GluGeometryHandler::setHexahedron(const V3D &p1, const V3D &p2,
                                       const V3D &p3, const V3D &p4,
                                       const V3D &p5, const V3D &p6,
                                       const V3D &p7, const V3D &p8) {
  type = HEXAHEDRON;
  m_points.assign({p1, p2, p3, p4, p5, p6, p7, p8});
}

void GluGeometryHandler::setSphere(const V3D &c, double r) {
  type = SPHERE;
  m_points.assign({c});
  radius = r;
}
void GluGeometryHandler::setCylinder(const V3D &c, const V3D &a, double r,
                                     double h) {
  type = CYLINDER;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
void GluGeometryHandler::setCone(const V3D &c, const V3D &a, double r,
                                 double h) {
  type = CONE;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
void GluGeometryHandler::setSegmentedCylinder(const V3D &c, const V3D &a,
                                              double r, double h) {
  type = SEGMENTED_CYLINDER;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
}
}
