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

class QsciLexer; //Mantid - For QScintilla
class QsciAPIs;

//! An interpreter for evaluating scripting code. Abstract.
/**
 * ScriptingEnv objects represent a running interpreter, possibly with global
 * variables, and are responsible for generating Script objects (which do
 * the actual evaluation of code).
 */
class ScriptingEnv : public QObject
{
  Q_OBJECT

  public:
  ScriptingEnv(ApplicationWindow *parent, const char *langName);
  ~ScriptingEnv();
  //! Initialize the environment
  bool initialize();
  bool isRunning() const { return m_is_running; }
  void setIsRunning(bool running) { m_is_running = running; }

  //! Instantiate the Script subclass matching the ScriptEnv subclass.
  virtual Script *newScript(const QString&, QObject*, const QString&) { return 0; }
      
  //! If an exception / error occured, return a nicely formated stack backtrace.
  virtual QString stackTraceString() { return QString::null; }

  //! Return a list of supported mathematical functions. These should be imported into the global namespace.
  virtual const QStringList mathFunctions() const { return QStringList(); }
  //! Return a documentation string for the given mathematical function.
  virtual const QString mathFunctionDoc(const QString&) const { return QString::null; }
  //! Return a list of file extensions commonly used for this language.
  virtual const QStringList fileExtensions() const { return QStringList(); };
  //! Construct a filter expression from fileExtension(), suitable for QFileDialog.
  const QString fileFilter() const;

  const QString scriptingLanguage() const;
  //If the environment supports evaluation as well as execution then override and return true
  virtual bool supportsEvaluation() { return false; }
  //! Is progress reporting supported
  virtual bool supportsProgressReporting() const { return false; }
  //!Whether we should be reporting progress  
  bool reportProgress() const { return m_report_progress; }
  //!Set whether we should be reporting progress
  void reportProgress(bool on) { m_report_progress = on; }

  /// Return the code lexer, can be NULL
  QsciLexer* getCodeLexer() const { return m_lexer; };
  /// Set the code lexer for this environment
  void setCodeLexer(QsciLexer* lexer);
  /// Execute a code string in the current environment
  void execute(const QString & code);						

  public slots:
    // global variables
    virtual bool setQObject(QObject*, const char*) { return false; }
    virtual bool setInt(int, const char*) { return false; }
    virtual bool setDouble(double, const char*) { return false; }

    virtual void refreshAlgorithms() {};
    virtual void refreshCompletion() {};

    //! Clear the global environment. What exactly happens depends on the implementation.
    virtual void clear() {}

    /// Slot to handle the started signal from the api auto complete preparation
    void apiPrepStarted();
    /// Slot to handle a cancelled signal from the api auto-complete preparation
    void apiPrepCancelled();
    /// Slot to handle the completed signal from the api auto complete preparation
    void apiPrepDone();

    //! Increase the reference count. This should only be called by scripted and Script to avoid memory leaks.
    void incref();
    //! Decrease the reference count. This should only be called by scripted and Script to avoid segfaults.
    void decref();

  signals:
    //! signal an error condition / exception
    void error(const QString & message, const QString & scriptName, int lineNumber);
    //! output that is not handled by a Script
    void print(const QString & output);
    
protected:
    // Load code completion source file
    void updateCodeCompletion(const QString & fname, bool prepare);

  protected:
    //! whether the interpreter has been successfully initialized
    bool d_initialized;
    //! the context in which we are running
    ApplicationWindow *d_parent;
    // The current script object
    Script* m_current_script;

private:
  /** Override to perform some initialisation code */
  virtual bool start() { return true; }
  /** Override to perform some finalisation code */
  virtual void shutdown() {}

  private:
    //! the reference counter
    int d_refcount;
  
  //Mantid - Store the language name of the concrete implementation so that
  // the script window title can be set appropriately
  const char * languageName;
  // Is progress reporting on?
  bool m_report_progress;
  // Whether a script is running
  bool m_is_running;
  /// An optional code lexer for this environment
  QsciLexer *m_lexer;
  /// An installed API for code completion
  QsciAPIs *m_completer;
  /// Flag indicating if the API is currently preparing code completion information
  bool m_api_preparing;
};

#endif
