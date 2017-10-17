#ifndef MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_
#define MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_

#include "FindFilesThreadPoolManager.h"

#include <QObject>

namespace MantidQt {
namespace API {

class FakeMWRunFiles : public QObject {
Q_OBJECT
public slots:
  /// Slot called when file finding thread has finished.
  void inspectThreadResult(const FindFilesSearchResults& result) {
    m_results = result;
  }

  /// Slot called when the file finding thread has finished.
  void setSignalRecieved() {
    m_finishedSignalRecieved = true;
  }

signals:
  void fileFindingFinished();

public:
  FakeMWRunFiles() : m_results(), m_finishedSignalRecieved(false) {
    connect(this, SIGNAL(fileFindingFinished()),
            this, SLOT(setSignalRecieved()),
            Qt::DirectConnection);
  }

  FindFilesSearchResults getResults() { return m_results; };
  bool isFinishedSignalRecieved() { return m_finishedSignalRecieved; };

private:
  FindFilesSearchResults m_results;
  bool m_finishedSignalRecieved;
};

}
}

#endif /* MANTIDQT_API_FINDFILESTHREADPOOLMANAGERTESTMOCKOBJECTS_H_ */
