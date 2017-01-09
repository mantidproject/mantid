#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYTHREAD_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYTHREAD_H_

// included for UNUSED_ARG
#include "MantidKernel/System.h"
#include "MantidQtCustomInterfaces/Tomography/TomographyProcess.h"
#include <QString>
#include <QThread>

namespace MantidQt {
namespace CustomInterfaces {
/*
TomographyThread class that can handle a single worker, and get all the standard
output and standard error content from the process.

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
class TomographyThread : public QThread {
  Q_OBJECT
public:
  TomographyThread(QObject *parent, TomographyProcess *worker)
      : QThread(parent), m_worker(worker) {
    // interactions between the thread and the worker are defined here
    connect(this, SIGNAL(started()), worker, SLOT(startWorker()));
    connect(this, SIGNAL(started()), this, SLOT(startWorker()));

    connect(worker, SIGNAL(readyReadStandardOutput()), this,
            SLOT(readWorkerStdOut()));
    connect(worker, SIGNAL(readyReadStandardError()), this,
            SLOT(readWorkerStdErr()));

    connect(worker, SIGNAL(finished(int)), this, SLOT(finished(int)));

    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()),
            Qt::DirectConnection);

    connect(this, SIGNAL(terminated()), worker, SLOT(terminate()));
    m_worker->moveToThread(this);
  }

  ~TomographyThread() override {
    // this will terminate the process if another reconstruction is started,
    // thus not allowing to have multiple reconstructions running at the same
    // time
    emit terminated();

    // this causes segfault in processRefreshJobs if the check isnt here
    if (m_workerRunning || !m_worker) {
      // emit that the worker has been forcefully closed, exit with error code 1
      // this is bad, find a way to notify without an explicit emit on thread
      // destroy
      emit workerFinished(m_workerPID, 1);
    }
  }

  void setProcessPID(const qint64 pid) { m_workerPID = pid; }

  qint64 getProcessPID() const { return m_workerPID; }

public slots:
  void finished(const int exitCode) {
    // queue up object deletion
    m_worker->deleteLater();
    m_workerRunning = false;
    // emit the exit code to the presenter so the process info can be updated
    emit workerFinished(m_workerPID, exitCode);
  }

  void readWorkerStdOut() const {
    auto *worker = qobject_cast<TomographyProcess *>(sender());
    QString out(worker->readAllStandardOutput());
    if (!out.isEmpty())
      emit stdOutReady(out.trimmed());
  }

  void readWorkerStdErr() const {
    auto *worker = qobject_cast<TomographyProcess *>(sender());
    QString out(worker->readAllStandardError());

    if (!out.isEmpty())
      emit stdErrReady(out.trimmed());
  }

  void startWorker() { m_workerRunning = true; }

signals:
  void workerFinished(const qint64, const int);
  void stdOutReady(const QString &s) const;
  void stdErrReady(const QString &s) const;

private:
  bool m_workerRunning = false;
  /// Holder for the current running process' PID
  qint64 m_workerPID;
  TomographyProcess *const m_worker;
};
} // CustomInterfaces
} // MantidQt
#endif
