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
#include "PythonThreading.h"
#include "Script.h"
#include "MantidQtAPI/WorkspaceObserver.h"

#include <QFileInfo>

class ScriptingEnv;
class PythonScripting;

/**
 * This class holds, compiles and executes the Python code. 
 */
class PythonScript : public Script, MantidQt::API::WorkspaceObserver
{
  Q_OBJECT
public:
  /// Constructor
  PythonScript(PythonScripting *env, const QString &name, const InteractionType interact,
               QObject * context);
  /// Destructor
  ~PythonScript();

  // -------------------------- Print/error message handling ------------------
  /// Connects the python stdout to a Qt signal
  inline void write(const QString &text) 
  { 
    emit print(text); 
  }
  /// 'Fake' method needed for IPython import
  inline void flush()
  {}
  /// Is the given code complete
  bool compilesToCompleteStatement(const QString & code) const;
  /// Is the script being executed
  bool isExecuting() const;

public slots:
  /// Compile the code, returning true if it was successful, false otherwise
  bool compile(const QString & code);
  /// Evaluate the current code and return a result as a QVariant
  QVariant evaluate(const QString & code);
  /// Execute the current code and return a boolean indicating success/failure
  bool execute(const QString &);
  /// Execute the code asynchronously, returning immediately after the execution has started
  QFuture<bool> executeAsync(const QString & code);

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
  /// Helper class to ensure the sys.path variable is updated correctly
  struct PythonPathHolder
  {
    /// Update the path with the given entry
    PythonPathHolder(const QString & entry)
      : m_path(entry)
    {
      if( QFileInfo(m_path).exists() )
      {
        appendPath(m_path);
      }
    }
    /// Remove the entry from the path
    ~PythonPathHolder()
    {
      removePath(m_path);
    }
    void appendPath(const QString & path)
    {
      GlobalInterpreterLock pythonLock;
      QString code =
        "if r'%1' not in sys.path:\n"
        "    sys.path.append(r'%1')";
      code = code.arg(path);
      PyRun_SimpleString(code);
    }
    void removePath(const QString & path)
    {
      GlobalInterpreterLock pythonLock;
      QString code =
        "if r'%1' in sys.path:\n"
        "    sys.path.remove(r'%1')";
      code = code.arg(path);
      PyRun_SimpleString(code.toAscii());
    }
  private:
    const QString m_path;
  };

  inline PythonScripting * pythonEnv() const { return m_pythonEnv; }
  void initialize(const QString & name, QObject *context);
  void beginStdoutRedirect();
  void endStdoutRedirect();

  // --------------------------- Script compilation/execution  -----------------------------------
  /// Performs the call to Python
  bool doExecution(const QString & code);

  /// Compile to bytecode
  PyObject * compileToByteCode(const QString &, bool for_eval=true);
  /// Create a list of keywords for the code completion API
  void generateAutoCompleteList();

  // ---------------------------- Variable reference ---------------------------------------------
  /// Listen to add notifications from the ADS
  void addHandle(const std::string& wsName, const Mantid::API::Workspace_sptr ws);
  /// Listen to add/replace notifications from the ADS
  void afterReplaceHandle(const std::string& wsName,const Mantid::API::Workspace_sptr ws);
  /// Listen to delete notifications
  void postDeleteHandle(const std::string& wsName);
  /// Listen to ADS clear notifications
  void clearADSHandle();
  /// Add/update a Python reference to the given workspace
  void addPythonReference(const std::string& wsName,const Mantid::API::Workspace_sptr ws);
  /// Delete a Python reference to the given workspace name
  void deletePythonReference(const std::string& wsName);


  PythonScripting * m_pythonEnv;
  PyObject *localDict, *stdoutSave, *stderrSave;
  bool isFunction;
  QString fileName;
  bool m_isInitialized;
  PythonPathHolder m_pathHolder;
  /// Set of current python variables that point to worksapce handles
  std::set<std::string> m_workspaceHandles;
};

#endif
