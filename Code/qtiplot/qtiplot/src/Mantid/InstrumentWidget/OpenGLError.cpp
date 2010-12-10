#include "OpenGLError.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <gl/gl.h>
#include <gl/glu.h>
#include <iostream>
#include <sstream>

/**
  * Check for a GL error and throw OpenGLError if found
  * @param funName Name of the function where checkGLError is called.
  *   The message returned be what() method of the exception has the form:
  *   "OpenGL error detected in " + funName + ": " + error_description
  */
void OpenGLError::check(const std::string& funName)
{
   GLuint err=glGetError();
   if (err)
   {
     std::ostringstream ostr;
     ostr<<"OpenGL error detected in " << funName <<": " << gluErrorString(err);
     std::cerr<<ostr.str()<<'\n';
     throw OpenGLError(ostr.str());
   }
}
