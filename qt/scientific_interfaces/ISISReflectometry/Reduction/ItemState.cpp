// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ItemState.h"

namespace MantidQt {
namespace CustomInterfaces {

ItemState::ItemState() : m_state(State::NOT_STARTED), m_message(boost::none) {}

std::string ItemState::message() const {
  if (m_message)
    return m_message.get();
  return "";
}

void ItemState::setProgress(double progress, std::string const &message) {
  m_progress = progress;
  m_message = message;
}

void ItemState::setStarting() { m_state = State::STARTING; }

void ItemState::setRunning() { m_state = State::RUNNING; }

void ItemState::setSuccess() { m_state = State::SUCCESS; }

void ItemState::setWarning(std::string const &message) {
  m_state = State::WARNING;
  m_message = message;
}

void ItemState::setError(std::string const &message) {
  m_state = State::ERROR;
  m_message = message;
}
} // namespace CustomInterfaces
} // namespace MantidQt
