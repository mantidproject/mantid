#ifndef MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_
#define MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_

#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"

#include <QApplication>
#include <QObject>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif

namespace {
// We don't currently link QTest, so here we are just reimplementing the Qt
// sleep function so we can predict how long threads will run for.
void qSleep(int ms) {
#ifdef Q_OS_WIN
  Sleep(uint(ms));
#else
  struct timespec ts = {ms / 1000, (ms % 1000) * 1000 * 1000};
  nanosleep(&ts, nullptr);
#endif
}
} // namespace

namespace MantidQt {
namespace API {

/** FakeFindFilesThread
 *
 * This overrides the run method of a FindFilesWorker to implement
 * a dummy method that will just sleep for a given number of milliseconds.
 */
class EXPORT_OPT_MANTIDQT_COMMON FakeFindFilesThread : public FindFilesWorker {
  Q_OBJECT
public:
  FakeFindFilesThread(
      const FindFilesSearchParameters &parameters,
      const FindFilesSearchResults &results = FindFilesSearchResults(),
      int milliseconds = 100)
      : FindFilesWorker(parameters), m_results(results),
        m_milliseconds(milliseconds) {}

protected:
  void run() override {
    qSleep(m_milliseconds);
    QCoreApplication::processEvents();
    emit finished(m_results);
  }

private:
  FindFilesSearchResults m_results;
  int m_milliseconds;
};

/** FakeMWRunFiles
 *
 * This implements the slots required to listen to a FindFilesWorker
 * and will simply capture the result produced by the worker.
 *
 * It will also capture if the worker notified that the search was
 * finished.
 */
class EXPORT_OPT_MANTIDQT_COMMON FakeMWRunFiles : public QObject {
  Q_OBJECT

public:
  FakeMWRunFiles() : m_results(), m_finishedSignalRecieved(false) {
    connect(this, SIGNAL(fileFindingFinished()), this,
            SLOT(setSignalRecieved()));
  }

  /// Get the captured results of a file search
  FindFilesSearchResults getResults() { return m_results; };
  /// Get if the finished searching signal was recieved
  bool isFinishedSignalRecieved() { return m_finishedSignalRecieved; };

public slots:
  /// Slot called when file finding thread has finished.
  void inspectThreadResult(const FindFilesSearchResults &result) {
    m_results = result;
  }

  /// Slot called when the file finding thread has finished.
  void setSignalRecieved() { m_finishedSignalRecieved = true; }

signals:
  /// Signal emitted to itself when files were found
  void fileFindingFinished();

private:
  /// Results captured from the worker thread.
  FindFilesSearchResults m_results;
  /// Whether a finished signal was recieved
  bool m_finishedSignalRecieved;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_ */
