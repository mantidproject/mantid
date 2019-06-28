// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Item.h"

namespace MantidQt {
namespace CustomInterfaces {

Item::Item() : m_itemState(), m_skipped(false) {}

State Item::state() const { return m_itemState.state(); }

std::string Item::message() const { return m_itemState.message(); }

void Item::resetState(bool resetChildren) {
  UNUSED_ARG(resetChildren);
  resetOutputs();
  m_itemState.reset();
}

void Item::setSkipped(bool skipped) { m_skipped = skipped; }

bool Item::success() const {
  return m_itemState.state() == State::ITEM_COMPLETE;
}

bool Item::complete() const {
  return m_itemState.state() == State::ITEM_COMPLETE ||
         m_itemState.state() == State::ITEM_ERROR ||
         m_itemState.state() == State::ITEM_WARNING;
}

void Item::setProgress(double p, std::string const &msg) {
  m_itemState.setProgress(p, msg);
}

void Item::setStarting() { m_itemState.setStarting(); }

void Item::setRunning() { m_itemState.setRunning(); }

void Item::setSuccess() { m_itemState.setSuccess(); }

void Item::setError(std::string const &msg) { m_itemState.setError(msg); }

bool Item::requiresProcessing(bool reprocessFailed) const {
  // Check the skipped flag. This means that items will not be processed even
  // if we're reprocessing failed rows
  if (m_skipped)
    return false;

  switch (state()) {
  case State::ITEM_NOT_STARTED:
    return true;
  case State::ITEM_STARTING:
  case State::ITEM_RUNNING:  // fall through
  case State::ITEM_COMPLETE: // fall through
  case State::ITEM_WARNING:  // fall through
    return false;
  case State::ITEM_ERROR:
    return reprocessFailed;
    // Don't include default so that the compiler warns if a value is not
    // handled
  }
  return false;
}
} // namespace CustomInterfaces
} // namespace MantidQt
