#include "MantidGeometry/Rendering/BitmapGeometryHandler.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Instrument/RectangularDetector.h"
#include <climits>
#include <iostream>

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;

/**
 * @return A shared_ptr to a new copy of this object
 */
boost::shared_ptr<GeometryHandler> BitmapGeometryHandler::clone() const {
  return boost::make_shared<BitmapGeometryHandler>(*this);
}

/// Parameter constructor
BitmapGeometryHandler::BitmapGeometryHandler(RectangularDetector *comp)
    : GeometryHandler(dynamic_cast<IObjComponent *>(comp)) {
  // Save the rectangular detector link for later.
  m_rectDet = comp;
}

BitmapGeometryHandler::BitmapGeometryHandler()
    : GeometryHandler(static_cast<Object *>(nullptr)), m_rectDet(nullptr) {}

///< Create an instance of concrete geometry handler for ObjComponent
BitmapGeometryHandler *
BitmapGeometryHandler::createInstance(IObjComponent *comp) {
  (void)comp;
  return new BitmapGeometryHandler();
}

///< Create an instance of concrete geometry handler for Object
BitmapGeometryHandler *
BitmapGeometryHandler::createInstance(boost::shared_ptr<Object> obj) {
  (void)obj;
  return new BitmapGeometryHandler();
}

///< Create an instance of concrete geometry handler for Object
GeometryHandler *BitmapGeometryHandler::createInstance(Object *obj) {
  (void)obj;
  return new BitmapGeometryHandler();
}

//----------------------------------------------------------------------------------------------
/** Triangulate the Object - this function will not be used.
 *
 */
void BitmapGeometryHandler::Triangulate() {
  // std::cout << "BitmapGeometryHandler::Triangulate() called\n";
}

//----------------------------------------------------------------------------------------------
///< Render Object or ObjComponent
void BitmapGeometryHandler::Render() {
  m_renderer.render(*m_rectDet);
}

//----------------------------------------------------------------------------------------------
///< Prepare/Initialize Object/ObjComponent to be rendered
void BitmapGeometryHandler::Initialize() {
  // std::cout << "BitmapGeometryHandler::Initialize() called\n";
}
}
}
