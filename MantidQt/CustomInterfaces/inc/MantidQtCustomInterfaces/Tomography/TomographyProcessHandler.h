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
              << " ARGS >> " << constructSingleStringFromVector(m_args);
  }

  std::string constructSingleStringFromVector(const QStringList &args) const {
    std::string allOpts;
    for (const auto &arg : args) {
      allOpts += arg.toStdString() + " ";
    }
    return allOpts;
  }

public slots:
  bool isRunning() { return this->state() == QProcess::ProcessState::Running; }
  void startWorker() {
    // DEBUG
    std::cout << "\nDEBUG >> STARTING PROCESS <<";
    start(m_runnable, m_args);
  }

  void readStdOut() {
    QString output(readAllStandardOutput());
    std::string stdOutput = output.toStdString();
    std::cout << "\nDEBUG >> PROCESS STDOUT";
  }

  void readStdErr() {
    QString error(readAllStandardError());
    std::string stdError = error.toStdString();
    std::cout << "\nDEBUG >> PROCESS ERROR";
  }

private:
  QStringList
  constructArgumentsFromVector(const std::vector<std::string> &args) {
    QStringList list;

    for (auto &arg : args) {
      list << QString::fromStdString(arg);
    }

    return list;
  }

  QString m_runnable;
  QStringList m_args;
};

class TomographyThreadHandler : public QThread {
  Q_OBJECT
public:
  TomographyThreadHandler(QObject *parent, TomographyProcessHandler * worker) : QThread(parent) {
    connect(worker, SIGNAL(readyReadStandardOutput()), this, SLOT(readWorkerStdOut()));
    connect(worker, SIGNAL(readyReadStandardError()), this, SLOT(readWorkerStdErr()));
  }

  void run() {
    auto pid = getPID();
    std::cout << "\n\nDEBUG >> STARTING THREAD WITH ID: " << pid << "\n";
    this->exec();
  }


public slots:
  size_t getPID() {
    // auto pid = this->currentThreadId();
//    #ifdef _WIN32
    // m_pid = (long long)pid;
//    #else
//    m_pid = pid;
//    #endif

    return 0;
  }
  void readWorkerStdOut() {
    auto * worker = qobject_cast<TomographyProcessHandler*>(sender());
    QString output(worker->readAllStandardOutput());
    emit stdOutReady(output);    
  }

  void readWorkerStdErr() {
    auto * worker = qobject_cast<TomographyProcessHandler*>(sender());
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
