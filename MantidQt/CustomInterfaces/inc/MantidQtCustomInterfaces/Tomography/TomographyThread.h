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
      : QThread(parent) {
    // interactions between the thread and the worker are defined here
    connect(this, SIGNAL(started()), worker, SLOT(startWorker()));

    connect(worker, SIGNAL(readyReadStandardOutput()), this,
            SLOT(readWorkerStdOut()));
    connect(worker, SIGNAL(readyReadStandardError()), this,
            SLOT(readWorkerStdErr()));

    connect(worker, SIGNAL(finished(int)), this, SLOT(workerFinished(int)));

    // make sure we know who the worker is
    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()),
            Qt::DirectConnection);

    // TODO test if terminates process
    connect(this, SIGNAL(terminated()), worker, SLOT(terminate()));

    worker->moveToThread(this);
    m_worker = worker;
  }

  ~TomographyThread() override {
	  emit terminated(); // TODO try this tomorrow!
  }

public slots:
  void workerFinished(int exitCode) {
    UNUSED_ARG(exitCode);
    // queue up object deletion
    m_worker->deleteLater();
    emit workerFinished();
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

signals:
  void workerFinished();
  void stdOutReady(const QString &s) const;
  void stdErrReady(const QString &s) const;

private:
  TomographyProcess *m_worker;
};
} // CustomInterfaces
} // MantidQt
#endif
