#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/OCGeometryGenerator.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
OCGeometryHandler::OCGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp) {
  Triangulator = nullptr;
}

OCGeometryHandler::OCGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(obj) {
  Triangulator = new OCGeometryGenerator(obj.get());
}

OCGeometryHandler::OCGeometryHandler(Object *obj) : GeometryHandler(obj) {
  Triangulator = new OCGeometryGenerator(obj);
}

boost::shared_ptr<GeometryHandler> OCGeometryHandler::clone() const {
  auto clone = boost::make_shared<OCGeometryHandler>(*this);
  if (this->Triangulator)
    clone->Triangulator = new OCGeometryGenerator(this->Obj);
  return clone;
}

OCGeometryHandler::~OCGeometryHandler() {
  if (Triangulator != nullptr)
    delete Triangulator;
}

GeometryHandler *OCGeometryHandler::createInstance(IObjComponent *comp) {
  return new OCGeometryHandler(comp);
}

GeometryHandler *
OCGeometryHandler::createInstance(boost::shared_ptr<Object> obj) {
  return new OCGeometryHandler(obj);
}

GeometryHandler *OCGeometryHandler::createInstance(Object *obj) {
  return new OCGeometryHandler(obj);
}

void OCGeometryHandler::Triangulate() {
  // Check whether Object is triangulated otherwise triangulate
  if (Obj != nullptr && !boolTriangulated) {
    Triangulator->Generate();
    boolTriangulated = true;
  }
}

void OCGeometryHandler::Render() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    m_renderer.render(Triangulator->getObjectSurface());
  } else if (ObjComp != nullptr) {
    m_renderer.render(ObjComp);
  }
}

void OCGeometryHandler::Initialize() { Render(); }

int OCGeometryHandler::NumberOfTriangles() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getNumberOfTriangles();
  } else {
    return 0;
  }
}

int OCGeometryHandler::NumberOfPoints() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getNumberOfPoints();
  } else {
    return 0;
  }
}

double *OCGeometryHandler::getTriangleVertices() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getTriangleVertices();
  } else {
    return nullptr;
  }
}

int *OCGeometryHandler::getTriangleFaces() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getTriangleFaces();
  } else {
    return nullptr;
  }
}
} // namespace Geometry
} // namespace Mantid
