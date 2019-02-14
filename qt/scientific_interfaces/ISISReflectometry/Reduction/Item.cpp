// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "Item.h"

namespace MantidQt {
namespace CustomInterfaces {

Item::Item() : m_itemState() {}

Item::Item(Item const &rhs) : m_itemState() { UNUSED_ARG(rhs); }

Item &Item::operator=(Item const &rhs) {
  UNUSED_ARG(rhs);
  m_itemState = ItemState();
  return *this;
}

State Item::state() const { return m_itemState.state(); }

std::string Item::message() const { return m_itemState.message(); }

void Item::resetState() { m_itemState.reset(); }

void Item::setProgress(double p, std::string const &msg) {
  m_itemState.setProgress(p, msg);
}

void Item::setStarting() { m_itemState.setStarting(); }

void Item::setRunning() { m_itemState.setRunning(); }

void Item::setSuccess() { m_itemState.setSuccess(); }

void Item::setError(std::string const &msg) { m_itemState.setError(msg); }

bool Item::requiresProcessing(bool reprocessFailed) const {
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

void Item::algorithmStarted(Mantid::API::IAlgorithm_sptr const algorithm) {
  UNUSED_ARG(algorithm);
  setRunning();
}

void Item::algorithmComplete(Mantid::API::IAlgorithm_sptr const algorithm) {
  UNUSED_ARG(algorithm);
  setSuccess();
}

void Item::algorithmError(Mantid::API::IAlgorithm_sptr const algorithm,
                          std::string const &msg) {
  UNUSED_ARG(algorithm);
  setError(msg);
}
} // namespace CustomInterfaces
} // namespace MantidQt
