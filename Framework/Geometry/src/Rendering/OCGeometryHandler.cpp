#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#include "MantidGeometry/Rendering/OCGeometryGenerator.h"
#include "MantidGeometry/Rendering/OCGeometryRenderer.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
OCGeometryHandler::OCGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp) {
  Triangulator = nullptr;
  Renderer = new OCGeometryRenderer();
}

OCGeometryHandler::OCGeometryHandler(boost::shared_ptr<CSGObject> obj)
    : GeometryHandler(obj) {
  Triangulator = new OCGeometryGenerator(obj.get());
  Renderer = new OCGeometryRenderer();
}

OCGeometryHandler::OCGeometryHandler(CSGObject *obj) : GeometryHandler(obj) {
  Triangulator = new OCGeometryGenerator(obj);
  Renderer = new OCGeometryRenderer();
}

boost::shared_ptr<GeometryHandler> OCGeometryHandler::clone() const {
  auto clone = boost::make_shared<OCGeometryHandler>(*this);
  clone->Renderer = new OCGeometryRenderer(*(this->Renderer));
  if (this->Triangulator)
    clone->Triangulator = new OCGeometryGenerator(this->Obj);
  return clone;
}

OCGeometryHandler::~OCGeometryHandler() {
  if (Triangulator != nullptr)
    delete Triangulator;
  if (Renderer != nullptr)
    delete Renderer;
}

GeometryHandler *OCGeometryHandler::createInstance(IObjComponent *comp) {
  return new OCGeometryHandler(comp);
}

GeometryHandler *
OCGeometryHandler::createInstance(boost::shared_ptr<CSGObject> obj) {
  return new OCGeometryHandler(obj);
}

GeometryHandler *OCGeometryHandler::createInstance(CSGObject *obj) {
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
    Renderer->Render(Triangulator->getObjectSurface());
  } else if (ObjComp != nullptr) {
    Renderer->Render(ObjComp);
  }
}

void OCGeometryHandler::Initialize() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    Renderer->Initialize(Triangulator->getObjectSurface());
  } else if (ObjComp != nullptr) {
    Renderer->Initialize(ObjComp);
  }
}

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
}
}
