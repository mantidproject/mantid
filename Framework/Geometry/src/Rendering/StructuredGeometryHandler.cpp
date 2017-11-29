#include "MantidGeometry/Instrument/StructuredDetector.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidGeometry/Rendering/StructuredGeometryHandler.h"
#include <climits>
#include <iostream>

#include <boost/make_shared.hpp>

namespace Mantid {
namespace Geometry {
using Kernel::V3D;

/**
 * @return A shared_ptr to a new copy of this object
 */
boost::shared_ptr<GeometryHandler> StructuredGeometryHandler::clone() const {
  return boost::make_shared<StructuredGeometryHandler>(*this);
}

/// Parameter constructor
StructuredGeometryHandler::StructuredGeometryHandler(StructuredDetector *comp)
    : GeometryHandler(dynamic_cast<IObjComponent *>(comp)) {
  // Save the structured detector link for later.
  m_Det = comp;
}

StructuredGeometryHandler::StructuredGeometryHandler()
    : GeometryHandler(static_cast<Object *>(nullptr)), m_Det(nullptr) {}

///< Create an instance of concrete geometry handler for ObjComponent
StructuredGeometryHandler *
StructuredGeometryHandler::createInstance(IObjComponent *) {
  return new StructuredGeometryHandler();
}

///< Create an instance of concrete geometry handler for Object
StructuredGeometryHandler *
    StructuredGeometryHandler::createInstance(boost::shared_ptr<Object>) {
  return new StructuredGeometryHandler();
}

///< Create an instance of concrete geometry handler for Object
GeometryHandler *StructuredGeometryHandler::createInstance(Object *) {
  return new StructuredGeometryHandler();
}

//----------------------------------------------------------------------------------------------
/** Triangulate the Object - this function will not be used.
 *
 */
void StructuredGeometryHandler::Triangulate() {
  // do nothing
}

//----------------------------------------------------------------------------------------------
///< Draw pixels according to StructuredDetector vertices
void StructuredGeometryHandler::Render() { m_renderer.render(*m_Det); }

//----------------------------------------------------------------------------------------------
///< Prepare/Initialize Object/ObjComponent to be rendered
void StructuredGeometryHandler::Initialize() {
  // do nothing
}
}
}
