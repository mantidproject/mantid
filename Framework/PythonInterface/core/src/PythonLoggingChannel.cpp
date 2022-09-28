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
constexpr int PY_CRITICAL = 50;
constexpr int PY_ERROR = 40;
constexpr int PY_WARNING = 30;
constexpr int PY_INFO = 20;
constexpr int PY_DEBUG = 10;
constexpr int PY_NOTSET = 0;

auto pythonLevel(const Message::Priority prio) {
  switch (prio) {
  case Message::Priority::PRIO_FATAL:
  case Message::Priority::PRIO_CRITICAL:
    return PY_CRITICAL;
  case Message::Priority::PRIO_ERROR:
    return PY_ERROR;
  case Message::Priority::PRIO_WARNING:
    return PY_WARNING;
  case Message::Priority::PRIO_NOTICE:
  case Message::Priority::PRIO_INFORMATION:
    return PY_INFO;
  case Message::Priority::PRIO_DEBUG:
  case Message::Priority::PRIO_TRACE:
    return PY_DEBUG;
  default:
    return PY_NOTSET;
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
    const auto numericLevel = pythonLevel(msg.getPriority());
    logFn(numericLevel, msg.getText());
  }
}
} // namespace Poco
