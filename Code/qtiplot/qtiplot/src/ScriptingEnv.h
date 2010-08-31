/***************************************************************************
    File                 : ScriptingEnv.h
    Project              : QtiPlot
    --------------------------------------------------------------------
    Copyright            : (C) 2006 by Ion Vasilief, 
                           Tilman Hoener zu Siederdissen,
                           Knut Franke
    Email (use @ for *)  : ion_vasilief*yahoo.fr, thzs*gmx.net
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
#ifndef SCRIPTINGENV_H
#define SCRIPTINGENV_H

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QObject>
#include <QStringList>
#include <QEvent>

#include "customevents.h"

class ApplicationWindow;
class Script;
class QsciLexer;

/**
 * A ScriptingEnv object represents a running interpreter, possibly with global
 * variables, and is responsible for generating Script objects that perform
 * the actual evaluation of code.
 */
class ScriptingEnv : public QObject
{
  Q_OBJECT

  public:
  ///Constructor
  ScriptingEnv(ApplicationWindow *parent, const char *langName);
  /// Destructor
  ~ScriptingEnv();
  /// Initialize the environment
  bool initialize();
  /// Is the environment initialized
  bool isInitialized() const { return d_initialized; }
  /// Query if any code is currently being executed
  bool isRunning() const { return m_is_running; }
  /// Set that a script is being executed
  void setIsRunning(bool running) { m_is_running = running; }
  /// Create a script object that is responsible for executing actual code
  virtual Script *newScript(const QString&, QObject*, bool interactive = true, 
			    const QString &name="<input>")
  {
    (void)(interactive); //Stop compiler warning
    (void)(name); //Stop compiler warning
    return NULL;
  }
  //! If an exception / error occured, return a nicely formated stack backtrace.
  virtual QString stackTraceString() { return QString::null; }
  /// Return a list of supported mathematical functions. These should be imported into the global namespace.
  virtual const QStringList mathFunctions() const { return QStringList(); }
  /// Return a documentation string for the given mathematical function.
  virtual const QString mathFunctionDoc(const QString&) const { return QString::null; }
  /// Return a list of file extensions commonly used for this language.
  virtual const QStringList fileExtensions() const { return QStringList(); };
  /// Construct a filter expression from fileExtension(), suitable for QFileDialog.
  const QString fileFilter() const;
  /// Return the name of the scripting language supported by this environment
  const QString scriptingLanguage() const;
  /// If the environment supports evaluation as well as execution then override and return true
  virtual bool supportsEvaluation() { return false; }
  ///  Is progress reporting supported
  virtual bool supportsProgressReporting() const { return false; }
  /// Whether we should be reporting progress  
  bool reportProgress() const { return m_report_progress; }
  //!Set whether we should be reporting progress
  void reportProgress(bool on) { m_report_progress = on; }
  /// Create a code lexer for this environment, can be NULL. Ownership of a created object 
  /// is transferred to the caller.
  virtual QsciLexer * createCodeLexer() const { return NULL; }

public slots:
  /// Set a reference to a QObject in the global scope
  virtual bool setQObject(QObject*, const char*) { return false; }
  /// Set a reference to an integer in the global scope
  virtual bool setInt(int, const char*) { return false; }
  /// Set a reference to a double  in the global scope
  virtual bool setDouble(double, const char*) { return false; }
  /// Update the global namespace of algorithms. Bit of a hack for Python algorithms
  virtual void refreshAlgorithms() {};
  
  /// Clear the global environment. What exactly happens depends on the implementation.
  virtual void clear() {}
  /// Increase the reference count. This should only be called by Scripted and Script to avoid memory leaks.
  void incref();
  /// Decrease the reference count. This should only be called by Scripted and Script to avoid segfaults.
  void decref();
  
signals:
  /// signal an error condition / exception
  void error(const QString & message, const QString & scriptName, int lineNumber);
  /// output that is not handled by a Script
  void print(const QString & output);

protected:
  /// whether the interpreter has been successfully initialized
  bool d_initialized;
  /// the context in which we are running
  ApplicationWindow *d_parent;

private:
  /// Private default constructor
  ScriptingEnv();
  /** Override to perform some initialisation code */
  virtual bool start() { return true; }
  /** Override to perform some finalisation code */
  virtual void shutdown() {}

private:
  /// the reference counter
  int d_refcount;
  ///Mantid - Store the language name of the concrete implementation so that
  /// the script window title can be set appropriately
  const char * languageName;
  /// Is progress reporting on?
  bool m_report_progress;
  /// Whether a script is running
  bool m_is_running;
};

/**
 * Keeps a static list of available interpreters and instantiates them on demand
 */
class ScriptingLangManager
{
public:
  /// Return an instance of the first implementation we can find.
  static ScriptingEnv *newEnv(ApplicationWindow *parent);
  /// Return an instance of the implementation specified by name, NULL on failure.
  static ScriptingEnv *newEnv(const char *name, ApplicationWindow *parent);
  /// Return the names of available implementations.
  static QStringList languages();
  /// Return the number of available implementations.
  static int numLanguages();
  
private:
  typedef ScriptingEnv*(*ScriptingEnvConstructor)(ApplicationWindow*);
  typedef struct {
    const char *name;
    ScriptingEnvConstructor constructor;
  } ScriptingLang;
  /// Available languages
  static ScriptingLang g_langs[];
};

#endif
