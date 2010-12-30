#ifndef OPENGLERROR_H_
#define OPENGLERROR_H_

#include "MantidKernel/Logger.h"

#include <stdexcept>
#include <string>

/**
  * Exception for wrapping an OpenGL error
  */
class OpenGLError: public std::exception
{
public:
  explicit OpenGLError(const std::string& msg):m_msg(msg){}
  ~OpenGLError() throw(){}
  const char * what() const throw() {return m_msg.c_str();}
  static bool check(const std::string& funName);
  static bool hasError(const std::string& funName){return check(funName);}
  static std::ostream& log();
private:
  std::string m_msg;
  static Mantid::Kernel::Logger& s_log;
};

#endif /*OPENGLERROR_H_*/

