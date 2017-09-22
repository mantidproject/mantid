/***************************************************************************
  File                 : PythonScripting.h
  Project              : QtiPlot
--------------------------------------------------------------------
  Copyright            : (C) 2006 by Knut Franke
  Email (use @ for *)  : knut.franke*gmx.de
  Description          : Execute Python code from within QtiPlot

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
#ifndef PYTHON_SCRIPTING_H
#define PYTHON_SCRIPTING_H

#include "PythonScript.h"
#include "ScriptingEnv.h"

#include <QSharedPointer>

class QObject;
class QString;

/**
 * A scripting environment for executing Python code.
 */
class PythonScripting : public ScriptingEnv {

  Q_OBJECT

public:
  /// Factory function
  static ScriptingEnv *constructor(ApplicationWindow *parent);
  /// Destructor
  ~PythonScripting() override;
  /// Write text to std out
  void write(const QString &text) { emit print(text); }
  /// Simulate file-like object (required for IPython)
  inline void flush() {}
  /// Simulate file-like object (required for colorama)
  inline bool closed() const { return false; }
  /// Simulate file-like object
  inline bool isatty() const { return false; }

  /// 'Fake' method needed for IPython import
  void set_parent(PyObject *) {}

  /// Set the argv attribute on the sys module
  void setSysArgs(const QStringList &args) override;

  /// Create a new script object that can execute code within this enviroment
  Script *newScript(const QString &name, QObject *context,
                    const Script::InteractionType interact) const override;

  /// Create a new code lexer for Python
  QsciLexer *createCodeLexer() const override;
  void redirectStdOut(bool on) override;

  // Python supports progress monitoring
  bool supportsProgressReporting() const override { return true; }
  /// Does this support abort requests?
  bool supportsAbortRequests() const override { return true; }

  /// Return a string represenation of the given object
  QString toString(PyObject *object, bool decref = false);
  /// Convert a Python list object to a Qt QStringList
  QStringList toStringList(PyObject *py_seq);
  /// Convert a QtringList to a Python List
  PyObject *toPyList(const QStringList &items);
  /// Returns an integer representation of the object. No check is performed to
  /// see if it is an integer
  long toLong(PyObject *object, bool decref = false);
  /// Raise an asynchronous exception in the given thread
  void raiseAsyncException(long id, PyObject *exc);

  /// Return a list of file extensions for Python
  const QStringList fileExtensions() const override;

  /// Set a reference to a QObject in the given dictionary
  bool setQObject(QObject *, const char *, PyObject *dict);
  /// Set a reference to a QObject in the global dictionary
  bool setQObject(QObject *val, const char *name) override {
    return setQObject(val, name, NULL);
  }
  /// Set a reference to an int in the global dictionary
  bool setInt(int, const char *) override;
  bool setInt(int, const char *, PyObject *dict);
  /// Set a reference to a double in the global dictionary
  bool setDouble(double, const char *) override;
  bool setDouble(double, const char *, PyObject *dict);

  /// Return a list of mathematical functions define by qtiplot
  const QStringList mathFunctions() const override;
  /// Return a doc string for the given function
  const QString mathFunctionDoc(const QString &name) const override;
  /// Return the global dictionary for this environment
  PyObject *globalDict() { return m_globals; }
  /// Return the sys dictionary for this environment
  PyObject *sysDict() { return m_sys; }

private:
  /// Constructor
  explicit PythonScripting(ApplicationWindow *parent);
  /// Default constructor
  PythonScripting();
  /// Start the environment
  bool start() override;
  /// Shutdown the environment
  void shutdown() override;
  /// Configure python path
  void setupPythonPath();
  /// Configure sip
  void setupSip();
  /// Exec the mantidplotrc file
  bool loadInitRCFile();

private:
  /// The global dictionary
  PyObject *m_globals;
  /// A dictionary of math functions
  PyObject *m_math;
  /// The dictionary of the sys module
  PyObject *m_sys;
  /// Pointer to the main threads state
  PyThreadState *m_mainThreadState;
  /// Wrap's acquisition of the GIL
  PythonGIL m_gil;
};

#endif
