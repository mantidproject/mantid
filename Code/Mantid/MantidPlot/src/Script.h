/***************************************************************************
    File                 : Script.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, knut.franke*gmx.de
    Description          : Scripting abstraction layer

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *  This program is free software; you can redistribute it and/or modify   *
 *  it under the terms of the GNU General Public License as published by   *
 *  the Free Software Foundation; either version 2 of the License, or      *
 *  (at your option) any later version.                                    *
 *                                                                         *
 *  This program is distributed in the hope that it will be useful,        *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *  GNU General Public License for more details.                           *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor,                    *
 *   Boston, MA  02110-1301  USA                                           *
 *                                                                         *
 ***************************************************************************/
#ifndef SCRIPT_H
#define SCRIPT_H

#include <QEvent>
#include <QFuture>
#include <QRunnable>
#include <QString>
#include <QStringList>
#include <QThreadPool>
#include <QVariant>

#include "ScriptCode.h"

//-------------------------------------------
// Forward declarations
//-------------------------------------------
class ApplicationWindow;
class ScriptingEnv;
class Script;

/**
 * Script objects represent a chunk of code, possibly together with local
 * variables. The code may be changed and executed multiple times during the
 * lifetime of an object.
 */
class Script : public QObject
{
  Q_OBJECT

  public:
  /// Interaction type
  enum InteractionType {Interactive, NonInteractive};
  /// Execution mode
  enum ExecutionMode {Serialised, Asynchronous, Running, NotExecuting};

  /// Constructor
  Script(ScriptingEnv *env, const QString &name, const InteractionType interact,
         QObject * context = NULL);
  /// Destructor
  ~Script();
  /// Returns the envirnoment this script is tied to
  inline ScriptingEnv *environment() { return m_env; }
  /// Returns the identifier for the script.
  inline const std::string & identifier() const { return m_name; }
  /// Update the identifier for the object.
  virtual void setIdentifier(const QString &name);
  /// Return the current context
  const QObject * context() const { return m_context; }
  /// Set the context in which the code is to be executed.
  virtual void setContext(QObject *context) { m_context = context; }

  /// Is this an interactive script
  inline bool isInteractive() { return m_interactMode == Interactive; }
  /// Is the script being executed
  inline bool isExecuting() const { return m_execMode != NotExecuting; }

  /// Enable progress reporting for this script
  void enableProgressReporting() { m_reportProgress = true; }
  /// Disable progress reporting for this script
  void disableProgressReporting() { m_reportProgress = false; }
  /// Query progress reporting state
  bool reportProgress() const { return m_reportProgress; }

  bool redirectStdOut() const { return m_redirectOutput; }
  void redirectStdOut(bool on) { m_redirectOutput = on; }

  /// Create a list of keywords for the code completion API
  virtual void generateAutoCompleteList() {};
  // Does the code compile to a complete statement, i.e no more input is required
  virtual bool compilesToCompleteStatement(const QString & code) const = 0;

public slots:
  /// Compile the code, returning true/false depending on the status
  bool compile(const ScriptCode & code);
  /// Evaluate the Code, returning QVariant() on an error / exception.
  QVariant evaluate(const ScriptCode & code);
  /// Execute the Code, returning false on an error / exception.
  bool execute(const ScriptCode & code);
  /// Execute the code asynchronously, returning immediately after the execution has started
  QFuture<bool> executeAsync(const ScriptCode & code);

  /// Asks Mantid to release all free memory
  void releaseFreeMemory();
  /// Sets the execution mode to NotExecuting
  void setNotExecuting();
  /// Sets the execution mode to Running to indicate something is running
  void setIsRunning();
 
  // local variables
  virtual bool setQObject(QObject*, const char*) { return false; }
  virtual bool setInt(int, const char*) { return false; }
  virtual bool setDouble(double, const char*) { return false; }
  
signals:
  /// A signal defining when this script has started executing
  void started(const QString & message);
  /// A signal defining when this script has completed successfully
  void finished(const QString & message);
  /// signal an error condition / exception
  void error(const QString & message, const QString & scriptName, int lineNumber);
  /// output generated by the code
  void print(const QString & output);
  /// Line number changed
  void currentLineChanged(int lineno, bool error);
  // Signal that new keywords are available
  void autoCompleteListGenerated(const QStringList & keywords);
  
protected:
  /// Return the true line number by adding the offset
  inline int getRealLineNo(const int codeLine) const { return codeLine + m_code.offset(); }
  /// Return the code string
  inline const std::string & codeString() const { return m_code.codeString(); }
  /// Return the script code object
  inline const ScriptCode & scriptCode() const { return m_code; }
  /// Compile the code, returning true/false depending on the status
  virtual bool compileImpl() = 0;
  /// Evaluate the Code, returning QVariant() on an error / exception.
  virtual QVariant evaluateImpl() = 0;
  /// Execute the Code, returning false on an error / exception.
  virtual bool executeImpl() = 0;

private:
  /**
   * Worker task for the asynchronous exec calls
   */
  class ScriptTask : public QFutureInterface<bool>, public QRunnable
  {
  public:
    ScriptTask(Script & script);
    QFuture<bool> start();
    void run();

  private:
    ScriptTask();
    Script & m_script;
  };
  /**
   * ThreadPool that allows only a single thread
   */
  class ScriptThreadPool : public QThreadPool
  {
  public:
    ScriptThreadPool();
  };

  /// Setup the code from a script code object
  void setupCode(const ScriptCode & code);
  /// Normalise line endings for the given code. The Python C/API does not seem to like CRLF endings so normalise to just LF
  QString normaliseLineEndings(QString text) const;

  ScriptingEnv *m_env;
  std::string m_name; //Easier to convert to C string
  ScriptCode m_code;
  QObject *m_context;
  bool m_redirectOutput;
  bool m_reportProgress;

  InteractionType m_interactMode;
  ExecutionMode m_execMode;

  ScriptThreadPool *m_thread;
};


#endif
