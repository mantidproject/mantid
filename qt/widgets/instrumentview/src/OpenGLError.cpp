// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/OpenGLError.h"
#include "MantidGeometry/Rendering/OpenGL_Headers.h"
#include "MantidKernel/Logger.h"
#include <sstream>

namespace {
// Initialize logger
Mantid::Kernel::Logger g_log("OpenGL");
} // namespace

namespace MantidQt::MantidWidgets {
/**
 * Check for a GL error and throw OpenGLError if found
 * @param funName :: Name of the function where checkGLError is called.
 *   The message returned be what() method of the exception has the form:
 *   "OpenGL error detected in " + funName + ": " + error_description
 */
bool OpenGLError::check(const std::string &funName) {
  GLuint err = glGetError();
  if (err) {
    std::ostringstream ostr;
    ostr << "OpenGL error detected in " << funName << ": " << gluErrorString(err);
    g_log.error() << ostr.str() << '\n';
    // throw OpenGLError(ostr.str());
    return true;
  }
  return false;
}

std::string OpenGLError::openGlVersion() {
  const unsigned char *verString = glGetString(GL_VERSION);
  return std::string(reinterpret_cast<const char *>(verString));
}

std::ostream &OpenGLError::log() { return g_log.error(); }

std::ostream &OpenGLError::logDebug() { return g_log.debug(); }

} // namespace MantidQt::MantidWidgets
