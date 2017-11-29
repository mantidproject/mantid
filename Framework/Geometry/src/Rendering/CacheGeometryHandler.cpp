#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/CacheGeometryHandler.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidKernel/MultiThreaded.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

using namespace detail;
CacheGeometryHandler::CacheGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp) {
  m_triangulator = nullptr;
}

CacheGeometryHandler::CacheGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(obj) {
  m_triangulator.reset(new GeometryTriangulator(obj.get()));
}

CacheGeometryHandler::CacheGeometryHandler(Object *obj) : GeometryHandler(obj) {
  m_triangulator.reset(new GeometryTriangulator(obj));
}

CacheGeometryHandler::CacheGeometryHandler(const CacheGeometryHandler &handler)
    : GeometryHandler(handler) {
  m_triangulator = nullptr;

  if (handler.Obj != nullptr) {
    Obj = handler.Obj;
    m_triangulator.reset(new GeometryTriangulator(Obj));
  } else if (handler.ObjComp != nullptr)
    ObjComp = handler.ObjComp;
}

boost::shared_ptr<GeometryHandler> CacheGeometryHandler::clone() const {
  auto clone = boost::make_shared<CacheGeometryHandler>(*this);
  if (this->m_triangulator)
    clone->m_triangulator.reset(new GeometryTriangulator(this->Obj));
  else
    clone->m_triangulator = nullptr;
  return clone;
}

CacheGeometryHandler::~CacheGeometryHandler() {
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
    m_triangulator->triangulate();
    boolTriangulated = true;
  }
}

void CacheGeometryHandler::Render() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    m_renderer.renderTriangulated(*m_triangulator);
  } else if (ObjComp != nullptr) {
    m_renderer.render(*ObjComp);
  }
}

void CacheGeometryHandler::Initialize() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    m_renderer.renderTriangulated(*m_triangulator);
  } else if (ObjComp != nullptr) {
    m_renderer.render(*ObjComp);
  }
}

size_t CacheGeometryHandler::numberOfTriangles() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->numTriangleFaces();
  }
  else {
    return 0;
  }
}

size_t CacheGeometryHandler::numberOfPoints() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->numTriangleVertices();
  }
  else {
    return 0;
  }
}

boost::optional<const std::vector<double> &>
CacheGeometryHandler::getTriangleVertices() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->getTriangleVertices();
  } else {
    return boost::none;
  }
}

boost::optional<const std::vector<int> &>
CacheGeometryHandler::getTriangleFaces() {
  if (Obj != nullptr) {
    Obj->updateGeometryHandler();
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->getTriangleFaces();
  } else {
    return boost::none;
  }
}

/**
Sets the geometry cache using the triangulation information provided
@param noPts :: the number of points
@param noFaces :: the number of faces
@param pts :: a double array of the points
@param faces :: an int array of the faces
*/
void CacheGeometryHandler::setGeometryCache(size_t nPts, size_t nFaces,
                                            std::vector<double> &&pts,
                                            std::vector<int> &&faces) {
  m_triangulator->setGeometryCache(nPts, nFaces, std::move(pts),
                                   std::move(faces));
  boolTriangulated = true;
}
}
}
