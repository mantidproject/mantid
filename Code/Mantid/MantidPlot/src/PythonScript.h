/***************************************************************************
	File                 : PythonScript.h
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
#ifndef PYTHON_SCRIPT_H
#define PYTHON_SCRIPT_H

#include "PythonSystemHeader.h"
#include "Script.h"

class QObject;
class QString;
class ScriptingEnv;
class PythonScripting;

/**
 * This class holds, compiles and executes the Python code. 
 */
class PythonScript : public Script
{
  Q_OBJECT
  public:
  /// Constructor
  PythonScript(PythonScripting *env, const QString &code, QObject *context = 0, 
	       const QString &name="<input>", bool interactive = true, bool reportProgress = false);
  ///Destructor
  ~PythonScript();
  /// A function to connect to the ouput stream of the running Python code
  inline void write(const QString &text) 
  { 
    emit print(text); 
  }
  
  /// Emit a new line signal
  inline void broadcastNewLineNumber(int lineno)
  {
    emit currentLineChanged(getLineOffset() + lineno, true);
  }

 
public slots:
  /// Compile to bytecode
  bool compile(bool for_eval=true);
  /// Evaluate the current code
  QVariant eval();
  /// Excute the current code
  bool exec();
  /// Construct the error message from the stack trace (if one exists)
  QString constructErrorMsg();
  /// Set the name of the passed object so that Python can refer to it
  bool setQObject(QObject *val, const char *name);
  /// Set the name of the integer so that Python can refer to it
  bool setInt(int val, const char* name);
  /// Set the name of the double so that Python can refer to it
  bool setDouble(double val, const char* name);
  /// Set the context for this script
  void setContext(QObject *context);

private:
  /// A call-once init function
  void initialize();
  // Append or remove a path from the Python sys.path
  void updatePath(const QString & filename, bool append = true);
  /// Perform a call to the Python eval function with the necessary wrapping
  PyObject* executeScript(PyObject* return_tuple);  
  /// Create a list of keywords for the code completion API
  QStringList createAutoCompleteList() const;

private:
  PythonScripting* env() const; 
  void beginStdoutRedirect();
  void endStdoutRedirect();

  PyObject *PyCode, *localDict, *stdoutSave, *stderrSave;
  bool isFunction;
  QString fileName;
  bool m_isInitialized;
};

#endif
