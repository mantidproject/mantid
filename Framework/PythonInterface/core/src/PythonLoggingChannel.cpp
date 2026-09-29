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
#include "MantidPythonInterface/core/WrapPython.h"
#include <Poco/Message.h>
#include <boost/python/errors.hpp>
#include <boost/python/import.hpp>
#include <boost/python/object.hpp>
#include <deque>
#include <mutex>
#include <sstream>

namespace Poco {

namespace {
// See https://docs.python.org/3/library/logging.html#logging-levels
constexpr int PY_CRITICAL = 50;
constexpr int PY_ERROR = 40;
constexpr int PY_WARNING = 30;
constexpr int PY_INFO = 20;
constexpr int PY_DEBUG = 10;
constexpr int PY_NOTSET = 0;
constexpr size_t MAX_QUEUE_SIZE = 10000;

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

struct PythonLoggingChannel::State {
  ~State() {
    if (!pyLogger)
      return;

    if (Py_IsInitialized()) {
      Mantid::PythonInterface::GlobalInterpreterLock gil;
      pyLogger = nullptr;
    } else {
      operator delete(pyLogger.release());
    }
  }

  std::mutex mutex;
  std::deque<Message> queue;
  std::unique_ptr<boost::python::object> pyLogger;
  size_t droppedMessages{0};
  bool callbackScheduled{false};
  bool closed{false};
};

PythonLoggingChannel::PythonLoggingChannel() : m_state(std::make_shared<State>()) {
  Mantid::PythonInterface::GlobalInterpreterLock gil;
  auto logger = (boost::python::import("logging").attr("getLogger")("Mantid"));
  m_state->pyLogger = std::make_unique<boost::python::object>(std::move(logger));
}

PythonLoggingChannel::~PythonLoggingChannel() { closeImpl(); }

void PythonLoggingChannel::log(const Poco::Message &msg) { enqueue(msg); }

void PythonLoggingChannel::log(Poco::Message &&msg) { enqueue(std::move(msg)); }

void PythonLoggingChannel::enqueue(Poco::Message msg) {
  const auto state = m_state;
  if (!state || !Py_IsInitialized())
    return;

  bool scheduleCallback{false};
  {
    std::lock_guard lock(state->mutex);
    if (state->closed)
      return;

    if (state->queue.size() < MAX_QUEUE_SIZE)
      state->queue.emplace_back(std::move(msg));
    else
      ++state->droppedMessages;

    if (!state->callbackScheduled && !state->queue.empty()) {
      state->callbackScheduled = true;
      scheduleCallback = true;
    }
  }

  if (!scheduleCallback)
    return;

  auto *stateHolder = new std::shared_ptr<State>(state);
  if (Py_AddPendingCall(&PythonLoggingChannel::drainQueue, stateHolder) != 0) {
    delete stateHolder;
    std::lock_guard lock(state->mutex);
    state->callbackScheduled = false;
  }
}

int PythonLoggingChannel::drainQueue(void *statePtr) {
  std::unique_ptr<std::shared_ptr<State>> stateHolder(static_cast<std::shared_ptr<State> *>(statePtr));
  drainQueue(*stateHolder);
  return 0;
}

void PythonLoggingChannel::drainQueue(const std::shared_ptr<State> &state) {
  if (!Py_IsInitialized())
    return;

  while (true) {
    Message message;
    size_t droppedMessages{0};
    {
      std::lock_guard lock(state->mutex);
      if (state->queue.empty()) {
        state->callbackScheduled = false;
        return;
      }

      message = std::move(state->queue.front());
      state->queue.pop_front();
      droppedMessages = state->droppedMessages;
      state->droppedMessages = 0;
    }

    try {
      const auto logFn = state->pyLogger->attr("log");
      if (droppedMessages > 0) {
        std::ostringstream warning;
        warning << "PythonLoggingChannel dropped " << droppedMessages << " messages because its queue was full";
        logFn(PY_WARNING, warning.str());
      }
      logFn(pythonLevel(message.getPriority()), message.getText());
    } catch (boost::python::error_already_set &) {
      PyErr_Print();
    }
  }
}

void PythonLoggingChannel::flush() {
  const auto state = m_state;
  if (!state || !Py_IsInitialized())
    return;

  Mantid::PythonInterface::GlobalInterpreterLock gil;
  drainQueue(state);
}

void PythonLoggingChannel::close() { closeImpl(); }

void PythonLoggingChannel::closeImpl() {
  const auto state = m_state;
  if (!state)
    return;

  {
    std::lock_guard lock(state->mutex);
    state->closed = true;
  }
  flush();
  m_state.reset();
}
} // namespace Poco
