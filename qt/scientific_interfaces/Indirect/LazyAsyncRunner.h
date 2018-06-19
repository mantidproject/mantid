#ifndef MANTIDQTCUSTOMINTERFACES_LAZYASYNCRUNNER_H_
#define MANTIDQTCUSTOMINTERFACES_LAZYASYNCRUNNER_H_

#include <QtCore>

#include <boost/optional.hpp>

/**
  A lazy asynchronous runner; forgets all but the most recent callback.
  Copyright &copy; 2015-2016 ISIS Rutherford Appleton Laboratory, NScD
  Oak Ridge National Laboratory & European Spallation Source
  This file is part of Mantid.
  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.
  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {

class DLLExport QtLazyAsyncRunnerBase : public QObject {
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
class DLLExport QtLazyAsyncRunner : public QtLazyAsyncRunnerBase {
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
