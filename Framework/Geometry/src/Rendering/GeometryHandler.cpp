// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Rendering/GeometryTriangulator.h"
#include "MantidGeometry/Rendering/RenderingHelpers.h"
#include "MantidGeometry/Rendering/RenderingMesh.h"
#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {

GeometryHandler::GeometryHandler(IObjComponent *comp) : m_objComp(comp) {}

GeometryHandler::GeometryHandler(boost::shared_ptr<CSGObject> obj)
    : m_triangulator(new detail::GeometryTriangulator(obj.get())),
      m_csgObj(obj.get()) {}

GeometryHandler::GeometryHandler(CSGObject *obj)
    : m_triangulator(new detail::GeometryTriangulator(obj)), m_csgObj(obj) {}

GeometryHandler::GeometryHandler(const MeshObject &obj)
    : m_triangulator(
          new detail::GeometryTriangulator(detail::makeRenderingMesh(obj))) {}

GeometryHandler::GeometryHandler(const MeshObject2D &obj)
    : m_triangulator(
          new detail::GeometryTriangulator(detail::makeRenderingMesh(obj))) {}

GeometryHandler::GeometryHandler(const GeometryHandler &handler) {
  if (handler.m_csgObj) {
    m_csgObj = handler.m_csgObj;
    if (handler.m_triangulator)
      m_triangulator.reset(new detail::GeometryTriangulator(m_csgObj));
  }
  if (handler.m_objComp)
    m_objComp = handler.m_objComp;
  if (handler.m_shapeInfo)
    m_shapeInfo = handler.m_shapeInfo;
}

/// Destructor
GeometryHandler::~GeometryHandler() {}

boost::shared_ptr<GeometryHandler> GeometryHandler::clone() const {
  return boost::make_shared<GeometryHandler>(*this);
}

void GeometryHandler::render() const {
  if (m_shapeInfo)
    RenderingHelpers::renderShape(*m_shapeInfo);
  else if (m_objComp != nullptr)
    RenderingHelpers::renderIObjComponent(*m_objComp);
  else if (canTriangulate())
    RenderingHelpers::renderTriangulated(*m_triangulator);
}

void GeometryHandler::initialize() const {
  if (m_csgObj != nullptr)
    m_csgObj->updateGeometryHandler();
  render();
}

size_t GeometryHandler::numberOfTriangles() const {
  if (canTriangulate())
    return m_triangulator->numTriangleFaces();
  return 0;
}

size_t GeometryHandler::numberOfPoints() const {
  if (canTriangulate())
    return m_triangulator->numTriangleVertices();
  return 0;
}

const std::vector<double> &GeometryHandler::getTriangleVertices() const {
  static std::vector<double> empty;
  if (canTriangulate())
    return m_triangulator->getTriangleVertices();
  return empty;
}

const std::vector<uint32_t> &GeometryHandler::getTriangleFaces() const {
  static std::vector<uint32_t> empty;
  if (canTriangulate())
    return m_triangulator->getTriangleFaces();
  return empty;
}

void GeometryHandler::setGeometryCache(size_t nPts, size_t nFaces,
                                       std::vector<double> &&pts,
                                       std::vector<uint32_t> &&faces) {
  if (canTriangulate()) {
    m_triangulator->setGeometryCache(nPts, nFaces, std::move(pts),
                                     std::move(faces));
  }
}

void GeometryHandler::GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                                    std::vector<Kernel::V3D> &vectors,
                                    double &radius, double &height) const {
  type = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_shapeInfo)
    m_shapeInfo->getObjectGeometry(type, vectors, radius, height);
}

void GeometryHandler::GetObjectGeom(detail::ShapeInfo::GeometryShape &type,
                                    std::vector<Kernel::V3D> &vectors,
                                    double &innerRadius, double &radius,
                                    double &height) const {
  type = detail::ShapeInfo::GeometryShape::NOSHAPE;
  if (m_shapeInfo)
    m_shapeInfo->getObjectGeometry(type, vectors, innerRadius, radius, height);
}

void GeometryHandler::setShapeInfo(detail::ShapeInfo &&shapeInfo) {
  m_triangulator.reset(nullptr);
  m_shapeInfo.reset(new detail::ShapeInfo(std::move(shapeInfo)));
}
} // namespace Geometry
} // namespace Mantid
