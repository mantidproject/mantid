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
#include <QtConcurrentRun>

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
    PyObject_CallMethod(scriptObj, "lineNumberChanged", "O i", frame->f_code->co_filename, frame->f_lineno);
    return retcode;
  }
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
 * Emits a signal from this object indicating the current line number of the
 * code. This includes any offset
 * @param codeObject A pointer to the code object whose line is executing
 * @param lineNo The line number that the code is currently executing
 */
void PythonScript::lineNumberChanged(PyObject *codeObject, int lineNo)
{
  if(codeObject == m_CodeFileObject)
  {
    lineNo += getLineOffset();
    emit currentLineChanged(lineNo, false);
  }
}


/**
 * Compile the code returning true if successful, false otherwise
 * @param code
 * @return True if success, false otherwise
 */
bool PythonScript::compile(const QString & code)
{
  PyObject *codeObject = compileToByteCode(code, false);
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
QVariant PythonScript::evaluate(const QString & code)
{
  GlobalInterpreterLock gil;
  PyObject *compiledCode = this->compileToByteCode(code, true);
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
      emit error(constructErrorMsg(), name(), 0);
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
      emit error(constructErrorMsg(), name(), 0);
    }
    return QVariant();
  }
  return qret;
}

bool PythonScript::execute(const QString & code)
{
  emit startedSerial("");
  return doExecution(code);
}

QFuture<bool> PythonScript::executeAsync(const QString & code)
{
  emit startedAsync("");
  return QtConcurrent::run(this, &PythonScript::doExecution, code);
}

QString PythonScript::constructErrorMsg()
{
  if ( !PyErr_Occurred() ) 
  {
    return QString("");
  }
  PyObject *exception(NULL), *value(NULL), *traceback(NULL);
  PyTracebackObject *excit(NULL);
  PyErr_Fetch(&exception, &value, &traceback);
  PyErr_NormalizeException(&exception, &value, &traceback);

  // Get the filename of the error. This will be blank if it occurred in the main script
  QString filename("");
  int endtrace_line(-1);
  if( traceback )
  {
    excit = (PyTracebackObject*)traceback;
    while (excit && (PyObject*)excit != Py_None)
    {
      _frame *frame = excit->tb_frame;
      endtrace_line = excit->tb_lineno;
      filename = PyString_AsString(frame->f_code->co_filename);
      excit = excit->tb_next;
    }
  }

  // Exception value

  int msg_lineno(-1);
  int marker_lineno(-1);

  QString message("");
  QString exception_details("");
  if( PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError) )
  {
    msg_lineno = pythonEnv()->toString(PyObject_GetAttrString(value, "lineno"),true).toInt();
    if( traceback )
    {
      marker_lineno = endtrace_line;
    }
    else
    {
      // No traceback here get line from exception value object
      marker_lineno = msg_lineno;
    }

    message = "SyntaxError";
    QString except_value(pythonEnv()->toString(value));
    exception_details = except_value.section('(',0,0);
    filename = except_value.section('(',1).section(',',0,0);
  }
  else
  {
    if( traceback )
    {
      excit = (PyTracebackObject*)traceback;
      marker_lineno = excit->tb_lineno;
     }
    else
    {
      marker_lineno = -10000;
    }

    if( filename.isEmpty() )
    {
      msg_lineno = marker_lineno;
    }
    else
    {
      msg_lineno = endtrace_line;
    }
    message = pythonEnv()->toString(exception).section('.',1).remove("'>");
    exception_details = pythonEnv()->toString(value) + QString(' ');
  }
  message += " on line " + QString::number(marker_lineno);
  message += ": \"" + exception_details.trimmed() + "\" ";

  if( marker_lineno >=0 
      && !PyErr_GivenExceptionMatches(exception, PyExc_SystemExit) 
      && !filename.isEmpty()
    )
  {
    message += "in file '" + QFileInfo(filename).fileName() 
      + "' at line " + QString::number(msg_lineno);
  }
  
  emit currentLineChanged(marker_lineno, true);

  // We're responsible for the reference count of these objects
  Py_XDECREF(traceback);
  Py_XDECREF(value);
  Py_XDECREF(exception);

  PyErr_Clear();

  return message + QString("\n");
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
 * Sets the context for the script and if name points to a file then
 * sets the __file__ variable
 * @param name A string identifier for the script
 * @param context A QObject defining the context
 */
void PythonScript::initialize(const QString & name, QObject *context)
{
  GlobalInterpreterLock pythonlock;
  PyObject *pymodule = PyImport_AddModule("__main__");
  localDict = PyDict_Copy(PyModule_GetDict(pymodule));
  if( QFileInfo(name).exists() )
  {
    QString scriptPath = QFileInfo(name).absoluteFilePath();
    // Make sure the __file__ variable is set
    PyDict_SetItem(localDict,PyString_FromString("__file__"), PyString_FromString(scriptPath.toAscii().data()));
  }
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

/// Performs the call to Python
bool PythonScript::doExecution(const QString & code)
{
  emit started("Script execution started.");
  bool success(false);
  GlobalInterpreterLock gil;
  beginStdoutRedirect();

  PyCodeObject *compiledCode = (PyCodeObject*)Py_CompileString(code.toAscii(), nameAsCStr(), Py_file_input);
  PyObject *result(NULL);
  if(compiledCode)
  {
    PyObject *me(NULL);
    if(isInteractive())
    {
      m_CodeFileObject = compiledCode->co_filename;
      PyObject *me = createSipInstanceFromMe();
      PyEval_SetTrace((Py_tracefunc)&traceLineNumber, me);
    }
    result = PyEval_EvalCode(compiledCode, localDict, localDict);
    if(isInteractive())
    {
      PyEval_SetTrace(NULL, NULL);
      Py_XDECREF(me);
    }
    Py_DECREF(compiledCode);
    m_CodeFileObject = NULL;
  }
  endStdoutRedirect();
  if(result)
  {
    Py_DECREF(result);
    if(isInteractive())
    {
      generateAutoCompleteList();
    }
    emit finished("Script execution finished.");
    success = true;
  }
  else
  {
    emit error(constructErrorMsg(), name(), 0);
  }
  return success;
}

/**
 * Compile the code
 */
PyObject *PythonScript::compileToByteCode(const QString & code, bool for_eval)
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
  PyObject *compiledCode = Py_CompileString(code, nameAsCStr(), Py_file_input);

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
    QString fdef = "def __doit__("+signature+"):\n";
    fdef.append(code);
    fdef.replace('\n',"\n\t");
    compiledCode = Py_CompileString(fdef, nameAsCStr(), Py_file_input);
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
    // Code contains statements (or errors), but we do not need to get
    // a return value.
    PyErr_Clear(); // silently ignore errors
    compiledCode = Py_CompileString(code, nameAsCStr(), Py_file_input);
    success = (compiledCode != NULL);
  }

  if(!success)
  {
    emit error(constructErrorMsg(), name(), 0);
    compiledCode = NULL;
  }
  return compiledCode;
}

/**
 * Create a list autocomplete keywords
 */
void PythonScript::generateAutoCompleteList()
{
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
  if( PyErr_Occurred() || !keywords )
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
