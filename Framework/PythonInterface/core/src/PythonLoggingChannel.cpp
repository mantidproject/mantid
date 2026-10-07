// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

// local includes
#include "MantidPythonInterface/core/PythonLoggingChannel.h"

// 3rd-party includes
#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WrapPython.h"
#include <Poco/Message.h>
#include <boost/python/errors.hpp>
#include <boost/python/import.hpp>
#include <boost/python/object.hpp>
#include <deque>
#include <limits>
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
constexpr size_t MAX_QUEUE_SIZE_DEFAULT = 10000;
const std::string MAX_QUEUE_SIZE_KEY("logging.python.queueSize");

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

/// The maximum number of queued messages from the properties file. A value of 0 removes the limit.
/// This must not be called from the constructor: the channel can be created by ConfigService::configureLogging
/// while ConfigService itself is still being constructed, and calling Instance() there would recurse.
size_t configuredMaxQueueSize() {
  auto &config = Mantid::Kernel::ConfigService::Instance();
  // check first because getString emits a log message for a missing key, which would schedule another drain
  if (!config.hasProperty(MAX_QUEUE_SIZE_KEY))
    return MAX_QUEUE_SIZE_DEFAULT;

  const auto value = config.getValue<int>(MAX_QUEUE_SIZE_KEY);
  if (!value || *value < 0)
    return MAX_QUEUE_SIZE_DEFAULT;
  if (*value == 0)
    return std::numeric_limits<size_t>::max();
  return static_cast<size_t>(*value);
}

} // namespace

struct PythonLoggingChannel::State {
  // The special behavior here is needed because Poco's LoggingFactory can be destroyed
  // after the Python interpreter was shut down.
  ~State() {
    if (Py_IsInitialized()) {
      Mantid::PythonInterface::GlobalInterpreterLock gil;
      // Destroy the object while the GIL is held.
      pyLogger = nullptr;
    } else {
      // The Python interpreter has been shut down and our logger object destroyed.
      // We can no longer safely call the destructor of *pyLogger,
      // so just deallocate the memory.
      operator delete(pyLogger.release());
    }
  }

  std::mutex mutex;
  std::deque<Message> queue;
  std::unique_ptr<boost::python::object> pyLogger;
  size_t maxQueueSize{MAX_QUEUE_SIZE_DEFAULT};
  size_t droppedMessages{0};
  bool callbackScheduled{false};
  bool draining{false};
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
  // Py_AddPendingCall cannot be used once the interpreter has been shut down
  if (!Py_IsInitialized())
    return;

  bool scheduleCallback{false};
  {
    std::lock_guard lock(m_state->mutex);
    if (m_state->closed)
      return;

    if (m_state->queue.size() < m_state->maxQueueSize)
      m_state->queue.emplace_back(msg);
    else
      ++m_state->droppedMessages;

    if (!m_state->callbackScheduled && !m_state->queue.empty()) {
      m_state->callbackScheduled = true;
      scheduleCallback = true;
    }
  }

  if (!scheduleCallback)
    return;

  // The pending call keeps the state alive in case it runs after this channel has been destroyed
  auto stateHolder = std::make_unique<std::shared_ptr<State>>(m_state);
  if (Py_AddPendingCall(&PythonLoggingChannel::pendingCallDrainQueue, stateHolder.get()) == 0) {
    stateHolder.release(); // ownership passed to pendingCallDrainQueue
  } else {
    std::lock_guard lock(m_state->mutex);
    m_state->callbackScheduled = false;
  }
}

int PythonLoggingChannel::pendingCallDrainQueue(void *stateHolder) {
  // Python runs pending calls on the main thread with the GIL held
  const std::unique_ptr<std::shared_ptr<State>> holder(static_cast<std::shared_ptr<State> *>(stateHolder));
  drainQueue(**holder);
  return 0;
}

void PythonLoggingChannel::drainQueue(State &state) {
  // Refresh the limit on the Python thread so changes to the properties take effect without recreating the channel.
  // Messages logged before the first drain use the default limit.
  const auto maxQueueSize = configuredMaxQueueSize();

  // The mutex is held asymmetrically on purpose. Producers (any C++ thread) hold it only to push a message, and this
  // consumer holds it only to pop one; the call into Python below happens with the mutex released. This means a C++
  // thread never waits on Python while logging, and a Python handler that logs back into Mantid re-enters enqueue()
  // without deadlocking. pyLogger is not protected by the mutex because it is only used while holding the GIL.
  //
  // Calling into Python can run a pending drain on this thread (e.g. inside flush()). The nested drain returns
  // immediately so that the outer loop delivers every message in order.
  {
    std::lock_guard lock(state.mutex);
    if (state.draining)
      return;
    state.draining = true;
  }

  while (true) {
    Message message;
    size_t droppedMessages{0};
    {
      std::lock_guard lock(state.mutex);
      state.maxQueueSize = maxQueueSize;
      if (state.queue.empty()) {
        state.callbackScheduled = false;
        state.draining = false;
        return;
      }

      message = std::move(state.queue.front());
      state.queue.pop_front();
      droppedMessages = state.droppedMessages;
      state.droppedMessages = 0;
    }

    try {
      const auto logFn = state.pyLogger->attr("log");
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
  if (!Py_IsInitialized())
    return;

  Mantid::PythonInterface::GlobalInterpreterLock gil;
  drainQueue(*m_state);
}

void PythonLoggingChannel::close() { closeImpl(); }

// m_state is never reset, so the C++ threads calling log() can read it while another thread closes the channel.
// Messages logged after closing are discarded by the closed flag.
void PythonLoggingChannel::closeImpl() {
  {
    std::lock_guard lock(m_state->mutex);
    m_state->closed = true;
  }
  flush();
}
} // namespace Poco
