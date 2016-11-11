#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESSHANDLER_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESSHANDLER_H_

#include "MantidKernel/make_unique.h"
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QThread>
#include <iostream>
#include <memory>
#include <utility>

// more things
#include <Poco/Path.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>

namespace MantidQt {
namespace CustomInterfaces {

class TomographyThread : public QThread {
  Q_OBJECT
public:
  TomographyThread(QObject *parent) {
    m_process = Mantid::Kernel::make_unique<QProcess>(parent);
    connect(m_process.get(), SIGNAL(readyReadStandardOutput()), this,
            SLOT(readNewData()));
  }
  // intentionally copy the vector
  void setup(const std::string runnable, const std::vector<std::string> &args) {
    // m_process = Mantid::Kernel::make_unique<QProcess>();
    m_qRunnable = std::move(QString::fromStdString(runnable));
    m_qListArgs = std::move(constructArgumentsFromVector(args));
    m_ready = true;
  }

  bool isRunning() { return false; }

  QStringList
  constructArgumentsFromVector(const std::vector<std::string> &args) {
    QStringList list;

    for (auto &arg : args) {
      list << QString::fromStdString(arg);
    }

    return list;
  }

  Qt::HANDLE getPID() { return thread()->currentThreadId(); }

public slots:
  void readNewData() {
    // ...
    std::cout << "Im handling the output\n\n";
  }

private:
  void run() { m_process->start(m_qRunnable, m_qListArgs); }

  QString m_qRunnable;
  QStringList m_qListArgs;
  bool m_ready = false;
  std::unique_ptr<QProcess> m_process;
  // Poco::Process::PID m_pid = 0;
};

// class TomographyProcessHandler : public QObject{
//   public:
//     TomographyProcessHandler(){
//       m_thread = Mantid::Kernel::make_unique<TomographyThread>(this);
//     }

//     void setup(const std::string runnable, const std::vector<std::string>
//     &args){
//       m_thread->setup(runnable, args);
//     }

//     void start(){
//       m_thread->start();
//     }

//     Qt::HANDLE getPID(){
//       return m_thread->getPID();
//     }

// private:
//   std::unique_ptr<TomographyThread> m_thread;
// };
class TomographyProcessHandler : public QThread {
public:
  TomographyProcessHandler()
      : m_outPipe(), m_errPipe(), m_outstr(m_outPipe), m_errstr(m_errPipe) {}

  void setup(const std::string runnable, const std::vector<std::string> args) {
    // move the copies
    m_runnable = std::move(runnable);
    m_args = std::move(args);
  }

  Poco::Process::PID getPID() { return m_pid; }

  std::string getOutputString() { return getStringFromStream(m_outstr); }

  std::string getErrorString() { return getStringFromStream(m_errstr); }

private:
  void run() {
    // m_mutex.lock();
    m_handle = Mantid::Kernel::make_unique<Poco::ProcessHandle>(
        Poco::Process::launch(m_runnable, m_args));
    m_pid = m_handle->id();
    
  }

  std::string getStringFromStream(Poco::PipeInputStream &str) {
    // avoid deadlocking    
    if (m_pid > 0) {
      return "Reconstruction process running...";
    }
    std::string outstring;
    Poco::StreamCopier::copyToString(str, outstring);
    if (!outstring.empty())
      return outstring;
    return "Reconstruction process encountered unknown error! Please check reconstruction "
           "logs.";
  }

  // std::mutex m_mutex;
  std::string m_output;
  std::string m_runnable;
  std::vector<std::string> m_args;
  Poco::Process::PID m_pid;
  Poco::Pipe m_outPipe;
  Poco::Pipe m_errPipe;
  Poco::PipeInputStream m_outstr;
  Poco::PipeInputStream m_errstr;
  std::unique_ptr<Poco::ProcessHandle> m_handle;
};
} // CustomInterfaces
} // MantidQt
#endif
