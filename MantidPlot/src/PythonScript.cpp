/***************************************************************************
  File                 : PythonScript.cpp
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
// get rid of a compiler warning
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif

#include "PythonScript.h"
#include "MantidKernel/WarningSuppressions.h"
#include "PythonScripting.h"

#include "sipAPI_qti.h"

#include <stdexcept>

namespace {
// Avoids a compiler warning about implicit 'const char *'->'char*' conversion
// under clang
#define STR_LITERAL(str) const_cast<char *>(str)

/**
 * A callback, set using PyEval_SetTrace, that is called by Python
 * to allow inspection into the current execution frame. It is currently
 * used to emit the line number of the frame that is being executed.
 * @param scriptObj :: A reference to the object passed as the second argument
 * of PyEval_SetTrace
 * @param frame :: A reference to the current frame object
 * @param event :: An integer defining the event type, see
 * http://docs.python.org/c-api/init.html#profiling-and-tracing
 * @param arg :: Meaning varies depending on event type, see
 * http://docs.python.org/c-api/init.html#profiling-and-tracing
 */
int traceLineNumber(PyObject *scriptObj, PyFrameObject *frame, int event,
                    PyObject *arg) {
  Q_UNUSED(arg);
  int retcode(0);
  if (event != PyTrace_LINE)
    return retcode;
  PyObject_CallMethod(scriptObj, STR_LITERAL("lineNumberChanged"),
                      STR_LITERAL("O i"), frame->f_code->co_filename,
                      frame->f_lineno);
  return retcode;
}

/// Message to emit when everything worked out fine
static const QString MSG_FINISHED = "Script execution finished.";
/// Message to emit when starting
static const QString MSG_STARTED = "Script execution started.";
}

/**
 * Constructor
 */
PythonScript::PythonScript(PythonScripting *env, const QString &name,
                           const InteractionType interact, QObject *context)
    : Script(env, name, interact, context), m_interp(env), localDict(nullptr),
      stdoutSave(nullptr), stderrSave(nullptr), m_codeFileObject(nullptr),
      m_threadID(-1), isFunction(false), m_isInitialized(false),
      m_pathHolder(name), m_recursiveAsyncGIL() {
  initialize(name, context);
}

/**
 * Destructor
 */
PythonScript::~PythonScript() {
  ScopedPythonGIL lock;
  this->abort();
  observeAdd(false);
  observeAfterReplace(false);
  observePostDelete(false);
  observeADSClear(false);

  this->disconnect();
  Py_XDECREF(m_algorithmInThread);
  Py_XDECREF(localDict);
}

/**
 * Set the name of the script. If empty, set a default so that the code object
 * behaves correctly
 * w.r.t inspect.stack(). If no name is set inspect.stack throws a RangeError.
 * @param name An identifier for this object. Used in stack traces when
 * reporting errors
 */
void PythonScript::setIdentifier(const QString &name) {
  QString identifier = name;
  if (identifier.isEmpty())
    identifier = "New script";
  Script::setIdentifier(identifier);

  // Update or set the __file__ attribute
  if (QFileInfo(identifier).exists()) {
    QString scriptPath = QFileInfo(identifier).absoluteFilePath();
    // Make sure the __file__ variable is set
    PyDict_SetItem(localDict, FROM_CSTRING("__file__"),
                   FROM_CSTRING(scriptPath.toAscii().data()));
  }
}

/**
 * Creates a PyObject that wraps the calling C++ instance.
 * Ownership is transferred to the caller, i.e. the caller
 * is responsible for calling Py_DECREF
 * @return A PyObject wrapping this instance
 */
PyObject *PythonScript::createSipInstanceFromMe() {
  const sipTypeDef *sipClass = sipFindType("PythonScript");
  PyObject *sipWrapper = sipConvertFromType(this, sipClass, nullptr);
  assert(sipWrapper);
  return sipWrapper;
}

/**
 * @param code A lump of python code
 * @return True if the code forms a complete statment
 */
bool PythonScript::compilesToCompleteStatement(const QString &code) const {
  bool result(false);
  ScopedPythonGIL lock;
  PyObject *compiledCode = Py_CompileString(code.toAscii(), "", Py_file_input);
  if (PyObject *exception = PyErr_Occurred()) {
    // Certain exceptions still mean the code is complete
    result = (PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError) ||
              PyErr_GivenExceptionMatches(exception, PyExc_OverflowError) ||
              PyErr_GivenExceptionMatches(exception, PyExc_ValueError) ||
              PyErr_GivenExceptionMatches(exception, PyExc_TypeError) ||
              PyErr_GivenExceptionMatches(exception, PyExc_MemoryError));

    PyErr_Clear();
  } else {
    result = true;
  }
  Py_XDECREF(compiledCode);
  return result;
}

/**
 * Called from Python with the codeObject and line number of the currently
 * executing line
 * @param codeObject A pointer to the code object whose line is executing
 * @param lineNo The line number that the code is currently executing, note that
 * this will be relative to the top of the code that was executed
 */
void PythonScript::lineNumberChanged(PyObject *codeObject, int lineNo) {
  if (codeObject == m_codeFileObject) {
    sendLineChangeSignal(getRealLineNo(lineNo), false);
  }
}

/**
 * Emit the line change signal for the give line no
 * @param lineNo The line number to flag
 * @param error True if it is an error
 */
void PythonScript::sendLineChangeSignal(int lineNo, bool error) {
  emit currentLineChanged(lineNo, error);
}

/**
 * Create a list autocomplete keywords
 */
void PythonScript::generateAutoCompleteList() {
  ScopedPythonGIL lock;
  PyObject *keywords = PyObject_CallFunctionObjArgs(
      PyDict_GetItemString(m_interp->globalDict(),
                           "_ScopeInspector_GetFunctionAttributes"),
      localDict, NULL);
  if (PyErr_Occurred() || !keywords) {
    PyErr_Print();
    return;
  }
  QStringList keywordList = interp()->toStringList(keywords);
  Py_DECREF(keywords);
  emit autoCompleteListGenerated(keywordList);
}

/**
 * This emits the error signal and resets the error state
 * of the python interpreter.
 */
void PythonScript::emit_error() {
  // gil is necessary so other things don't continue
  ScopedPythonGIL lock;

  // return early if nothing happened
  if (!PyErr_Occurred()) {
    emit finished(MSG_FINISHED);
    return;
  }
  // get the error information out
  PyObject *exception(nullptr), *value(nullptr), *traceback(nullptr);
  PyErr_Fetch(&exception, &value, &traceback);

  // special check for system exceptions
  if (bool(exception) &&
      PyErr_GivenExceptionMatches(exception, PyExc_SystemExit) &&
      PyObject_HasAttrString(exception, "code")) {
    // value is the return code handed to sys.exit
    long code = 0;
    if (bool(value) && INT_CHECK(value)) {
      code = TO_LONG(value);
    }

    // if we are returning 0 then cleanup and return
    if (code == 0) {
      // maybe shouldn't clear the error, but for now this
      // is the agreed upon behavior
      PyErr_Clear();
      Py_XDECREF(traceback);
      Py_XDECREF(exception);
      Py_XDECREF(value);
      emit finished(MSG_FINISHED);
      return;
    }
  }

  // prework on the exception handling
  PyErr_NormalizeException(&exception, &value, &traceback);
  PyErr_Clear();

  // convert the traceback into something useful
  int lineNumber = 0;
  QString filename;
  if (traceback) {
    PyTracebackObject *tb = (PyTracebackObject *)traceback;
    lineNumber = tb->tb_lineno;
    filename = TO_CSTRING(tb->tb_frame->f_code->co_filename);
  }

  // the error message is the full (formated) traceback
  PyObject *str_repr = PyObject_Str(value);
  QString message;
  QTextStream msgStream(&message);
  if (value && str_repr) {
    if (exception == PyExc_SyntaxError) {
      msgStream << constructSyntaxErrorStr(value);
    } else {
      QString excTypeName(
          value->ob_type
              ->tp_name); // This is fully qualified with the module name
      excTypeName = excTypeName.section(".", -1);
      msgStream << excTypeName << ": " << TO_CSTRING(str_repr);
    }

  } else {
    msgStream << "Unknown exception has occurred.";
  }
  tracebackToMsg(msgStream, (PyTracebackObject *)(traceback));
  msgStream << "\n";

  Py_XDECREF(traceback);
  Py_XDECREF(exception);
  Py_XDECREF(value);

  emit error(msgStream.readAll(), filename, lineNumber);
}

/**
 * Constructs a string error message from a syntax exception value object
 * @param syntaxError An instance of PyExc_SyntaxError
 */
QString PythonScript::constructSyntaxErrorStr(PyObject *syntaxError) {
  QString exceptionAsStr = m_interp->toString(syntaxError);
  exceptionAsStr = exceptionAsStr.section("(", 0, 0).trimmed();
  const QString filename =
      m_interp->toString(PyObject_GetAttrString(syntaxError, "filename"), true);
  int lineno = static_cast<int>(
      m_interp->toLong(PyObject_GetAttrString(syntaxError, "lineno")));
#if PY_VERSION_HEX < 0x02070000
  // Syntax errors generated by earlier versions seem to have a line number off
  // by 1
  lineno -= 1;
#endif

  QString msg;
  // If the text attribute is not None then an offset in the code can be shown
  // using a ^ character
  PyObject *textObject = PyObject_GetAttrString(syntaxError, "text");
  if (textObject != Py_None) {
    QString text = m_interp->toString(textObject, true).trimmed();
    int offset = static_cast<int>(
        m_interp->toLong(PyObject_GetAttrString(syntaxError, "offset")));
    QString offsetMarker = QString(offset - 1, ' ') + "^";
    msg = "File \"%1\", line %2\n"
          "    %3\n"
          "    %4\n"
          "SyntaxError: %5";
    msg = msg.arg(filename);
    msg = msg.arg(lineno);
    msg = msg.arg(text, offsetMarker, exceptionAsStr);
  } else {
    msg = "File \"%1\", line %2\n"
          "SyntaxError: %3";
    msg = msg.arg(filename);
    msg = msg.arg(lineno);
    msg = msg.arg(exceptionAsStr);
  }
  if (filename == identifier().c_str()) {
    sendLineChangeSignal(lineno, true);
  }
  return msg;
}

/**
 * Form a traceback
 * @param msg The reference to the textstream to accumulate the message
 * @param traceback A traceback object
 * @param root If true then this is the root of the traceback
 */
void PythonScript::tracebackToMsg(QTextStream &msgStream,
                                  PyTracebackObject *traceback, bool root) {
  if (traceback == nullptr)
    return;
  msgStream << "\n  ";
  if (root)
    msgStream << "at";
  else
    msgStream << "caused by";

  int lineno = traceback->tb_lineno;
  QString filename =
      QString::fromAscii(TO_CSTRING(traceback->tb_frame->f_code->co_filename));
  if (filename == identifier().c_str()) {
    lineno = getRealLineNo(lineno);
    sendLineChangeSignal(lineno, true);
  }

  msgStream << " line " << lineno << " in \'" << filename << "\'";
  tracebackToMsg(msgStream, traceback->tb_next, false);
}

bool PythonScript::setQObject(QObject *val, const char *name) {
  if (localDict) {
    return interp()->setQObject(val, name, localDict);
  } else
    return false;
}

bool PythonScript::setInt(int val, const char *name) {
  return interp()->setInt(val, name, localDict);
}

bool PythonScript::setDouble(double val, const char *name) {
  return interp()->setDouble(val, name, localDict);
}

void PythonScript::setContext(QObject *context) {
  Script::setContext(context);
  setQObject(context, "self");
}

/**
 * Clears the current set of local variables, if they exist, and resets
 * the dictionary context back to the default set
 */
void PythonScript::clearLocals() {
  ScopedPythonGIL lock;

  PyObject *mainModule = PyImport_AddModule("__main__");
  PyObject *cleanLocals = PyDict_Copy(PyModule_GetDict(mainModule));

  if (localDict) {
    // Pull out variables that are not user-related
    PyObject *value = PyDict_GetItemString(localDict, "__file__");
    if (value)
      PyDict_SetItemString(cleanLocals, "__file__", value);
    // reset locals
    Py_DECREF(localDict);
    localDict = nullptr;
  }
  localDict = cleanLocals;
}

/**
 * Sets the context for the script and if name points to a file then
 * sets the __file__ variable
 * @param name A string identifier for the script
 * @param context A QObject defining the context
 */
void PythonScript::initialize(const QString &name, QObject *context) {
  clearLocals(); // holds and releases GIL

  ScopedPythonGIL lock;
  PythonScript::setIdentifier(name);
  setContext(context);

  PyObject *ialgorithm =
      PyObject_GetAttrString(PyImport_AddModule("mantid.api"), "IAlgorithm");
  m_algorithmInThread =
      PyObject_GetAttrString(ialgorithm, "_algorithmInThread");
  Py_INCREF(m_algorithmInThread);
}

//-------------------------------------------------------
// Private
//-------------------------------------------------------
/**
 * Redirect the std out to this object
 */
void PythonScript::beginStdoutRedirect() {
  if (!redirectStdOut())
    return;
  stdoutSave = PyDict_GetItemString(interp()->sysDict(), "stdout");
  Py_XINCREF(stdoutSave);
  stderrSave = PyDict_GetItemString(interp()->sysDict(), "stderr");
  Py_XINCREF(stderrSave);
  interp()->setQObject(this, "stdout", interp()->sysDict());
  interp()->setQObject(this, "stderr", interp()->sysDict());
}

/**
 * Restore the std out to this object to what it was before the last call to
 * beginStdouRedirect
 */
void PythonScript::endStdoutRedirect() {
  if (!redirectStdOut())
    return;

  PyDict_SetItemString(interp()->sysDict(), "stdout", stdoutSave);
  Py_XDECREF(stdoutSave);
  PyDict_SetItemString(interp()->sysDict(), "stderr", stderrSave);
  Py_XDECREF(stderrSave);
}

/**
 * To be called from the main thread before an async call that is
 * recursive. See ApplicationWindow::runPythonScript
 * @return True if the lock was released by this call, false otherwise
 */
bool PythonScript::recursiveAsyncSetup() {
  if (PythonGIL::locked()) {
    m_recursiveAsyncGIL.release();
    return true;
  }
  return false;
}

/**
 * To be called from the main thread immediately after an async call
 * that is recursive. See ApplicationWindow::runPythonScript
 * @param relock If true then relock the GIL on this thread. This should use the
 * value returned by recursiveAsyncSetup.
 */
void PythonScript::recursiveAsyncTeardown(bool relock) {
  if (relock) {
    m_recursiveAsyncGIL.acquire();
  }
}

/**
 * Compile the code returning true if successful, false otherwise
 * @param code
 * @return True if success, false otherwise
 */
bool PythonScript::compileImpl() {
  PyObject *codeObject = compileToByteCode(false);

  return codeObject != nullptr;
}

/**
 * Evaluate the code and return the value
 * @return
 */
QVariant PythonScript::evaluateImpl() {
  ScopedPythonGIL lock;
  PyObject *compiledCode = this->compileToByteCode(true);
  if (!compiledCode) {
    return QVariant("");
  }
  PyObject *pyret;
  beginStdoutRedirect();
  if (PyCallable_Check(compiledCode)) {
    PyObject *empty_tuple = PyTuple_New(0);
    pyret = PyObject_Call(compiledCode, empty_tuple, localDict);
    Py_DECREF(empty_tuple);
  } else {
    pyret = PyEval_EvalCode(CODE_OBJECT(compiledCode), localDict, localDict);
  }
  endStdoutRedirect();
  if (!pyret) {
    if (PyErr_ExceptionMatches(PyExc_ValueError) ||
        PyErr_ExceptionMatches(PyExc_ZeroDivisionError)) {
      PyErr_Clear(); // silently ignore errors
      return QVariant("");
    } else {
      emit_error();
      return QVariant();
    }
  }

  QVariant qret = QVariant();
  /* None */
  if (pyret == Py_None) {
    qret = QVariant("");
  }
  /* numeric types */
  else if (PyFloat_Check(pyret)) {
    qret = QVariant(PyFloat_AS_DOUBLE(pyret));
  } else if (INT_CHECK(pyret)) {
    qret = QVariant((qlonglong)TO_LONG(pyret));
  }
#if !defined(IS_PY3K)
  else if (PyLong_Check(pyret)) {
    qret = QVariant((qlonglong)PyLong_AsLongLong(pyret));
  }
#endif
  else if (PyNumber_Check(pyret)) {
    PyObject *number = PyNumber_Float(pyret);
    if (number) {
      qret = QVariant(PyFloat_AS_DOUBLE(number));
      Py_DECREF(number);
    }
  }
  /* bool */
  DIAG_OFF("parentheses-equality")
  else if (PyBool_Check(pyret)) {
  DIAG_ON("parentheses-equality")
    qret = QVariant(pyret == Py_True);
  }
  // could handle advanced types (such as PyList->QValueList) here if needed
  /* fallback: try to convert to (unicode) string */
  if (!qret.isValid()) {
#if defined(IS_PY3K)
    // In 3 everything is unicode
    PyObject *pystring = PyObject_Str(pyret);
    if (pystring) {
      qret = QVariant(QString::fromUtf8(_PyUnicode_AsString(pystring)));
    }
#else
    PyObject *pystring = PyObject_Unicode(pyret);
    if (pystring) {
      PyObject *asUTF8 =
          PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(pystring),
                               (int)PyUnicode_GET_DATA_SIZE(pystring), nullptr);
      Py_DECREF(pystring);
      if (asUTF8) {
        qret = QVariant(QString::fromUtf8(PyString_AS_STRING(asUTF8)));
        Py_DECREF(asUTF8);
      } else if ((pystring = PyObject_Str(pyret))) {
        qret = QVariant(QString(PyString_AS_STRING(pystring)));
        Py_DECREF(pystring);
      }
    }
#endif
  }
  Py_DECREF(pyret);
  if (PyErr_Occurred()) {
    if (PyErr_ExceptionMatches(PyExc_ValueError) ||
        PyErr_ExceptionMatches(PyExc_ZeroDivisionError)) {
      PyErr_Clear(); // silently ignore errors
      return QVariant("");
    } else {
      emit_error();
    }
    return QVariant();
  }
  return qret;
}

/**
 * On construction set a reference to a given value and at destruction reset
 * it
 * to its original
 */
struct TemporaryValue {
  TemporaryValue(long &refVal, long tmp) : initial(refVal), ref(refVal) {
    ref = tmp;
  }
  ~TemporaryValue() { ref = initial; }

private:
  long initial;
  long &ref;
};

bool PythonScript::executeImpl() {
  TemporaryValue holder(m_threadID, getThreadID());
  return executeString();
}

void PythonScript::abortImpl() {
  // The current thread for this script could be
  // in one of two states:
  //   1. A C++ algorithm is being executed so it must be
  //      interrupted using algorithm.cancel()
  //   2. Pure Python is executing and can be interrupted
  //      with a KeyboardInterrupt exception
  // In both situations we issue a KeyboardInterrupt just in case the
  // algorithm
  // hasn't implemented cancel() checking so that when control returns the
  // Python the
  // interrupt should be picked up.
  ScopedPythonGIL lock;
  m_interp->raiseAsyncException(m_threadID, PyExc_KeyboardInterrupt);
  PyObject *curAlg =
      PyObject_CallFunction(m_algorithmInThread, STR_LITERAL("l"), m_threadID);
  if (curAlg && curAlg != Py_None) {
    PyObject_CallMethod(curAlg, STR_LITERAL("cancel"), STR_LITERAL(""));
  }
}

/**
 * The value returned here is only valid when called by a valid
 * thread that has a valid Python threadstate
 * @return A long int giving a unique ID for the thread
 */
long PythonScript::getThreadID() {
  ScopedPythonGIL lock;
  return PyThreadState_Get()->thread_id;
}

/// Performs the call to Python
bool PythonScript::executeString() {
  emit started(MSG_STARTED);
  bool success(false);
  ScopedPythonGIL lock;

  PyObject *compiledCode = compileToByteCode(false);
  PyObject *result(nullptr);
  if (compiledCode) {
    result = executeCompiledCode(compiledCode);
  }
  // If an error has occurred we need to construct the error message
  // before any other python code is run
  if (!result) {
    emit_error();
    // If a script was aborted we both raise a KeyboardInterrupt and
    // call Algorithm::cancel to make sure we capture it. The doubling
    // can leave an interrupt in the pipeline so we clear it was we've
    // got the error info out
    m_interp->raiseAsyncException(m_threadID, nullptr);
  } else {
    emit finished(MSG_FINISHED);
    success = true;
    if (isInteractive()) {
      generateAutoCompleteList();
    }
  }

  Py_XDECREF(compiledCode);
  Py_XDECREF(result);

  return success;
}

namespace {
/**
 * RAII struct for installing a tracing function
 * to monitor the line number events and ensuring it is removed
 * when the object is destroyed
 */
struct InstallTrace {
  explicit InstallTrace(PythonScript &scriptObject)
      : m_sipWrappedScript(nullptr) {
    if (scriptObject.reportProgress()) {
      m_sipWrappedScript = scriptObject.createSipInstanceFromMe();
      PyEval_SetTrace((Py_tracefunc)&traceLineNumber, m_sipWrappedScript);
    }
  }
  ~InstallTrace() {
    PyEval_SetTrace(nullptr, nullptr);
    Py_XDECREF(m_sipWrappedScript);
  }

private:
  InstallTrace();
  Q_DISABLE_COPY(InstallTrace)
  PyObject *m_sipWrappedScript;
};
}

/**
 * Executes the compiled code object. If NULL nothing happens
 * @param compiledCode An object that has been compiled from a code fragment
 * @return The result python object
 */
PyObject *PythonScript::executeCompiledCode(PyObject *compiledCode) {
  PyObject *result(nullptr);
  if (!compiledCode)
    return result;

  InstallTrace traceInstall(*this);
  beginStdoutRedirect();
  result = PyEval_EvalCode(CODE_OBJECT(compiledCode), localDict, localDict);
  endStdoutRedirect();
  return result;
}

/**
 * Check an object for a result.  A valid pointer indicates success
 * @param result The output from a PyEval call
 * @return A boolean indicating success status
 */
bool PythonScript::checkResult(PyObject *result) { return result != nullptr; }

/**
 * Compile the code
 */
PyObject *PythonScript::compileToByteCode(bool for_eval) {
  // Support for the convenient col() and cell() functions.
  // This can't be done anywhere else, because we need access to the local
  // variables self, i and j.
  if (context()) {
    if (context()->inherits("Table")) {
      // A bit of a hack, but we need either IndexError or len() from
      // __builtins__.
      PyDict_SetItemString(
          localDict, "__builtins__",
          PyDict_GetItemString(interp()->globalDict(), "__builtins__"));
      PyObject *ret = PyRun_String(
          "def col(c,*arg):\n"
          "\ttry: return self.cell(c,arg[0])\n"
          "\texcept(IndexError): return self.cell(c,i)\n"
          "def cell(c,r):\n"
          "\treturn self.cell(c,r)\n"
          "def tablecol(t,c):\n"
          "\treturn self.folder().rootFolder().table(t,True).cell(c,i)\n"
          "def _meth_table_col_(t,c):\n"
          "\treturn t.cell(c,i)\n"
          "self.__class__.col = _meth_table_col_",
          Py_file_input, localDict, localDict);
      if (ret)
        Py_DECREF(ret);
      else
        PyErr_Print();
    } else if (context()->inherits("Matrix")) {
      // A bit of a hack, but we need either IndexError or len() from
      // __builtins__.
      PyDict_SetItemString(
          localDict, "__builtins__",
          PyDict_GetItemString(interp()->globalDict(), "__builtins__"));
      PyObject *ret =
          PyRun_String("def cell(*arg):\n"
                       "\ttry: return self.cell(arg[0],arg[1])\n"
                       "\texcept(IndexError): return self.cell(i,j)\n",
                       Py_file_input, localDict, localDict);
      if (ret)
        Py_DECREF(ret);
      else
        PyErr_Print();
    }
  }
  bool success(false);
  // Simplest case: Code is a single expression
  PyObject *compiledCode = Py_CompileString(
      codeString().c_str(), identifier().c_str(), Py_file_input);

  if (compiledCode) {
    success = true;
  } else if (for_eval) {
    // Code contains statements (or errors) and we want to get a return
    // value from it.
    // So we wrap the code into a function definition,
    // execute that (as Py_file_input) and store the function object in
    // PyCode.
    // See http://mail.python.org/pipermail/python-list/2001-June/046940.html
    // for why there isn't an easier way to do this in Python.
    PyErr_Clear(); // silently ignore errors
    PyObject *key(nullptr), *value(nullptr);
    Py_ssize_t i(0);
    QString signature = "";
    while (PyDict_Next(localDict, &i, &key, &value)) {
      signature.append(TO_CSTRING(key)).append(",");
    }
    signature.truncate(signature.length() - 1);
    std::string fdef = "def __doit__(" + signature.toStdString() + "):\n";
    fdef += codeString();
    compiledCode =
        Py_CompileString(fdef.c_str(), identifier().c_str(), Py_file_input);
    if (compiledCode) {
      PyObject *tmp = PyDict_New();
      Py_XDECREF(PyEval_EvalCode(CODE_OBJECT(compiledCode), localDict, tmp));
      Py_DECREF(compiledCode);
      compiledCode = PyDict_GetItemString(tmp, "__doit__");
      Py_XINCREF(compiledCode);
      Py_DECREF(tmp);
    }
    success = (compiledCode != nullptr);
  } else {
  }

  if (success) {
    m_codeFileObject = ((PyCodeObject *)(compiledCode))->co_filename;
  } else {
    compiledCode = nullptr;
    m_codeFileObject = nullptr;
  }
  return compiledCode;
}
