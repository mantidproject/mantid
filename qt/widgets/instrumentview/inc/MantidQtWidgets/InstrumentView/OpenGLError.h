#ifndef OPENGLERROR_H_
#define OPENGLERROR_H_

#include <stdexcept>
#include <string>

namespace MantidQt {
namespace MantidWidgets {
/**
 * Exception for wrapping an OpenGL error
 */
class OpenGLError : public std::exception {
public:
  explicit OpenGLError(const std::string &msg) : m_msg(msg) {}
  const char *what() const noexcept override { return m_msg.c_str(); }
  static bool check(const std::string &funName);
  static bool hasError(const std::string &funName) { return check(funName); }
  static std::ostream &log();
  static std::ostream &logDebug();

private:
  std::string m_msg;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif /*OPENGLERROR_H_*/
