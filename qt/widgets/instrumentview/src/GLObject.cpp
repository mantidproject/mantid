#ifdef _WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidKernel/System.h"
#include "MantidQtWidgets/InstrumentView/GLObject.h"

namespace MantidQt {
namespace MantidWidgets {
int icount;

GLObject::GLObject(bool withDisplayList, const std::string &name)
    : mName(name), mChanged(true) {
  if (withDisplayList) {
    mDisplayListId = glGenLists(1);
  } else {
    mDisplayListId = 0;
  }
}
GLObject::~GLObject() {
  if (mDisplayListId != 0)
    glDeleteLists(mDisplayListId, 1);
}

/**
 * This method draws the opengl display list.
 */
void GLObject::draw() const {
  // std::cout << "GLObject::draw() for " << this->mName << "\n";
  if (mChanged)
    construct();
  if (mDisplayListId != 0) {
    // std::cerr << mDisplayListId << '\n';
    glCallList(mDisplayListId);
  } else {
    this->define();
  }
}

/**
 * This method constructs the opengl display list
 */
void GLObject::construct() const {
  if (mDisplayListId == 0) {
    mChanged = false;
    return;
  }
  init();
  glNewList(mDisplayListId,
            GL_COMPILE); // Construct display list for object representation
  this->define();
  glEndList();
  // if (dynamic_cast<ObjCompAssemblyActor*>(this))
  if (glGetError() == GL_OUT_OF_MEMORY) // Throw an exception
    throw Mantid::Kernel::Exception::OpenGLError("OpenGL: Out of video memory");
  mChanged = false; // Object Marked as changed.
}

/**
 * Virtual method which initializes the the Object before creating the display
 * list
 */
void GLObject::init() const {}

/**
 * Virtual method which constructs the opengl rendering commands.
 */
void GLObject::define() const {}

void GLObject::setName(const std::string &name) { mName = name; }
std::string GLObject::getName() const { return mName; }

} // namespace MantidWidgets
} // namespace MantidQt
