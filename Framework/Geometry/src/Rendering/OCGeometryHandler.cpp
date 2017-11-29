#include "MantidGeometry/Rendering/OCGeometryHandler.h"
#include "MantidGeometry/IObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using namespace detail;

OCGeometryHandler::OCGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp) {
  m_triangulator = nullptr;
}

OCGeometryHandler::OCGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(obj) {
  m_triangulator.reset(new GeometryTriangulator(obj.get()));
}

OCGeometryHandler::OCGeometryHandler(Object *obj) : GeometryHandler(obj) {
  m_triangulator.reset(new GeometryTriangulator(obj));
}

OCGeometryHandler::OCGeometryHandler(const OCGeometryHandler &handler)
    : GeometryHandler(handler) {
  m_triangulator = nullptr;

  if (handler.Obj != nullptr) {
    Obj = handler.Obj;
    m_triangulator.reset(new GeometryTriangulator(Obj));
  } else if (handler.ObjComp != nullptr)
    ObjComp = handler.ObjComp;
}

boost::shared_ptr<GeometryHandler> OCGeometryHandler::clone() const {
  auto clone = boost::make_shared<OCGeometryHandler>(*this);
  if (this->m_triangulator)
    clone->m_triangulator.reset(new GeometryTriangulator(this->Obj));
  return clone;
}

OCGeometryHandler::~OCGeometryHandler() {
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
    m_triangulator->triangulate();
    boolTriangulated = true;
  }
}

void OCGeometryHandler::Render() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    m_renderer.render(m_triangulator->getOCSurface());
  } else if (ObjComp != nullptr) {
    m_renderer.render(*ObjComp);
  }
}

void OCGeometryHandler::Initialize() { Render(); }

size_t OCGeometryHandler::numberOfTriangles() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->numTriangleFaces();
  }
  else {
    return 0;
  }
}

size_t OCGeometryHandler::numberOfPoints() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->numTriangleVertices();
  }
  else {
    return 0;
  }
}
boost::optional<const std::vector<double> &> OCGeometryHandler::getTriangleVertices() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->getTriangleVertices();
  } else {
    return boost::none;
  }
}

boost::optional<const std::vector<int> &> OCGeometryHandler::getTriangleFaces() {
  if (Obj != nullptr) {
    if (!boolTriangulated)
      Triangulate();
    return m_triangulator->getTriangleFaces();
  } else {
    return boost::none;
  }
}
} // namespace Geometry
} // namespace Mantid
