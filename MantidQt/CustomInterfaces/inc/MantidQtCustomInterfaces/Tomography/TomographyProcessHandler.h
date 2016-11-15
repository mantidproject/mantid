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

// class TomographyProcess : public QProcess {
//   Q_OBJECT
// public:
//   TomographyThread(QObject *parent) {
//     m_process = Mantid::Kernel::make_unique<QProcess>(parent);
//     connect(m_process.get(), SIGNAL(readyReadStandardOutput()), this,
//             SLOT(readNewData()));
//   }
//   // intentionally copy the vector
//   void setup(const std::string runnable, const std::vector<std::string>
//   &args) {
//     // m_process = Mantid::Kernel::make_unique<QProcess>();
//     m_qRunnable = std::move(QString::fromStdString(runnable));
//     m_qListArgs = std::move(constructArgumentsFromVector(args));
//     m_ready = true;
//   }

//   QStringList
//   constructArgumentsFromVector(const std::vector<std::string> &args) {
//     QStringList list;

//     for (auto &arg : args) {
//       list << QString::fromStdString(arg);
//     }

//     return list;
//   }

//   Qt::HANDLE getPID() { return thread()->currentThreadId(); }

// public slots:
//   void readNewData() {
//     // ...
//     std::cout << "Im handling the output\n\n";
//   }

// private:
//   void run() { m_process->start(m_qRunnable, m_qListArgs); }

//   QString m_qRunnable;
//   QStringList m_qListArgs;
//   bool m_ready = false;
//   std::unique_ptr<QProcess> m_process;
//   // Poco::Process::PID m_pid = 0;
// };

class TomographyProcessHandler : public QProcess {
  Q_OBJECT
public:
  TomographyProcessHandler(QObject *parent) : QProcess(parent) {}

  // intentionally copy the vector
  void setup(const std::string runnable, const std::vector<std::string> &args) {
    // m_process = Mantid::Kernel::make_unique<QProcess>();
    // const std::vector<std::string> argggggggg = {
    //     "C:/Users/QBR77747/Documents/mantid_fourth/mantid/scripts/Imaging/IMAT/"
    //     "tomo_reconstruct.py",
    //     "--tool=tomopy", "--algorithm=gridrec,", "--num-iter=5",
    //     "--input-path=C:/Users/QBR77747/Documents/mantid_workspaces/imaging/"
    //     "RB000888_test_stack_larmor_summed_201510/data_stack_larmor_summed",
    //     "--input-path-flat=C:/Users/QBR77747/Documents/mantid_workspaces/"
    //     "imaging/"
    //     "RB000888_test_stack_larmor_summed_201510/flat_stack_larmor_summed",
    //     "--input-path-dark=C:/Users/QBR77747/Documents/mantid_workspaces/"
    //     "imaging/"
    //     "RB000888_test_stack_larmor_summed_201510/dark_stack_larmor_summed",
    //     "--output=C:/Users/QBR77747/Documents/mantid_workspaces/imaging/"
    //     "RB000888_test_stack_larmor_summed_201510/processed/"
    //     "reconstruction_TomoPy_gridrec_2016November14_160724_215398000",
    //     "--median-filter-size=3", "--cor=0.000000", "--rotation=0",
    //     "--max-angle=360.000000", "--circular-mask=0.940000",
    //     "--out-img-format=png"};
    // const std::vector<std::string> argggggggg = {
    //     "C:\\Users\\QBR77747\\Documents\\mantid_"
    //     "fourth\\mantid\\scripts\\Imaging\\IMAT\\"
    //     "tomo_reconstruct.py",
    //     "--tool=tomopy", "--algorithm=gridrec,", "--num-iter=5",
    //     "--input-path=C:\\Users\\QBR77747\\Documents\\mantid_"
    //     "workspaces\\imaging\\"
    //     "RB000888_test_stack_larmor_summed_201510\\data_stack_larmor_summed",
    //     "--input-path-flat=C:\\Users\\QBR77747\\Documents\\mantid_workspaces\\"
    //     "imaging\\"
    //     "RB000888_test_stack_larmor_summed_201510\\flat_stack_larmor_summed",
    //     "--input-path-dark=C:\\Users\\QBR77747\\Documents\\mantid_workspaces\\"
    //     "imaging\\"
    //     "RB000888_test_stack_larmor_summed_201510\\dark_stack_larmor_summed",
    //     "--output=C:\\Users\\QBR77747\\Documents\\mantid_workspaces\\imaging\\"
    //     "RB000888_test_stack_larmor_summed_201510\\processed\\"
    //     "reconstruction_TomoPy_gridrec_2016November14_160724_215398000",
    //     "--median-filter-size=3", "--cor=0.000000", "--rotation=0",
    //     "--max-angle=360.000000", "--circular-mask=0.940000",
    //     "--out-img-format=png"};
    // const std::string runnnnnnnn = "C:\\Anaconda\\python.exe";
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
    std::cout << "\n\n\nDEBUG >> STARTING PROCESS <<<<<<<<\n\n\n";
    start(m_runnable, m_args);
    auto env = this->processEnvironment();
    std::cout << "\nDEBUG >> PROCESSID " << this->pid() << " ... "
              << (long long)this->pid() << "\n";
    auto currentPath = env.value("PATH");
    std::cout << "\nDEBUG >> PROCESS CURRENT PATH >> "
              << currentPath.toStdString() << "\n";
    QString newPath =
        currentPath + ";C:\\Anaconda\\Lib;C:\\Anaconda\\Lib\\site-packages";
    std::cout << "\nDEBUG >> PROCESS NEW PATH >> " << newPath.toStdString()
              << "\n";
    env.insert("PATH", newPath);
    this->setProcessEnvironment(env);
    // pretty sure this blocks the main thread..
    while (waitForReadyRead(10000)) {
    }

    QString output(readAllStandardOutput());
    QString error(readAllStandardError());
    std::string stdOutput = output.toStdString();
    std::cout << "\nDEBUG >> PROCESS STDOUT" << stdOutput << "\n";
    std::string stdError = error.toStdString();
    std::cout << "\nDEBUG >> PROCESS ERROR" << stdError << "\n";
    emit finished();
  }

signals:
  void finished();

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
  TomographyThreadHandler(QObject *parent) : QThread(parent) {}

  void run() {
    std::cout << "\n\nDEBUG >> STARTING THREAD\n\n";
    emit started();
    this->exec();
  }
  size_t getPID() {
    auto pid = this->currentThreadId();

    // u wot? static_cast?
    return (long long)pid;
  }

signals:
  void started();
};
// class TomographyProcessHandler : public QThread {
// public:
//   TomographyProcessHandler()
//       : m_pid(0), m_outPipe(), m_errPipe(), m_outstr(m_outPipe),
//       m_errstr(m_errPipe) {}

//   ~TomographyProcessHandler(){
//     Poco::Process::kill(m_pid);
//   }
//   void setup(const std::string runnable, const std::vector<std::string> args)
//   {
//     // move the copies
//     m_runnable = std::move(runnable);
//     m_args = std::move(args);
//   }
//   int exit(){
//     this->exit();
//   }
//   Poco::Process::PID getPID() const {
//     if(this->isRunning())
//       while(m_pid == 0){
//       }
//       return m_pid;
//     return 0;
//   }

//   std::string getOutputString() { return getStringFromStream(m_outstr); }

//   std::string getErrorString() { return getStringFromStream(m_errstr); }

//   bool runzz(Poco::Process::PID pid) const {
//     return this->isRunning() && Poco::Process::isRunning(pid);
//   }
//   bool runzz() const {
//     return this->isRunning() && Poco::Process::isRunning(m_pid);
//   }

// private:
//   void run() {
//     m_handle = Mantid::Kernel::make_unique<Poco::ProcessHandle>(
//         Poco::Process::launch(m_runnable, m_args));
//     m_pid = m_handle->id();
//     m_mutex.lock();
//     // Poco::StreamCopier::copyStream(m_outstr, *m_stream);
//     // while(Poco::Process::isRunning(m_handle.get())){
//     Poco::StreamCopier::copyToString(m_outstr, m_outString);
//     // }
//     m_mutex.unlock();
//   }

//   std::string getStringFromStream(Poco::PipeInputStream &str) {
//     // avoid deadlocking
//     // if (m_pid > 0) {
//     //   return "Reconstruction process running...";
//     // }

//     if(!m_mutex.try_lock()){
//       return "Reconstruction in progress...";
//     }
//     // std::string outstring;
//     // Poco::StreamCopier::copyToString(str, outstring, 256);
//     // if (!outstring.empty())
//     //   return outstring;
//     // remove the trylock
//     m_mutex.unlock();
//     if (!m_outString.empty())
//       return m_outString;
//     return "Reconstruction process encountered unknown error! Please check
//     reconstruction "
//            "logs.";
//   }

//   std::mutex m_mutex;
//   std::ostream * m_stream;
//   std::string m_outString = "Reconstruction in progress...";
//   std::string m_runnable;
//   std::vector<std::string> m_args;
//   Poco::Process::PID m_pid;
//   Poco::Pipe m_outPipe;
//   Poco::Pipe m_errPipe;
//   Poco::PipeInputStream m_outstr;
//   Poco::PipeInputStream m_errstr;
//   std::unique_ptr<Poco::ProcessHandle> m_handle;
// };
} // CustomInterfaces
} // MantidQt
#endif
