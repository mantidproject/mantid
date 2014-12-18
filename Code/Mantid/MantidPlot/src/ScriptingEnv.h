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

#include "Script.h"

#include <QVariant>
#include <QStringList>
#include <QObject>
#include <QEvent>

#include "customevents.h"

class ApplicationWindow;
class QsciLexer;

/**
 * A ScriptingEnv object represents a running interpreter. It can create
 * script objects that execute arbitrary strings of code.
 */
class ScriptingEnv : public QObject
{
  Q_OBJECT

  public:
  ///Constructor
  ScriptingEnv(ApplicationWindow *parent, const QString & langName);
  /// Destructor
  ~ScriptingEnv();
  /// Start the environment
  bool initialize();
  /// Shutdown the environment in a more controlled manner than the destructor allows
  void finalize();

  /// Is the environment initialized
  bool isInitialized() const { return d_initialized; }

  /// If the environment supports it, set the system arguments
  virtual void setSysArgs(const QStringList & args) = 0;

  /// Create a script object that is responsible for executing actual code
  virtual Script *newScript(const QString &name, QObject * context, const Script::InteractionType interact) const = 0;

  //! If an exception / error occured, return a nicely formated stack backtrace.
  virtual QString stackTraceString() { return QString::null; }
  /// Return a list of supported mathematical functions. These should be imported into the global namespace.
  virtual const QStringList mathFunctions() const { return QStringList(); }
  /// Return a documentation string for the given mathematical function.
  virtual const QString mathFunctionDoc(const QString&) const { return QString::null; }
  /// Return a list of file extensions commonly used for this language.
  virtual const QStringList fileExtensions() const { return QStringList(); }
  /// Construct a filter expression from fileExtension(), suitable for QFileDialog.
  const QString fileFilter() const;
  /// Return the name of the scripting language supported by this environment
  const QString languageName() const;
  /// If the environment supports evaluation as well as execution then override and return true
  virtual bool supportsEvaluation() { return false; }
  ///  Is progress reporting supported
  virtual bool supportsProgressReporting() const { return false; }
  /// Create a code lexer for this environment, can be NULL. Ownership of a created object 
  /// is transferred to the caller.
  virtual QsciLexer * createCodeLexer() const { return NULL; }

  virtual void redirectStdOut(bool) {}

public slots:
  /// Set a reference to a QObject in the global scope
  virtual bool setQObject(QObject*, const char*) { return false; }
  /// Set a reference to an integer in the global scope
  virtual bool setInt(int, const char*) { return false; }
  /// Set a reference to a double  in the global scope
  virtual bool setDouble(double, const char*) { return false; }
  
  /// Clear the global environment. What exactly happens depends on the implementation.
  virtual void clear() {}
  /// Increase the reference count. This should only be called by Scripted and Script to avoid memory leaks.
  void incref();
  /// Decrease the reference count. This should only be called by Scripted and Script to avoid segfaults.
  void decref();
  
signals:
  /// Starting
  void starting();
  /// Stopping
  void shuttingDown();
  /// signal an error condition / exception
  void error(const QString & message, const QString & scriptName, int lineNumber);
  /// output that is not handled by a Script
  void print(const QString & output);

protected:
  /// Override to perform some initialisation code
  virtual bool start() { return true; }
  /// Override to perform shutdown code
  virtual void shutdown() {}
  /// Set that a script is being executed
   void setIsRunning(bool running) { m_is_running = running; }


  /// whether the interpreter has been successfully initialized
  bool d_initialized;
  /// the context in which we are running
  ApplicationWindow *d_parent;

private:
  /// Private default constructor
  ScriptingEnv();

private:
  /// Whether a script is running
  bool m_is_running;
  int d_refcount;
  QString m_languageName;
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
  static ScriptingEnv *newEnv(const QString &name, ApplicationWindow *parent);
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
