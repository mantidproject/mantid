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
void StructuredGeometryHandler::Render() {
  V3D pos;

  // Wait for no error
  while (glGetError() != GL_NO_ERROR)
    ;

  auto xVerts = m_Det->getXValues();
  auto yVerts = m_Det->getYValues();
  auto r = m_Det->getR();
  auto g = m_Det->getG();
  auto b = m_Det->getB();

  if (xVerts.size() != yVerts.size())
    return;

  auto w = m_Det->xPixels() + 1;
  auto h = m_Det->yPixels() + 1;

  glBegin(GL_QUADS);

  for (size_t iy = 0; iy < h - 1; iy++) {
    for (size_t ix = 0; ix < w - 1; ix++) {

      glColor3ub((GLubyte)r[(iy * (w - 1)) + ix],
                 (GLubyte)g[(iy * (w - 1)) + ix],
                 (GLubyte)b[(iy * (w - 1)) + ix]);

      pos = V3D(xVerts[(iy * w) + ix + w], yVerts[(iy * w) + ix + w], 0.0);
      glVertex3f(static_cast<GLfloat>(pos.X()), static_cast<GLfloat>(pos.Y()),
                 static_cast<GLfloat>(pos.Z()));
      pos = V3D(xVerts[(iy * w) + ix + w + 1], yVerts[(iy * w) + ix + w + 1],
                0.0);
      glVertex3f(static_cast<GLfloat>(pos.X()), static_cast<GLfloat>(pos.Y()),
                 static_cast<GLfloat>(pos.Z()));
      pos = V3D(xVerts[(iy * w) + ix + 1], yVerts[(iy * w) + ix + 1], 0.0);
      glVertex3f(static_cast<GLfloat>(pos.X()), static_cast<GLfloat>(pos.Y()),
                 static_cast<GLfloat>(pos.Z()));
      pos = V3D(xVerts[(iy * w) + ix], yVerts[(iy * w) + ix], 0.0);
      glVertex3f(static_cast<GLfloat>(pos.X()), static_cast<GLfloat>(pos.Y()),
                 static_cast<GLfloat>(pos.Z()));
    }
  }

  glEnd();

  if (glGetError() > 0)
    std::cout << "OpenGL error in StructuredGeometryHandler::Render \n";

  glDisable(
      GL_TEXTURE_2D); // stop texture mapping - not sure if this is necessary.
}

//----------------------------------------------------------------------------------------------
///< Prepare/Initialize Object/ObjComponent to be rendered
void StructuredGeometryHandler::Initialize() {
  // do nothing
}
}
}
