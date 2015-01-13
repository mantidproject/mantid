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
#include "PythonScripting.h"

#include "sipAPI_qti.h"

#include <QVariant>
#include <QMessageBox>

#include <iostream>
#include <stdexcept>


namespace
{
  /**
   * A callback, set using PyEval_SetTrace, that is called by Python
   * to allow inspection into the current execution frame. It is currently
   * used to emit the line number of the frame that is being executed.
   * @param scriptObj :: A reference to the object passed as the second argument of PyEval_SetTrace
   * @param frame :: A reference to the current frame object
   * @param event :: An integer defining the event type, see http://docs.python.org/c-api/init.html#profiling-and-tracing
   * @param arg :: Meaning varies depending on event type, see http://docs.python.org/c-api/init.html#profiling-and-tracing
   */
  int traceLineNumber(PyObject *scriptObj, PyFrameObject *frame, int event, PyObject *arg)
  {
    Q_UNUSED(arg);
    int retcode(0);
    if(event != PyTrace_LINE) return retcode;
      std::string str1 = "lineNumberChanged";
      std::string str2 = "O i";
    PyObject_CallMethod(scriptObj,&str1[0],&str2[0], frame->f_code->co_filename, frame->f_lineno);
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
PythonScript::PythonScript(PythonScripting *env, const QString &name, const InteractionType interact,
                           QObject * context)
  : Script(env, name, interact, context), m_pythonEnv(env), localDict(NULL),
    stdoutSave(NULL), stderrSave(NULL), m_CodeFileObject(NULL), isFunction(false), m_isInitialized(false),
    m_pathHolder(name), m_workspaceHandles()
{
  initialize(name, context);
}

/**
 * Destructor
 */
PythonScript::~PythonScript()
{
  GlobalInterpreterLock pythonLock;
  observeAdd(false);
  observeAfterReplace(false);
  observePostDelete(false);
  observeADSClear(false);

  this->disconnect();
  Py_XDECREF(localDict);
}

/**
 * Set the name of the script. If empty, set a default so that the code object behaves correctly
 * w.r.t inspect.stack(). If no name is set inspect.stack throws a RangeError.
 * @param name An identifier for this object. Used in stack traces when reporting errors
 */
void PythonScript::setIdentifier(const QString & name)
{
  QString identifier = name;
  if(identifier.isEmpty()) identifier = "New script";
  Script::setIdentifier(identifier);

  // Update or set the __file__ attribute
  if( QFileInfo(identifier).exists() )
  {
    QString scriptPath = QFileInfo(identifier).absoluteFilePath();
    // Make sure the __file__ variable is set
    PyDict_SetItem(localDict,PyString_FromString("__file__"), PyString_FromString(scriptPath.toAscii().data()));
  }
}


/**
 * Creates a PyObject that wraps the calling C++ instance.
 * Ownership is transferred to the caller, i.e. the caller
 * is responsible for calling Py_DECREF
 * @return A PyObject wrapping this instance
 */
PyObject * PythonScript::createSipInstanceFromMe()
{
  static sipWrapperType *sipClass(NULL);
  if(!sipClass)
  {
    sipClass = sipFindClass("PythonScript");
  }
  PyObject *sipWrapper = sipConvertFromInstance(this, sipClass, NULL);
  assert(sipWrapper);
  return sipWrapper;
}

/**
 * @param code A lump of python code
 * @return True if the code forms a complete statment
 */
bool PythonScript::compilesToCompleteStatement(const QString & code) const
{
  bool result(false);
  GlobalInterpreterLock gil;
  PyObject *compiledCode = Py_CompileString(code.toAscii(), "", Py_file_input);
  if( PyObject *exception = PyErr_Occurred() )
  {
    // Certain exceptions still mean the code is complete
    if(PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError) ||
       PyErr_GivenExceptionMatches(exception, PyExc_OverflowError) ||
       PyErr_GivenExceptionMatches(exception, PyExc_ValueError) ||
       PyErr_GivenExceptionMatches(exception, PyExc_TypeError) ||
       PyErr_GivenExceptionMatches(exception, PyExc_MemoryError))
    {
      result = true;
    }
    else
    {
      result = false;
    }
    PyErr_Clear();
  }
  else
  {
    result = true;
  }
  Py_XDECREF(compiledCode);
  return result;
}

/**
 * Called from Python with the codeObject and line number of the currently executing line
 * @param codeObject A pointer to the code object whose line is executing
 * @param lineNo The line number that the code is currently executing, note that
 * this will be relative to the top of the code that was executed
 */
void PythonScript::lineNumberChanged(PyObject *codeObject, int lineNo)
{
  if(codeObject == m_CodeFileObject)
  {
    sendLineChangeSignal(getRealLineNo(lineNo), false);
  }
}

/**
 * Emit the line change signal for the give line no
 * @param lineNo The line number to flag
 * @param error True if it is an error
 */
void PythonScript::sendLineChangeSignal(int lineNo, bool error)
{
  emit currentLineChanged(lineNo, error);
}


/**
 * Create a list autocomplete keywords
 */
void PythonScript::generateAutoCompleteList()
{
  GlobalInterpreterLock gil;

  PyObject *main_module = PyImport_AddModule("__main__");
  PyObject *method = PyString_FromString("_ScopeInspector_GetFunctionAttributes");
  PyObject *keywords(NULL);
  if( method && main_module )
  {
    keywords = PyObject_CallMethodObjArgs(main_module, method, localDict, NULL);
  }
  else
  {
    return;
  }
  QStringList keywordList;
  if(PyErr_Occurred() || !keywords)
  {
    PyErr_Print();
    return;
  }

  keywordList = pythonEnv()->toStringList(keywords);
  Py_DECREF(keywords);
  Py_DECREF(method);
  emit autoCompleteListGenerated(keywordList);
}

/**
 * This emits the error signal and resets the error state
 * of the python interpreter.
 */
void PythonScript::emit_error()
{
  // gil is necessary so other things don't continue
  GlobalInterpreterLock gil;

  // return early if nothing happened
  if (!PyErr_Occurred())
  {
    emit finished(MSG_FINISHED);
    return;
  }
  // get the error information out
  PyObject *exception(NULL), *value(NULL), *traceback(NULL);
  PyErr_Fetch(&exception, &value, &traceback);

  // special check for system exceptions
  if (bool(exception)
      && PyErr_GivenExceptionMatches(exception, PyExc_SystemExit)
      && PyObject_HasAttrString(exception, "code"))
  {
    // value is the return code handed to sys.exit
    long code = 0;
    if (bool(value) && PyInt_Check(value))
    {
      code = PyInt_AsLong(value);
    }

    // if we are returning 0 then cleanup and return
    if (code == 0)
    {
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
  if (traceback)
  {
    PyTracebackObject* tb = (PyTracebackObject*)traceback;
    lineNumber = tb->tb_lineno;
    filename = PyString_AsString(tb->tb_frame->f_code->co_filename);
  }

  // the error message is the full (formated) traceback
  PyObject *str_repr = PyObject_Str(value);
  QString message;
  QTextStream msgStream(&message);
  if( value && str_repr )
  {
    if(exception == PyExc_SyntaxError)
    {
      msgStream << constructSyntaxErrorStr(value);
    }
    else
    {
      QString excTypeName(value->ob_type->tp_name); // This is fully qualified with the module name
      excTypeName = excTypeName.section(".", -1);
      msgStream << excTypeName << ": " << PyString_AsString(str_repr);
    }

  }
  else
  {
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
QString PythonScript::constructSyntaxErrorStr(PyObject *syntaxError)
{
  QString exceptionAsStr = m_pythonEnv->toString(syntaxError);
  exceptionAsStr = exceptionAsStr.section("(", 0, 0).trimmed();
  const QString filename = m_pythonEnv->toString(PyObject_GetAttrString(syntaxError, "filename"), true);
  int lineno = static_cast<int>(m_pythonEnv->toLong(PyObject_GetAttrString(syntaxError, "lineno")));
#if PY_VERSION_HEX < 0x02070000
  // Syntax errors generated by earlier versions seem to have a line number off by 1
  lineno -= 1;
#endif

  QString msg;
  // If the text attribute is not None then an offset in the code can be shown using a ^ character
  PyObject *textObject = PyObject_GetAttrString(syntaxError, "text");
  if(textObject != Py_None)
  {
    QString text = m_pythonEnv->toString(textObject, true).trimmed();
    int offset = static_cast<int>(m_pythonEnv->toLong(PyObject_GetAttrString(syntaxError, "offset")));
    QString offsetMarker = QString(offset-1, ' ') + "^";
    msg = 
      "File \"%1\", line %2\n"
      "    %3\n"
      "    %4\n"
      "SyntaxError: %5";
    msg = msg.arg(filename);
    msg = msg.arg(lineno);
    msg = msg.arg(text, offsetMarker, exceptionAsStr);
  }
  else
  {
    msg = 
      "File \"%1\", line %2\n"
      "SyntaxError: %3";
    msg = msg.arg(filename);
    msg = msg.arg(lineno);
    msg = msg.arg(exceptionAsStr);
  }
  if(filename == identifier().c_str())
  {
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
                                  PyTracebackObject* traceback, 
                                  bool root)
{
  if(traceback == NULL) return;
  msgStream << "\n  ";
  if (root) msgStream << "at";
  else msgStream << "caused by";
  
  int lineno = traceback->tb_lineno;
  QString filename = QString::fromAscii(PyString_AsString(traceback->tb_frame->f_code->co_filename));
  if(filename == identifier().c_str())
  {
    lineno = getRealLineNo(lineno);
    sendLineChangeSignal(lineno, true);

  }
  
  msgStream << " line " << lineno << " in \'" << filename  << "\'";
  tracebackToMsg(msgStream, traceback->tb_next, false);
}

bool PythonScript::setQObject(QObject *val, const char *name)
{
  if (localDict)
  {
    return pythonEnv()->setQObject(val, name, localDict);
  }
  else
    return false;
}

bool PythonScript::setInt(int val, const char *name)
{
  return pythonEnv()->setInt(val, name, localDict);
}

bool PythonScript::setDouble(double val, const char *name)
{
  return pythonEnv()->setDouble(val, name, localDict);
}

void PythonScript::setContext(QObject *context)
{
  Script::setContext(context);
  setQObject(context, "self");
}

/**
 * Clears the current set of local variables, if they exist, and resets
 * the dictionary context back to the default set
 */
void PythonScript::clearLocals()
{
  GlobalInterpreterLock pythonLock;

  PyObject *mainModule = PyImport_AddModule("__main__");
  PyObject *cleanLocals = PyDict_Copy(PyModule_GetDict(mainModule));

  if(localDict)
  {
    // Pull out variables that are not user-related
    PyObject * value = PyDict_GetItemString(localDict, "__file__");
    if(value) PyDict_SetItemString(cleanLocals, "__file__", value);
    // reset locals
    Py_DECREF(localDict);
    localDict = NULL;
  }
  localDict = cleanLocals;
}

/**
 * Sets the context for the script and if name points to a file then
 * sets the __file__ variable
 * @param name A string identifier for the script
 * @param context A QObject defining the context
 */
void PythonScript::initialize(const QString & name, QObject *context)
{
  clearLocals(); // holds and releases GIL

  GlobalInterpreterLock pythonlock;
  PythonScript::setIdentifier(name);
  setContext(context);
}


//-------------------------------------------------------
// Private
//-------------------------------------------------------
/**
 * Redirect the std out to this object
 */
void PythonScript::beginStdoutRedirect()
{
  if(!redirectStdOut()) return;
  stdoutSave = PyDict_GetItemString(pythonEnv()->sysDict(), "stdout");
  Py_XINCREF(stdoutSave);
  stderrSave = PyDict_GetItemString(pythonEnv()->sysDict(), "stderr");
  Py_XINCREF(stderrSave);
  pythonEnv()->setQObject(this, "stdout", pythonEnv()->sysDict());
  pythonEnv()->setQObject(this, "stderr", pythonEnv()->sysDict());
}

/**
 * Restore the std out to this object to what it was before the last call to
 * beginStdouRedirect
 */
void PythonScript::endStdoutRedirect()
{
  if(!redirectStdOut()) return;

  PyDict_SetItemString(pythonEnv()->sysDict(), "stdout", stdoutSave);
  Py_XDECREF(stdoutSave);
  PyDict_SetItemString(pythonEnv()->sysDict(), "stderr", stderrSave);
  Py_XDECREF(stderrSave);
}

/**
 * Compile the code returning true if successful, false otherwise
 * @param code
 * @return True if success, false otherwise
 */
bool PythonScript::compileImpl()
{
  PyObject *codeObject = compileToByteCode(false);
  if(codeObject)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Evaluate the code and return the value
 * @return
 */
QVariant PythonScript::evaluateImpl()
{
  GlobalInterpreterLock gil;
  PyObject *compiledCode = this->compileToByteCode(true);
  if(!compiledCode)
  {
    return QVariant("");
  }
  PyObject *pyret;
  beginStdoutRedirect();
  if (PyCallable_Check(compiledCode)){
    PyObject *empty_tuple = PyTuple_New(0);
    pyret = PyObject_Call(compiledCode, empty_tuple, localDict);
    Py_DECREF(empty_tuple);
  }
  else
  {
    pyret = PyEval_EvalCode((PyCodeObject*)compiledCode, localDict, localDict);
  }
  endStdoutRedirect();
  if (!pyret)
  {
    if (PyErr_ExceptionMatches(PyExc_ValueError) || PyErr_ExceptionMatches(PyExc_ZeroDivisionError))
    {
      PyErr_Clear(); // silently ignore errors
      return QVariant("");
    }
    else
    {
      emit_error();
      return QVariant();
    }
  }

  QVariant qret = QVariant();
  /* None */
  if (pyret == Py_None)
  {
    qret = QVariant("");
  }
  /* numeric types */
  else if (PyFloat_Check(pyret))
  {
    qret = QVariant(PyFloat_AS_DOUBLE(pyret));
  }
  else if (PyInt_Check(pyret))
  {
    qret = QVariant((qlonglong)PyInt_AS_LONG(pyret));
  }
  else if (PyLong_Check(pyret))
  {
    qret = QVariant((qlonglong)PyLong_AsLongLong(pyret));
  }
  else if (PyNumber_Check(pyret))
  {
    PyObject *number = PyNumber_Float(pyret);
    if (number)
    {
      qret = QVariant(PyFloat_AS_DOUBLE(number));
      Py_DECREF(number);
    }
  }
  /* bool */
  else if (PyBool_Check(pyret))
  {
    qret = QVariant(pyret==Py_True, 0);
  }
  // could handle advanced types (such as PyList->QValueList) here if needed
  /* fallback: try to convert to (unicode) string */
  if(!qret.isValid())
  {
    PyObject *pystring = PyObject_Unicode(pyret);
    if (pystring)
    {
      PyObject *asUTF8 = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(pystring), (int)PyUnicode_GET_DATA_SIZE(pystring), NULL);
      Py_DECREF(pystring);
      if (asUTF8)
      {
        qret = QVariant(QString::fromUtf8(PyString_AS_STRING(asUTF8)));
        Py_DECREF(asUTF8);
      }
      else if ((pystring = PyObject_Str(pyret)))
      {
        qret = QVariant(QString(PyString_AS_STRING(pystring)));
        Py_DECREF(pystring);
      }
    }
  }

  Py_DECREF(pyret);
  if (PyErr_Occurred())
  {
    if (PyErr_ExceptionMatches(PyExc_ValueError) || PyErr_ExceptionMatches(PyExc_ZeroDivisionError))
    {
      PyErr_Clear(); // silently ignore errors
      return  QVariant("");
    }
    else
    {
      emit_error();
    }
    return QVariant();
  }
  return qret;
}

bool PythonScript::executeImpl()
{
  return executeString();
}


/// Performs the call to Python
bool PythonScript::executeString()
{
  emit started(MSG_STARTED);
  bool success(false);
  GlobalInterpreterLock gil;

  PyObject * compiledCode = compileToByteCode(false);
  PyObject *result(NULL);
  if(compiledCode)
  {
    result = executeCompiledCode(compiledCode);
  }
  // If an error has occurred we need to construct the error message
  // before any other python code is run
  QString msg;
  if(!result)
  {
    emit_error();
  }
  else
  {
    emit finished(MSG_FINISHED);
    success = true;
  }
  if(isInteractive())
  {
    generateAutoCompleteList();
  }

  Py_XDECREF(compiledCode);
  Py_XDECREF(result);

  return success;
}

namespace
{
  /**
   * RAII struct for installing a tracing function
   * to monitor the line number events and ensuring it is removed
   * when the object is destroyed
   */
  struct InstallTrace
  {
    InstallTrace(PythonScript & scriptObject)
      : m_sipWrappedScript(NULL)
    {
      if(scriptObject.reportProgress())
      {
        m_sipWrappedScript = scriptObject.createSipInstanceFromMe();
        PyEval_SetTrace((Py_tracefunc)&traceLineNumber, m_sipWrappedScript);
      }
    }
    ~InstallTrace()
    {
      PyEval_SetTrace(NULL, NULL);
      Py_XDECREF(m_sipWrappedScript);
    }
  private:
    InstallTrace();
    Q_DISABLE_COPY(InstallTrace);
    PyObject *m_sipWrappedScript;
  };
}

/**
 * Executes the compiled code object. If NULL nothing happens
 * @param compiledCode An object that has been compiled from a code fragment
 * @return The result python object
 */
PyObject* PythonScript::executeCompiledCode(PyObject *compiledCode)
{
  PyObject *result(NULL);
  if(!compiledCode) return result;

  InstallTrace traceInstall(*this);
  beginStdoutRedirect();
  result = PyEval_EvalCode((PyCodeObject*)compiledCode, localDict, localDict);
  endStdoutRedirect();
  return result;
}

/**
 * Check an object for a result.  A valid pointer indicates success
 * @param result The output from a PyEval call
 * @return A boolean indicating success status
 */
bool PythonScript::checkResult(PyObject *result)
{
  if(result)
  {
    return true;
  }
  else
  {
    return false;
  }
}


/**
 * Compile the code
 */
PyObject *PythonScript::compileToByteCode(bool for_eval)
{
  // Support for the convenient col() and cell() functions.
  // This can't be done anywhere else, because we need access to the local
  // variables self, i and j.
  if( context() )
  {
    if( context()->inherits("Table") )
    {
      // A bit of a hack, but we need either IndexError or len() from __builtins__.
      PyDict_SetItemString(localDict, "__builtins__",
          PyDict_GetItemString(pythonEnv()->globalDict(), "__builtins__"));
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
    }
    else if(context()->inherits("Matrix"))
    {
      // A bit of a hack, but we need either IndexError or len() from __builtins__.
      PyDict_SetItemString(localDict, "__builtins__",
          PyDict_GetItemString(pythonEnv()->globalDict(), "__builtins__"));
      PyObject *ret = PyRun_String(
          "def cell(*arg):\n"
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
  PyObject *compiledCode = Py_CompileString(codeString().c_str(), identifier().c_str(), Py_file_input);

  if( compiledCode )
  {
    success = true;
  }
  else if (for_eval)
  {
    // Code contains statements (or errors) and we want to get a return
    // value from it.
    // So we wrap the code into a function definition,
    // execute that (as Py_file_input) and store the function object in PyCode.
    // See http://mail.python.org/pipermail/python-list/2001-June/046940.html
    // for why there isn't an easier way to do this in Python.
    PyErr_Clear(); // silently ignore errors
    PyObject *key(NULL), *value(NULL);
    Py_ssize_t i(0);
    QString signature = "";
    while(PyDict_Next(localDict, &i, &key, &value))
    {
      signature.append(PyString_AsString(key)).append(",");
    }
    signature.truncate(signature.length()-1);
    std::string fdef = "def __doit__("+signature.toStdString()+"):\n";
    fdef += codeString();
    compiledCode = Py_CompileString(fdef.c_str(), identifier().c_str(), Py_file_input);
    if( compiledCode )
    {
      PyObject *tmp = PyDict_New();
      Py_XDECREF(PyEval_EvalCode((PyCodeObject*)compiledCode, localDict, tmp));
      Py_DECREF(compiledCode);
      compiledCode = PyDict_GetItemString(tmp,"__doit__");
      Py_XINCREF(compiledCode);
      Py_DECREF(tmp);
    }
    success = (compiledCode != NULL);
  }
  else
  {
  }

  if(success) 
  {
    m_CodeFileObject = ((PyCodeObject*)(compiledCode))->co_filename;
  }
  else
  {
    compiledCode = NULL;
    m_CodeFileObject = NULL;
  }
  return compiledCode;
}

//--------------------------------------------------------------------------------------------

/**
 * Listen to add notifications from the ADS and add a Python variable of the workspace name
 * to the current scope
 * @param wsName The name of the workspace
 * @param ws The ws ptr (unused)
 */
void PythonScript::addHandle(const std::string& wsName,const Mantid::API::Workspace_sptr ws)
{
  addPythonReference(wsName, ws);
}

/**
 * Listen to add/replace notifications from the ADS and add a Python variable of the workspace name
 * to the current scope
 * @param wsName The name of the workspace
 * @param ws The ws ptr (unused)
 */
void PythonScript::afterReplaceHandle(const std::string& wsName,const Mantid::API::Workspace_sptr ws)
{
  addPythonReference(wsName, ws);
}

/**
 * Removes a Python variable of the workspace name from the current scope
 * @param wsName The name of the workspace
 * @param ws The ws ptr (unused)
 */
void PythonScript::postDeleteHandle(const std::string& wsName)
{
  deletePythonReference(wsName);
}

/**
 * Clear all workspace handle references
 */
void PythonScript::clearADSHandle()
{
  std::set<std::string>::const_iterator iend = m_workspaceHandles.end();
  for( std::set<std::string>::const_iterator itr = m_workspaceHandles.begin(); itr != iend; )
  {
    // This also erases the element from current set. The standard says that erase only invalidates
    // iterators of erased elements so we need to increment the iterator and get back the previous value
    // i.e. the postfix operator
    this->deletePythonReference(*(itr++));
  }
  
  assert(m_workspaceHandles.empty());
}


/**
 * Add a Python variable of the workspace name
 * to the current scope
 * @param wsName The name of the workspace
 * @param ws The ws ptr (unused)
 */
void PythonScript::addPythonReference(const std::string& wsName,const Mantid::API::Workspace_sptr ws)
{
  UNUSED_ARG(ws);

  // Compile a code object
  const size_t length = wsName.length() * 2 + 10;
  char * code = new char[length + 1];
  const char * name = wsName.c_str();
  sprintf(code, "%s = mtd['%s']", name, name);
  PyObject *codeObj = Py_CompileString(code, "PythonScript::addPythonReference", Py_file_input);
  if( codeObj )
  {
    PyObject *ret = PyEval_EvalCode((PyCodeObject*)codeObj,localDict, localDict);
    Py_XDECREF(ret);
  }
  if( PyErr_Occurred() )
  {
    PyErr_Clear();
  }
  else
  {
    // Keep track of it
    m_workspaceHandles.insert(m_workspaceHandles.end(), wsName);
  }
  Py_XDECREF(codeObj);
  delete [] code;
}


/**
 * Delete a Python reference to the given workspace name
 * @param wsName The name of the workspace
 */
void PythonScript::deletePythonReference(const std::string& wsName)
{
  const size_t length = wsName.length() + 4;
  char * code = new char[length + 1];
  sprintf(code, "del %s", wsName.c_str());
  PyObject *codeObj = Py_CompileString(code, "PythonScript::deleteHandle", Py_file_input);
  if( codeObj )
  {
    PyObject *ret = PyEval_EvalCode((PyCodeObject*)codeObj,localDict, localDict);
    Py_XDECREF(ret);
  }
  if( PyErr_Occurred() )
  {
    PyErr_Clear();
  }
  else
  {
    m_workspaceHandles.erase(wsName);
  }
  Py_XDECREF(codeObj);
  delete [] code;

}
