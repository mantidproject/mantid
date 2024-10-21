// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include <functional>
#include <set>
#include <string>

/**
 * Simple classes for observer and observable pattern.
 * These can be used to replace signals and slots for mocking
 **/

class Observable;

class Observer {
public:
  virtual ~Observer() {};
  virtual void update() = 0;
};

/**
 * Simple observable class. This is used
 * to signify if a change has been made and then needs to
 * notify its observers.
 **/
class Observable {
  std::set<Observer *> m_observers;

public:
  /**
   * @param listener :: want to be notified when this observer changes
   **/
  void attach(Observer *listener) { m_observers.insert(listener); };
  /**
   * @param listener :: no longer want to be notified when this observer changes
   **/
  void detach(Observer *listener) { m_observers.erase(listener); };
  /**
   * Update all of the observers that a change has been made
   **/
  void notify() {
    for (auto &listener : m_observers) {
      listener->update();
    }
  };
};

/**
 * Simple observer class (for void functions/slots). This is used
 * to update when a change has been made on an observerable.
 **/
class VoidObserver : public Observer {
public:
  VoidObserver() : m_slot(nullptr) {};
  ~VoidObserver() {};
  /**
   * Sets the function/slot for the oberver
   * @param func:: the void function we want to call when the observer
   * sends a notify signal
   **/
  void setSlot(const std::function<void()> &func) { m_slot = func; };
  /**
   * Calls the function/slot
   **/
  void update() override { m_slot(); };

private:
  std::function<void()> m_slot;
};
