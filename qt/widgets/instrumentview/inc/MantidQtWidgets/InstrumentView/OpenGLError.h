// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <ostream>
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
  static std::string openGlVersion();

private:
  std::string m_msg;
};
} // namespace MantidWidgets
} // namespace MantidQt
