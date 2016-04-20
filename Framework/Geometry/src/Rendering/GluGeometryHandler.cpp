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
    : GeometryHandler(comp), radius(0.0), height(0.0), type(CUBOID) {
  Renderer = new GluGeometryRenderer();
}

GluGeometryHandler::GluGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(obj), radius(0.0), height(0.0), type(CUBOID) {
  Renderer = new GluGeometryRenderer();
}

GluGeometryHandler::GluGeometryHandler(Object *obj)
    : GeometryHandler(obj), radius(0.0), height(0.0), type(CUBOID) {
  Renderer = new GluGeometryRenderer();
}

boost::shared_ptr<GeometryHandler> GluGeometryHandler::clone() const {
  auto clone = boost::make_shared<GluGeometryHandler>(*this);
  clone->Renderer = new GluGeometryRenderer(); // overwrite the renderer
  return clone;
}

GluGeometryHandler::~GluGeometryHandler() {
  if (Renderer != nullptr)
    delete Renderer;
}

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
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderCube(Point1, Point2, Point3, Point4);
      break;
    case HEXAHEDRON:
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderHexahedron(Point1, Point2, Point3, Point4, Point5, Point6,
                             Point7, Point8);
      break;
    case SPHERE:
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderSphere(center, radius);
      break;
    case CYLINDER:
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderCylinder(center, axis, radius, height);
      break;
    case CONE:
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderCone(center, axis, radius, height);
      break;
    case SEGMENTED_CYLINDER:
      (dynamic_cast<GluGeometryRenderer *>(Renderer))
          ->RenderSegmentedCylinder(center, axis, radius, height);
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
    switch (type) {
    case CUBOID:
      mytype = 1;
      vectors.assign({Point1, Point2, Point3, Point4});
      break;
    case HEXAHEDRON:
      vectors.assign(
          {Point1, Point2, Point3, Point4, Point5, Point6, Point7, Point8});
      break;
    case SPHERE:
      vectors.assign({center});
      myradius = radius;
      break;
    default:
      vectors.assign({center, axis});
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
  Point1 = p1;
  Point2 = p2;
  Point3 = p3;
  Point4 = p4;
}

void GluGeometryHandler::setHexahedron(const V3D &p1, const V3D &p2,
                                       const V3D &p3, const V3D &p4,
                                       const V3D &p5, const V3D &p6,
                                       const V3D &p7, const V3D &p8) {
  type = HEXAHEDRON;
  Point1 = p1;
  Point2 = p2;
  Point3 = p3;
  Point4 = p4;
  Point5 = p5;
  Point6 = p6;
  Point7 = p7;
  Point8 = p8;
}

void GluGeometryHandler::setSphere(const V3D &c, double r) {
  type = SPHERE;
  center = c;
  radius = r;
}
void GluGeometryHandler::setCylinder(const V3D &c, const V3D &a, double r,
                                     double h) {
  type = CYLINDER;
  center = c;
  axis = a;
  radius = r;
  height = h;
}
void GluGeometryHandler::setCone(const V3D &c, const V3D &a, double r,
                                 double h) {
  type = CONE;
  center = c;
  axis = a;
  radius = r;
  height = h;
}
void GluGeometryHandler::setSegmentedCylinder(const V3D &c, const V3D &a,
                                              double r, double h) {
  type = SEGMENTED_CYLINDER;
  center = c;
  axis = a;
  radius = r;
  height = h;
}
}
}
