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
auto importPythonLogger() {
  const auto logging = boost::python::import("logging");
  return logging.attr("getLogger")("Mantid");
}

int pythonLevel(const Message::Priority prio) {
  // TODO get values from python
  switch (prio) {
  case Message::Priority::PRIO_FATAL:
  case Message::Priority::PRIO_CRITICAL:
    return 50;
  case Message::Priority::PRIO_ERROR:
    return 40;
  case Message::Priority::PRIO_WARNING:
    return 30;
  case Message::Priority::PRIO_NOTICE:
  case Message::Priority::PRIO_INFORMATION:
    return 20;
  case Message::Priority::PRIO_DEBUG:
  case Message::Priority::PRIO_TRACE:
    return 10;
  }
  return 0;
}

} // namespace

PythonLoggingChannel::PythonLoggingChannel() : m_pyLogger(importPythonLogger()) {}

void PythonLoggingChannel::log(const Poco::Message &msg) {
  const auto logFn = m_pyLogger.attr("log");
  const auto numericLevel = pythonLevel(msg.getPriority());
  logFn(numericLevel, msg.getText());
}
} // namespace Poco
