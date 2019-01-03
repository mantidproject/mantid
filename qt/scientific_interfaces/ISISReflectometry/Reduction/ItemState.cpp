// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ItemState.h"

namespace MantidQt {
namespace CustomInterfaces {

ItemState::ItemState()
    : m_state(State::NOT_STARTED), m_message(boost::none), m_progress(0.0),
      m_mutex() {}

ItemState::ItemState(ItemState const &rhs)
    : m_state(rhs.state()), m_message(rhs.message()),
      m_progress(rhs.progress()), m_mutex() {}

ItemState &ItemState::operator=(ItemState const &rhs) {
  m_state = rhs.state();
  m_message = rhs.message();
  m_progress = rhs.progress();
  return *this;
}

State ItemState::state() const { return m_state; }

std::string ItemState::message() const {
  // std::unique_lock<std::mutex> lock(m_mutex);
  if (m_message)
    return m_message.get();
  return "";
}

double ItemState::progress() const { return m_progress; }

void ItemState::setProgress(double progress, std::string const &message) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_progress = progress;
  m_message = message;
}

void ItemState::setStarting() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_state = State::STARTING;
}

void ItemState::setRunning() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_state = State::RUNNING;
}

void ItemState::setSuccess() {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_state = State::SUCCESS;
}

void ItemState::setWarning(std::string const &message) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_state = State::WARNING;
  m_message = message;
}

void ItemState::setError(std::string const &message) {
  std::unique_lock<std::mutex> lock(m_mutex);
  m_state = State::ERROR;
  m_message = message;
}
} // namespace CustomInterfaces
} // namespace MantidQt
