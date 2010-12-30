#include "OpenGLError.h"

#ifdef WIN32
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <iostream>
#include <sstream>

Mantid::Kernel::Logger& OpenGLError::s_log(Mantid::Kernel::Logger::get("OpenGL"));

/**
  * Check for a GL error and throw OpenGLError if found
  * @param funName Name of the function where checkGLError is called.
  *   The message returned be what() method of the exception has the form:
  *   "OpenGL error detected in " + funName + ": " + error_description
  */
bool OpenGLError::check(const std::string& funName)
{
   GLuint err=glGetError();
   if (err)
   {
     std::ostringstream ostr;
     ostr<<"OpenGL error detected in " << funName <<": " << gluErrorString(err);
     s_log.error()<<ostr.str()<<'\n';
     //throw OpenGLError(ostr.str());
     return true;
   }
   return false;
}

std::ostream& OpenGLError::log()
{
  return s_log.error();
}
