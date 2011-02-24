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

class QObject;
class QString;

/**
 * A scripting environment for executing Python code.
 */
class PythonScripting: public ScriptingEnv
{

  Q_OBJECT
  
public:
  /// Factory function
  static ScriptingEnv *constructor(ApplicationWindow *parent);
  /// Destructor
  ~PythonScripting();
  /// Write text to std out
  void write(const QString &text) { emit print(text); }
  /// Create a new code lexer for Python
  QsciLexer * createCodeLexer() const;
  // Python supports progress monitoring
  virtual bool supportsProgressReporting() const { return true; }
  /// Return a string represenation of the given object
  QString toString(PyObject *object, bool decref = false);
  //Convert a Python list object to a Qt QStringList
  QStringList toStringList(PyObject *py_seq);
  /// Create a new script object that can execute code within this enviroment
  Script *newScript(const QString &code, QObject *context, bool interactive = true, const QString &name="<input>")
  {
    return new PythonScript(this, code, interactive, context, name);
  }
  ///Return a list of file extensions for Python
  const QStringList fileExtensions() const;
  /// Set a reference to a QObject in the given dictionary
  bool setQObject(QObject*, const char*, PyObject *dict);
  /// Set a reference to a QObject in the global dictionary
  bool setQObject(QObject *val, const char *name) { return setQObject(val,name,NULL); }
  /// Set a reference to an int in the global dictionary
  bool setInt(int, const char*, PyObject *dict=NULL);
  /// Set a reference to a double in the global dictionary
  bool setDouble(double, const char*, PyObject *dict=NULL);
  /// Return a list of mathematical functions define by qtiplot
  const QStringList mathFunctions() const;
  /// Return a doc string for the given function
  const QString mathFunctionDoc (const QString &name) const;
  /// Return the global dictionary for this environment
  PyObject *globalDict() { return m_globals; }
  /// Return the sys dictionary for this environment
  PyObject *sysDict() { return m_sys; }
  /// The language name
  static const char * langName;
				     
public slots:
  /// Refresh Python algorithms state
  virtual void refreshAlgorithms(bool force = false);

private:
  /// Constructor
  PythonScripting(ApplicationWindow *parent);
  /// Default constructor
  PythonScripting();
  /// Start the environment
  bool start();
  /// Shutdown the environment
  void shutdown();
  /// Run execfile on a given file
  bool loadInitFile(const QString &path);

private:
  /// The global dictionary
  PyObject *m_globals;
  /// A dictionary of math functions
  PyObject *m_math;
  /// The dictionary of the sys module
  PyObject *m_sys;
  /// Refresh protection
  int refresh_allowed;
};

#endif
