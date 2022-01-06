// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// local includes
#include "MantidPythonInterface/core/PythonLoggingChannel.h"

// 3rd-party includes
#include <Poco/Message.h>
#include <boost/python/import.hpp>

namespace Poco {

// TODO gil ?

namespace {
auto importLogger() {
  const auto logging = boost::python::import("logging");
  return logging.attr("getLogger")("Mantid");
}

// See https://docs.python.org/3/library/logging.html#logging-levels
enum class PyLogLevel : int {
  CRITICAL = 50,
  ERROR = 40,
  WARNING = 30,
  INFO = 20,
  DEBUG = 10,
  NOTSET = 0,
};

PyLogLevel pythonLevel(const Message::Priority prio) {
  switch (prio) {
  case Message::Priority::PRIO_FATAL:
  case Message::Priority::PRIO_CRITICAL:
    return PyLogLevel::CRITICAL;
  case Message::Priority::PRIO_ERROR:
    return PyLogLevel::ERROR;
  case Message::Priority::PRIO_WARNING:
    return PyLogLevel::WARNING;
  case Message::Priority::PRIO_NOTICE:
  case Message::Priority::PRIO_INFORMATION:
    return PyLogLevel::INFO;
  case Message::Priority::PRIO_DEBUG:
  case Message::Priority::PRIO_TRACE:
    return PyLogLevel::DEBUG;
  default:
    return PyLogLevel::NOTSET;
  }
}

} // namespace

PythonLoggingChannel::PythonLoggingChannel() : m_pyLogger(importLogger()) {}

void PythonLoggingChannel::log(const Poco::Message &msg) {
  const auto logFn = m_pyLogger.attr("log");
  const auto numericLevel = static_cast<int>(pythonLevel(msg.getPriority()));
  logFn(numericLevel, msg.getText());
}
} // namespace Poco
