#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidGeometry/Rendering/Renderer.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
GeometryHandler::GeometryHandler(IObjComponent *comp)
    : m_objComp(comp), m_renderer(new detail::Renderer()) {}

GeometryHandler::GeometryHandler(boost::shared_ptr<Object> obj)
    : m_obj(obj.get()),
      m_triangulator(new detail::GeometryTriangulator(obj.get())),
      m_renderer(new detail::Renderer()) {}

GeometryHandler::GeometryHandler(Object *obj)
    : m_obj(obj), m_triangulator(new detail::GeometryTriangulator(obj)),
      m_renderer(new detail::Renderer()) {}

GeometryHandler::GeometryHandler(RectangularDetector *comp)
    : m_rectDet(comp), m_renderer(new detail::Renderer()) {}

GeometryHandler::GeometryHandler(StructuredDetector *comp)
    : m_structDet(comp), m_renderer(new detail::Renderer()) {}

GeometryHandler::GeometryHandler(const GeometryHandler &renderer)
    : m_renderer(new detail::Renderer()) {
  if (renderer.m_obj) {
    m_obj = renderer.m_obj;
    if (renderer.m_triangulator)
      m_triangulator.reset(new detail::GeometryTriangulator(m_obj));
  }
  if (renderer.m_objComp)
    m_objComp = renderer.m_objComp;
  if (renderer.m_shapeInfo)
    m_shapeInfo = renderer.m_shapeInfo;
  if (renderer.m_structDet)
    m_structDet = renderer.m_structDet;
  if (renderer.m_rectDet)
    m_rectDet = renderer.m_rectDet;
}

boost::shared_ptr<GeometryHandler> GeometryHandler::clone() const {
  return boost::make_shared<GeometryHandler>(GeometryHandler(*this));
}
GeometryHandler::~GeometryHandler() {}

void GeometryHandler::render() {
  if (m_rectDet)
    m_renderer->renderBitmap(*m_rectDet);
  else if (m_structDet)
    m_renderer->renderStructured(*m_structDet);
  else if (m_shapeInfo)
    m_renderer->renderShape(*m_shapeInfo);
  else if (m_objComp != nullptr)
    m_renderer->renderIObjComponent(*m_objComp);
  else if (canTriangulate())
    m_renderer->renderTriangulated(*m_triangulator);
}

void GeometryHandler::initialize() {
  if (m_obj != nullptr)
    m_obj->updateGeometryHandler();
  render();
}

size_t GeometryHandler::numberOfTriangles() {
  if (canTriangulate())
    return m_triangulator->numTriangleFaces();
  return 0;
}

size_t GeometryHandler::numberOfPoints() {
  if (canTriangulate())
    return m_triangulator->numTriangleVertices();
  return 0;
}

boost::optional<const std::vector<double> &>
GeometryHandler::getTriangleVertices() {
  if (canTriangulate())
    return m_triangulator->getTriangleVertices();
  return boost::none;
}

boost::optional<const std::vector<int> &> GeometryHandler::getTriangleFaces() {
  if (canTriangulate())
    return m_triangulator->getTriangleFaces();
  return boost::none;
}

void GeometryHandler::setGeometryCache(size_t nPts, size_t nFaces,
                                       std::vector<double> &&pts,
                                       std::vector<int> &&faces) {
  if (canTriangulate()) {
    m_triangulator->setGeometryCache(nPts, nFaces, std::move(pts),
                                     std::move(faces));
  }
}

void GeometryHandler::GetObjectGeom(detail::ShapeInfo::GeometryShape &mytype,
                                    std::vector<Kernel::V3D> &vectors,
                                    double &myradius, double &myheight) const {
  mytype = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_shapeInfo)
    m_shapeInfo->getObjectGeometry(mytype, vectors, myradius, myheight);
}

void GeometryHandler::setShapeInfo(detail::ShapeInfo &&shapeInfo) {
  m_triangulator.reset(nullptr);
  m_shapeInfo.reset(new detail::ShapeInfo(std::move(shapeInfo)));
}
} // namespace Geometry
} // namespace Mantid