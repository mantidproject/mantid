// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_CUSTOMINTERFACES_OBSERVERPATTERN_H_
#define MANTIDQT_CUSTOMINTERFACES_OBSERVERPATTERN_H_
#include <functional>
#include <set>
#include <string>

class observable;

class observer {
public:
  virtual void update() = 0;
  virtual void update(std::string arg) = 0;
};

class observable {
  std::set<observer *> m_observers;

public:
  void attach(observer *listener) { m_observers.insert(listener); };
  void detach(observer *listener) { m_observers.erase(listener); };
  void notify() {
    for (auto &listener : m_observers) {
      listener->update();
    }
  };
  void notify(std ::string arg) {
    for (auto &listener : m_observers) {
      listener->update(arg);
    }
  };
};

class loadObserver : public observer {
public:
  loadObserver() : m_slot(nullptr){};
  ~loadObserver(){};
  void setSlot(std::function<void()> &func) { m_slot = func; };
  void update() override { m_slot(); };
  void update(std::string) { m_slot(); };

private:
  std::function<void()> m_slot;
};

class generalObserver : public observer {
public:
  generalObserver() : m_slot(nullptr){};
  ~generalObserver(){};
  void setSlot(std::function<void(std::string)> &func) { m_slot = func; };
  void update(std ::string arg) override { m_slot(arg); };
  void update() override { m_slot(""); };

private:
  std::function<void(std::string)> m_slot;
};

#endif /* MANTIDQT_CUSTOMINTERFACES_OBSERVERPATTERN_H_ */
