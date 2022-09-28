// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ItemState.h"

namespace MantidQt::CustomInterfaces::ISISReflectometry {

ItemState::ItemState() : m_state(State::ITEM_NOT_STARTED), m_message(boost::none), m_progress(0.0) {}

ItemState::ItemState(State state) : m_state(state), m_message(boost::none), m_progress(0.0) {}

State ItemState::state() const { return m_state; }

std::string ItemState::message() const {
  if (m_message)
    return m_message.get();
  return "";
}

double ItemState::progress() const { return m_progress; }

void ItemState::setProgress(double progress, std::string const &message) {
  m_progress = progress;
  m_message = message;
}

void ItemState::setStarting() { m_state = State::ITEM_STARTING; }

void ItemState::setRunning() { m_state = State::ITEM_RUNNING; }

void ItemState::setSuccess() { m_state = State::ITEM_SUCCESS; }

void ItemState::setWarning(std::string const &message) {
  m_state = State::ITEM_WARNING;
  m_message = message;
}

void ItemState::setError(std::string const &message) {
  m_state = State::ITEM_ERROR;
  m_message = message;
}

void ItemState::setChildrenSuccess() { m_state = State::ITEM_CHILDREN_SUCCESS; }

void ItemState::reset() {
  m_state = State::ITEM_NOT_STARTED;
  m_message = std::string();
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
