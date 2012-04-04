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

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QEvent>
#include <QFuture>

//-------------------------------------------
// Forward declarations
//-------------------------------------------
class ApplicationWindow;
class ScriptingEnv;

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
  enum ExecutionMode {Serialised, Asynchronous, NotExecuting};

  /// Constructor
  Script(ScriptingEnv *env, const QString &name, const InteractionType interact,
         QObject * context = NULL);
  /// Destructor
  ~Script();
  /// Returns the envirnoment this script is tied to
  inline ScriptingEnv *environment() { return m_env; }
  /// Returns the identifier for the script.
  inline const QString name() const { return m_name; }
  /// Returns the identifier as a C string
  inline const char * nameAsCStr() const { return m_name.toAscii(); }
  /// Update the identifier for the object.
  void setName(const QString &name) { m_name = name; }
  /// Return the current context
  const QObject * context() const { return m_context; }
  /// Set the context in which the code is to be executed.
  virtual void setContext(QObject *context) { m_context = context; }
  /// Is this an interactive script
  bool isInteractive() { return m_interactMode == Interactive; }
  /// Is the script being executed
  inline bool isExecuting() const { return m_execMode != NotExecuting; }

  bool redirectStdOut() const { return m_redirectOutput; }
  void redirectStdOut(bool on) { m_redirectOutput = on; }

  // Does the code compile to a complete statement, i.e no more input is required
  virtual bool compilesToCompleteStatement(const QString & code) const = 0;

public slots:
  /// Compile the code, returning true/false depending on the status
  virtual bool compile(const QString & code) = 0;
  /// Evaluate the Code, returning QVariant() on an error / exception.
  virtual QVariant evaluate(const QString & code) = 0;
  /// Execute the Code, returning false on an error / exception.
  virtual bool execute(const QString & code) = 0;
  /// Execute the code asynchronously, returning immediately after the execution has started
  virtual QFuture<bool> executeAsync(const QString & code) = 0;

  /// Sets the execution mode to NotExecuting
  void setNotExecuting();
  /// Sets the execution mode to Serialised
  void setExecutingSerialised();
  /// Sets the execution mode to Serialised
  void setExecutingAsync();

  // local variables
  virtual bool setQObject(QObject*, const char*) { return false; }
  virtual bool setInt(int, const char*) { return false; }
  virtual bool setDouble(double, const char*) { return false; }
  
signals:
  /// A signal defining when this script has started executing
  void started(const QString & message);
  /// A separate signal defining when this script has started executing serial running
  void startedSerial(const QString & message);
  /// A separate signal defining when this script has started executing asynchronously
  void startedAsync(const QString & message);
  /// A signal defining when this script has completed successfully
  void finished(const QString & message);
  /// signal an error condition / exception
  void error(const QString & message, const QString & scriptName, int lineNumber);
  /// output generated by the code
  void print(const QString & output);
  /// Line number changed
  void currentLineChanged(int lineno, bool error);
  // Signal that new keywords are available
  void keywordsChanged(const QStringList & keywords);
  
private:
  /// Normalise line endings for the given code. The Python C/API does not seem to like CRLF endings so normalise to just LF
  QString normaliseLineEndings(QString text) const;

  ScriptingEnv *m_env;
  QString m_name;
  InteractionType m_interactMode;
  QObject *m_context;
  bool m_redirectOutput;
  ExecutionMode m_execMode;
};




#endif
