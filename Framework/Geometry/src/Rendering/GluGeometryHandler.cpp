#include "MantidGeometry/Rendering/GluGeometryHandler.h"
#include "MantidGeometry/Instrument/ObjComponent.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/make_unique.h"

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;
using namespace detail;

GluGeometryHandler::GluGeometryHandler(IObjComponent *comp)
    : GeometryHandler(comp), radius(0.0), height(0.0){}

GluGeometryHandler::GluGeometryHandler(boost::shared_ptr<Object> obj)
    : GeometryHandler(std::move(obj)), radius(0.0), height(0.0) {}

GluGeometryHandler::GluGeometryHandler(Object *obj)
    : GeometryHandler(obj), radius(0.0), height(0.0) {}

GluGeometryHandler::GluGeometryHandler(const GluGeometryHandler &other)
    : GeometryHandler(other), m_points(other.m_points), radius(other.radius),
      height(other.height), m_shapeInfo(other.m_shapeInfo) {
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
    m_renderer.renderShape(m_shapeInfo);
  } else if (ObjComp != nullptr) {
    m_renderer.render(*ObjComp);
  }
}

void GluGeometryHandler::GetObjectGeom(int &mytype,
                                       std::vector<Kernel::V3D> &vectors,
                                       double &myradius, double &myheight) {
  m_shapeInfo.getObjectGeometry(mytype, vectors, myradius, myheight);
}

void GluGeometryHandler::Initialize() { Render(); }

void GluGeometryHandler::setShapeInfo(detail::ShapeInfo &&shapeInfo)
{
  m_shapeInfo = std::move(shapeInfo);
}

}
}
