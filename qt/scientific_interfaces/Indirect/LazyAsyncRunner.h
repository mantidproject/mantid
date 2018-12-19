// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQTCUSTOMINTERFACES_LAZYASYNCRUNNER_H_
#define MANTIDQTCUSTOMINTERFACES_LAZYASYNCRUNNER_H_

#include "DllConfig.h"

#include <QtCore>

#include <boost/optional.hpp>

/**
  A lazy asynchronous runner; forgets all but the most recent callback.
*/
namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class MANTIDQT_INDIRECT_DLL QtLazyAsyncRunnerBase : public QObject {
  Q_OBJECT

signals:
  void finished();
  void finishedLazy();

protected slots:
  void currentFinishedBase() { currentFinished(); }

protected:
  virtual void currentFinished() = 0;
};

template <typename Callback>
class MANTIDQT_INDIRECT_DLL QtLazyAsyncRunner : public QtLazyAsyncRunnerBase {
public:
  using ReturnType = typename std::result_of<Callback()>::type;

  explicit QtLazyAsyncRunner()
      : m_current(), m_next(boost::none), m_initialized(false) {
    connect(&m_current, SIGNAL(finished()), this, SLOT(currentFinishedBase()));
  }

  void addCallback(Callback &&callback) {
    if (m_next.is_initialized())
      m_next = boost::none;

    if (m_current.isFinished() || !m_initialized)
      m_current.setFuture(QtConcurrent::run(callback));
    else
      m_next = std::forward<Callback>(callback);
    m_initialized = true;
  }

  bool isFinished() const { return m_current.isFinished(); }

  ReturnType result() const { return m_current.result(); }

protected:
  void currentFinished() override {
    if (m_next.is_initialized()) {
      m_current.setFuture(QtConcurrent::run(*m_next));
      m_next = boost::none;
      emit finished();
    } else
      emit finishedLazy();
  }

private:
  QFutureWatcher<ReturnType> m_current;
  boost::optional<Callback> m_next;
  bool m_initialized;
};

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt

#endif /* MANTIDQTCUSTOMINTERFACES_INDIRECTFITDATAPRESENTER_H_ */
