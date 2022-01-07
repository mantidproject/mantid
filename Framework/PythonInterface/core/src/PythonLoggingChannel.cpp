// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// local includes
#include "MantidPythonInterface/core/PythonLoggingChannel.h"

// 3rd-party includes
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include <Poco/Message.h>
#include <boost/python/import.hpp>

namespace Poco {

namespace {
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

PythonLoggingChannel::PythonLoggingChannel() {
  Mantid::PythonInterface::GlobalInterpreterLock gil;
  auto logger = (boost::python::import("logging").attr("getLogger")("Mantid"));
  m_pyLogger = std::make_unique<boost::python::object>(std::move(logger));
}

// The special behavior here is needed because Poco's LoggingFactory can be destroyed
// after the Python interpreter was shut down.
PythonLoggingChannel::~PythonLoggingChannel() {
  if (Py_IsInitialized()) {
    Mantid::PythonInterface::GlobalInterpreterLock gil;
    // Destroy the object while the GIL is held.
    m_pyLogger = nullptr;
  } else {
    // The Python interpreter has been shut down and our logger object destroyed.
    // We can no longer safely call the destructor of *m_pLogger,
    // so just deallocate the memory.
    operator delete(m_pyLogger.release());
  }
}

void PythonLoggingChannel::log(const Poco::Message &msg) {
  if (m_pyLogger && Py_IsInitialized()) {
    Mantid::PythonInterface::GlobalInterpreterLock gil;
    const auto logFn = m_pyLogger->attr("log");
    const auto numericLevel = static_cast<int>(pythonLevel(msg.getPriority()));
    logFn(numericLevel, msg.getText());
  }
}
} // namespace Poco
