#ifndef MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_
#define MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_

#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DllOption.h"
#include "MantidQtWidgets/Common/FindFilesThreadPoolManager.h"

#include <QObject>
#include <QApplication>

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
  nanosleep(&ts, NULL);
#endif
}
} // namespace

namespace MantidQt {
namespace API {
	namespace {
		using Mantid::Kernel::make_unique;
		std::unique_ptr<QApplication> createApplication() {
			int argc = 1;
			char *argv = "DummyTestingApplication";
			return make_unique<QApplication>(argc, &argv);
		}
	}
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

class EXPORT_OPT_MANTIDQT_COMMON FakeMWRunFiles : public QObject {
  Q_OBJECT
public slots:
  /// Slot called when file finding thread has finished.
  void inspectThreadResult(const FindFilesSearchResults& result) {
    m_results = result;
  }

  /// Slot called when the file finding thread has finished.
  void setSignalRecieved() { m_finishedSignalRecieved = true; }

signals:
  void fileFindingFinished();

public:
  FakeMWRunFiles() : m_results(), m_finishedSignalRecieved(false) {
	  qRegisterMetaType<FindFilesSearchParameters>();
	  qRegisterMetaType<FindFilesSearchResults>();
    connect(this, SIGNAL(fileFindingFinished()), this,
            SLOT(setSignalRecieved()));
  }

  FindFilesSearchResults getResults() { return m_results; };
  bool isFinishedSignalRecieved() { return m_finishedSignalRecieved; };

private:
  FindFilesSearchResults m_results;
  bool m_finishedSignalRecieved;
};

} // namespace API
} // namespace MantidQt

#endif /* MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_ */
