#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESSHANDLER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESSHANDLER_H_

#include "MantidKernel/make_unique.h"
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QThread>
#include <iostream>
#include <memory>
#include <mutex>
#include <utility>

// more things
#include <Poco/Path.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>

namespace MantidQt {
namespace CustomInterfaces {

class TomographyProcessHandler : public QProcess {
  Q_OBJECT
public:
  TomographyProcessHandler() : QProcess() {}

  // intentionally copy the vector
  void setup(const std::string runnable, const std::vector<std::string> &args) {
    m_runnable = std::move(QString::fromStdString(runnable));
    m_args = std::move(constructArgumentsFromVector(args));

    std::cout << "\nDEBUG >> SETUP PROCESS >> " << m_runnable.toStdString()
              << " ARGS >> " << m_allArgs;
  }

  std::string getRunnable() { return m_runnable.toStdString(); }
  std::string getArgs() { return m_allArgs; }
  Q_PID getPID() { return this->pid(); }

public slots:
  void startWorker() {
    // DEBUG
    std::cout << "\nDEBUG >> STARTING PROCESS <<";
    start(m_runnable, m_args);
  }

private:
  QStringList
  constructArgumentsFromVector(const std::vector<std::string> &args) {
    QStringList list;

    for (auto &arg : args) {
      list << QString::fromStdString(arg);
      m_allArgs += arg + " ";
    }

    return list;
  }

  QString m_runnable;
  QStringList m_args;

  std::string m_allArgs;
};

class TomographyThreadHandler : public QThread {
  Q_OBJECT
public:
  TomographyThreadHandler(QObject *parent, TomographyProcessHandler *worker)
      : QThread(parent) {
    connect(worker, SIGNAL(readyReadStandardOutput()), this,
            SLOT(readWorkerStdOut()));
    connect(worker, SIGNAL(readyReadStandardError()), this,
            SLOT(readWorkerStdErr()));
  }

public slots:
  void readWorkerStdOut() {
    auto *worker = qobject_cast<TomographyProcessHandler *>(sender());
    QString output(worker->readAllStandardOutput());
    emit stdOutReady(output);
  }

  void readWorkerStdErr() {
    auto *worker = qobject_cast<TomographyProcessHandler *>(sender());
    QString output(worker->readAllStandardError());
    emit stdErrReady(output);
  }

signals:
  void stdOutReady(const QString &s);
  void stdErrReady(const QString &s);

private:
  std::unique_ptr<QString> m_stdOut;
  std::unique_ptr<QString> m_stdErr;
  bool m_pidReady = false;
  size_t m_pid = 0;
};
} // CustomInterfaces
} // MantidQt
#endif
