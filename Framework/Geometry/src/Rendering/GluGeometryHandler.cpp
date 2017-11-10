#include "MantidGeometry/Rendering/GluGeometryHandler.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/GluGeometryRenderer.h"
#include "MantidKernel/make_unique.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;

GluGeometryHandler::GluGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(GeometryType::NOSHAPE) {}

GluGeometryHandler::GluGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(std::move(obj)),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(GeometryType::NOSHAPE) {}

GluGeometryHandler::GluGeometryHandler(Object *obj)
    : GeometryHandler(obj),
      Renderer(Kernel::make_unique<GluGeometryRenderer>()), radius(0.0),
      height(0.0), type(GeometryType::NOSHAPE) {}

GluGeometryHandler::GluGeometryHandler(const GluGeometryHandler &other)
    : GeometryHandler(other), m_points(other.m_points), radius(other.radius),
      height(other.height), type(other.type) {
  this->Renderer = Kernel::make_unique<GluGeometryRenderer>();
}

boost::shared_ptr<GeometryHandler> GluGeometryHandler::clone() const {
  return boost::make_shared<GluGeometryHandler>(*this);
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
    case GeometryType::CUBOID:
      Renderer->RenderCube(m_points[0], m_points[1], m_points[2], m_points[3]);
      break;
    case GeometryType::HEXAHEDRON:
      Renderer->RenderHexahedron(m_points[0], m_points[1], m_points[2],
                                 m_points[3], m_points[4], m_points[5],
                                 m_points[6], m_points[7]);
      break;
    case GeometryType::SPHERE:
      Renderer->RenderSphere(m_points[0], radius);
      break;
    case GeometryType::CYLINDER:
      Renderer->RenderCylinder(m_points[0], m_points[1], radius, height);
      break;
    case GeometryType::CONE:
      Renderer->RenderCone(m_points[0], m_points[1], radius, height);
      break;
    case GeometryType::SEGMENTED_CYLINDER:
      Renderer->RenderSegmentedCylinder(m_points[0], m_points[1], radius,
                                        height);
      break;
    case GeometryType::NOSHAPE:
      break;
    }
  } else if (ObjComp != nullptr) {
    Renderer->Render(ObjComp);
  }
}

void GluGeometryHandler::GetObjectGeom(int &mytype,
                                       std::vector<Kernel::V3D> &vectors,
                                       double &myradius, double &myheight) {
  mytype = 0;
  if (Obj != nullptr) {
    mytype = static_cast<int>(type);
    vectors = m_points;
    switch (type) {
    case GeometryType::CUBOID:
      break;
    case GeometryType::HEXAHEDRON:
      break;
    case GeometryType::SPHERE:
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
  type = GeometryType::CUBOID;
  m_points.assign({p1, p2, p3, p4});
}

void GluGeometryHandler::setHexahedron(const V3D &p1, const V3D &p2,
                                       const V3D &p3, const V3D &p4,
                                       const V3D &p5, const V3D &p6,
                                       const V3D &p7, const V3D &p8) {
  type = GeometryType::HEXAHEDRON;
  m_points.assign({p1, p2, p3, p4, p5, p6, p7, p8});
}

void GluGeometryHandler::setSphere(const V3D &c, double r) {
  type = GeometryType::SPHERE;
  m_points.assign({c});
  radius = r;
}
void GluGeometryHandler::setCylinder(const V3D &c, const V3D &a, double r,
                                     double h) {
  type = GeometryType::CYLINDER;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
void GluGeometryHandler::setCone(const V3D &c, const V3D &a, double r,
                                 double h) {
  type = GeometryType::CONE;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
void GluGeometryHandler::setSegmentedCylinder(const V3D &c, const V3D &a,
                                              double r, double h) {
  type = GeometryType::SEGMENTED_CYLINDER;
  m_points.assign({c, a});
  radius = r;
  height = h;
}
}
}
