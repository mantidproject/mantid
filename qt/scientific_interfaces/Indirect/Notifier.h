// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_NOTIFIER_H_
#define MANTIDQTCUSTOMINTERFACES_NOTIFIER_H_

template <class T> class Notifier {
public:
  void notify(const std::function<void(T &)> &fun) {
    for (auto &subscriber : m_subscribers) {
      fun(*subscriber);
    }
  };
  void subscribe(T *observer) { m_subscribers.emplace_back(observer); };

private:
  std::vector<T *> m_subscribers;
};

#endif
