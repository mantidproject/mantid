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

// Python headers have to go first!
#include "MantidQtAPI/PythonSystemHeader.h"
#include "MantidQtAPI/PythonThreading.h"

#include "Script.h"
#include "MantidQtAPI/WorkspaceObserver.h"

#include <QFileInfo>
#include <QDir>

class ScriptingEnv;
class PythonScripting;
struct _sipWrapperType;

/**
 * This class holds, compiles and executes the Python code.
 */
class PythonScript : public Script, MantidQt::API::WorkspaceObserver {
  Q_OBJECT
public:
  /// Constructor
  PythonScript(PythonScripting *env, const QString &name,
               const InteractionType interact, QObject *context);
  /// Destructor
  ~PythonScript() override;

  /// Set the identifier of the script. If empty, set a default so that the code
  /// object behaves correctly
  void setIdentifier(const QString &name) override;

  /// Create a PyObject that wraps this C++ instance
  PyObject *createSipInstanceFromMe();

  // -------------------------- I/O-like behaviour ------------------
  /// Connects the python stdout to a Qt signal
  inline void write(const QString &text) { emit print(text); }
  /// Simulate file-like object (required for IPython)
  inline void flush() {}
  /// Simulate file-like object (required for colorama)
  inline bool closed() const { return false; }
  /// Simulate file-like object
  inline bool isatty() const { return false; }
  /// Is the given code complete
  bool compilesToCompleteStatement(const QString &code) const override;

  // -------------------------- Line number tracing ---------------------------
  /// Emits a signal from this object indicating the current line number of the
  /// code. This includes any offset.
  void lineNumberChanged(PyObject *codeObject, int lineNo);
  /// Emit the line change signal
  void sendLineChangeSignal(int lineNo, bool error);

  /// Create a list of keywords for the code completion API
  void generateAutoCompleteList() override;

  /// Special handle for syntax errors as they have no traceback
  QString constructSyntaxErrorStr(PyObject *syntaxError);
  /// Convert a traceback to a string
  void tracebackToMsg(QTextStream &msgStream, PyTracebackObject *traceback,
                      bool root = true);

  /// Set the name of the passed object so that Python can refer to it
  bool setQObject(QObject *val, const char *name) override;
  /// Set the name of the integer so that Python can refer to it
  bool setInt(int val, const char *name) override;
  /// Set the name of the double so that Python can refer to it
  bool setDouble(double val, const char *name) override;
  /// Set the context for this script
  void setContext(QObject *context) override;
  /// Resets the local dictionary to the defaults
  void clearLocals() override;

private:
  /// Helper class to ensure the sys.path variable is updated correctly
  struct PythonPathHolder {
    /// Update the path with the given entry
    explicit PythonPathHolder(const QString &entry) : m_path(entry) {
      const QFileInfo filePath(m_path);
      if (filePath.exists()) {
        QDir directory = filePath.absoluteDir();
        // Check it is not a package
        if (!QFileInfo(directory, "__init__.py").exists()) {
          m_path = directory.absolutePath();
          appendPath(m_path);
        } else {
          m_path = "";
        }
      }
    }
    /// Remove the entry from the path
    ~PythonPathHolder() {
      if (!m_path.isEmpty())
        removePath(m_path);
    }

    void appendPath(const QString &path) {
      ScopedPythonGIL pythonLock;
      QString code = "if r'%1' not in sys.path:\n"
                     "    sys.path.append(r'%1')";
      code = code.arg(path);
      PyRun_SimpleString(code.toAscii().constData());
    }
    void removePath(const QString &path) {
      ScopedPythonGIL pythonLock;
      QString code = "if r'%1' in sys.path:\n"
                     "    sys.path.remove(r'%1')";
      code = code.arg(path);
      PyRun_SimpleString(code.toAscii());
    }

  private:
    QString m_path;
  };

  inline PythonScripting *pythonEnv() const { return m_pythonEnv; }
  void initialize(const QString &name, QObject *context);
  void beginStdoutRedirect();
  void endStdoutRedirect();

  // --------------------------- Script compilation/execution
  // -----------------------------------
  /// Compile the code, returning true if it was successful, false otherwise
  bool compileImpl() override;
  /// Evaluate the current code and return a result as a QVariant
  QVariant evaluateImpl() override;
  /// Execute the current code and return a boolean indicating success/failure
  bool executeImpl() override;
  /// Request that this script be aborted
  void abortImpl() override;
  /// Get the value of the Python thread ID when a script is executed
  long getThreadID();

  /// Performs the call to Python from a string
  bool executeString();
  /// Executes the code object and returns the result, may be null
  PyObject *executeCompiledCode(PyObject *compiledCode);
  /// Check an object for a result.
  bool checkResult(PyObject *result);
  /// Compile to bytecode
  PyObject *compileToByteCode(bool for_eval = true);

  // ---------------------------- Variable reference
  // ---------------------------------------------
  /// Listen to add notifications from the ADS
  void addHandle(const std::string &wsName,
                 const Mantid::API::Workspace_sptr ws) override;
  /// Listen to add/replace notifications from the ADS
  void afterReplaceHandle(const std::string &wsName,
                          const Mantid::API::Workspace_sptr ws) override;
  /// Listen to delete notifications
  void postDeleteHandle(const std::string &wsName) override;
  /// Listen to ADS clear notifications
  void clearADSHandle() override;
  /// Add/update a Python reference to the given workspace
  void addPythonReference(const std::string &wsName,
                          const Mantid::API::Workspace_sptr ws);
  /// Delete a Python reference to the given workspace name
  void deletePythonReference(const std::string &wsName);

  /// Send out an error and clear it from python.
  void emit_error();

  PythonScripting *m_pythonEnv;
  PyObject *localDict, *stdoutSave, *stderrSave;
  PyObject *m_codeFileObject;
  long m_threadID; ///< Python thread id
  /// A reference to the IAlgorithm._algorithmInThread static method
  PyObject *m_algorithmInThread;
  bool isFunction;
  QString fileName;
  bool m_isInitialized;
  PythonPathHolder m_pathHolder;
  /// Set of current python variables that point to workspace handles
  std::set<std::string> m_workspaceHandles;
};

#endif
