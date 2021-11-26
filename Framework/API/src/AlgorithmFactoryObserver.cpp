// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmFactoryObserver.h"

namespace {
template <typename Observer> void modifyObserver(const bool turnOn, bool &isObserving, Observer &observer) {
  if (turnOn && !isObserving) {
    Mantid::API::AlgorithmFactory::Instance().notificationCenter.addObserver(observer);
  } else if (!turnOn && isObserving) {
    Mantid::API::AlgorithmFactory::Instance().notificationCenter.removeObserver(observer);
  }
  isObserving = turnOn;
}
} // namespace

namespace Mantid::API {

AlgorithmFactoryObserver::AlgorithmFactoryObserver()
    : m_updateObserver(*this, &AlgorithmFactoryObserver::_updateHandle) {}

AlgorithmFactoryObserver::~AlgorithmFactoryObserver() {
  // Turn off/remove all observers
  this->observeUpdate(false);
}

// ------------------------------------------------------------
// Observe Methods
// ------------------------------------------------------------

/**
 * @brief Function will add/remove the observer to the AlgorithmFactory when
 * something is subscribed to it.
 *
 * @param turnOn bool; if this is True then, if not already present, the
 * observer will be added else removed if it's false.
 */
void AlgorithmFactoryObserver::observeUpdate(bool turnOn) {
  modifyObserver(turnOn, m_observingUpdate, m_updateObserver);
}

// ------------------------------------------------------------
// Virtual Methods
// ------------------------------------------------------------

/**
 * @brief If something subscribes to the AlgorithmFactory,
 * then this function will trigger.
 * works by overloading this class and overriding this function.
 *
 */
void AlgorithmFactoryObserver::updateHandle() {}

// ------------------------------------------------------------
// Private Methods
// ------------------------------------------------------------
void AlgorithmFactoryObserver::_updateHandle(AlgorithmFactoryUpdateNotification_ptr pNf) {
  UNUSED_ARG(pNf)
  this->updateHandle();
}

} // namespace Mantid::API
