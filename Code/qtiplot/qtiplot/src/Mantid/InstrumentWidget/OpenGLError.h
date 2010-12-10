#ifndef OPENGLERROR_H_
#define OPENGLERROR_H_

#include <stdexcept>
#include <string>

/**
  * Exception for wrapping an OpenGL error
  */
class OpenGLError: public std::exception
{
  std::string m_msg;
public:
  explicit OpenGLError(const std::string& msg):m_msg(msg){}
  const char * what() const{return m_msg.c_str();}
  static void check(const std::string& funName);
};

#endif /*OPENGLERROR_H_*/

