#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryRenderer.h"
#include "MantidGeometry/Rendering/CacheGeometryGenerator.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

CacheGeometryHandler::CacheGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp) {
  Triangulator = nullptr;
  Renderer = new CacheGeometryRenderer();
}

CacheGeometryHandler::CacheGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(obj) {
  Triangulator = new CacheGeometryGenerator(obj.get());
  Renderer = new CacheGeometryRenderer();
}

CacheGeometryHandler::CacheGeometryHandler(Object *obj) : GeometryHandler(obj) {
  Triangulator = new CacheGeometryGenerator(obj);
  Renderer = new CacheGeometryRenderer();
}

boost::shared_ptr<GeometryHandler> CacheGeometryHandler::clone() const {
  auto clone = boost::make_shared<CacheGeometryHandler>(*this);
  clone->Renderer = new CacheGeometryRenderer(*(this->Renderer));
  if (this->Triangulator)
    clone->Triangulator = new CacheGeometryGenerator(this->Obj);
  else
    clone->Triangulator = nullptr;
  return clone;
}

CacheGeometryHandler::~CacheGeometryHandler() {
  if (Triangulator != nullptr)
    delete Triangulator;
  if (Renderer != nullptr)
    delete Renderer;
}

GeometryHandler *CacheGeometryHandler::createInstance(IObjComponent *comp) {
  return new CacheGeometryHandler(comp);
}

GeometryHandler *
CacheGeometryHandler::createInstance(boost::shared_ptr<Object> obj) {
  return new CacheGeometryHandler(obj);
}

GeometryHandler *CacheGeometryHandler::createInstance(Object *obj) {
  return new CacheGeometryHandler(obj);
}

void CacheGeometryHandler::Triangulate() {
  // Check whether Object is triangulated otherwise triangulate
  PARALLEL_CRITICAL(Triangulate)
  if (Obj != nullptr && !boolTriangulated) {
    Triangulator->Generate();
    boolTriangulated = true;
  }
}

void CacheGeometryHandler::Render() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    Renderer->Render(
        Triangulator->getNumberOfPoints(), Triangulator->getNumberOfTriangles(),
        Triangulator->getTriangleVertices(), Triangulator->getTriangleFaces());
  } else if (ObjComp != nullptr) {
    Renderer->Render(ObjComp);
  }
}

void CacheGeometryHandler::Initialize() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    Renderer->Initialize(
        Triangulator->getNumberOfPoints(), Triangulator->getNumberOfTriangles(),
        Triangulator->getTriangleVertices(), Triangulator->getTriangleFaces());
  } else if (ObjComp != nullptr) {
    Renderer->Initialize(ObjComp);
  }
}

int CacheGeometryHandler::NumberOfTriangles() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getNumberOfTriangles();
  } else {
    return 0;
  }
}

int CacheGeometryHandler::NumberOfPoints() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getNumberOfPoints();
  } else {
    return 0;
  }
}

double *CacheGeometryHandler::getTriangleVertices() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getTriangleVertices();
  } else {
    return nullptr;
  }
}

int *CacheGeometryHandler::getTriangleFaces() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return Triangulator->getTriangleFaces();
  } else {
    return nullptr;
  }
}

/**
Sets the geometry cache using the triangulation information provided
@param noPts :: the number of points
@param noFaces :: the number of faces
@param pts :: a double array of the points
@param faces :: an int array of the faces
*/
void CacheGeometryHandler::setGeometryCache(int noPts, int noFaces, double *pts,
                                            int *faces) {
  Triangulator->setGeometryCache(noPts, noFaces, pts, faces);
  boolTriangulated = true;
}
}
}
