#ifndef MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESS_H_
#define MANTIDQTCUSTOMINTERFACES_TOMOGRAPHY_TOMOGRAPHYPROCESS_H_

#include <QProcess>
#include <QString>
#include <QStringList>
#include <vector>

namespace MantidQt {
namespace CustomInterfaces {

/*
TomographyProcess class that run external processes and provides some helper
functions. This class was designed to be used with TomographyThread to run
external processes asyncronously to the main thread of Mantid.

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
class TomographyProcess : public QProcess {
  Q_OBJECT
public:
  // we want a nullptr for parent so we can move it to a thread
  TomographyProcess() : QProcess(nullptr) {}

  // intentionally copy the vector
  void setup(const std::string &runnable, const std::vector<std::string> &args,
             const std::string &allOpts) {
    m_allArgs = allOpts;
    m_runnable = QString::fromStdString(runnable);
    m_args = buildArguments(args);
  }

  std::string getRunnable() const { return m_runnable.toStdString(); }
  std::string getArgs() const { return m_allArgs; }

  qint64 getPID() const {
    auto pid = this->pid();

    // qt connect could sometimes try to read the terminated process' PID
    if (!pid) {
      return 0;
    }

#ifdef _WIN32
    // windows gets a struct object with more info
    auto actualpid = static_cast<qint64>(pid->dwProcessId);
#else
    // linux just gets the PID
    auto actualpid = static_cast<qint64>(pid);
#endif
    return actualpid;
  }

public slots:
  /** This method should be used to start the worker as it passes the setup
   * runnable and args parameters into the base start method
  */
  void startWorker() { start(m_runnable, m_args); }

private:
  QStringList buildArguments(const std::vector<std::string> &args) const {
    QStringList list;
    list.reserve(static_cast<int>(args.size()));

    for (const auto &arg : args) {
      list << QString::fromStdString(arg);
    }

    return list;
  }

  QString m_runnable;
  QStringList m_args;

  std::string m_allArgs;
};
} // CustomInterfaces
} // MantidQt
#endif
