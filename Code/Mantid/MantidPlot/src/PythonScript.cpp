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
#include "ApplicationWindow.h"

#include <QObject>
#include <QVariant>
#include <QMessageBox>
#include <QFileInfo>
#include <iostream>
#include <stdexcept>

namespace
{
  // Keep a reference to the current PyCode filename object for the line tracing.
  // The tracing function is a static member function so this varibable needs to be global
  // so it's kept in an anonymous namespace to keep it at this file scope.
  PyObject* ROOT_CODE_OBJECT = NULL;
  PythonScript *CURRENT_SCRIPT_OBJECT = NULL;

  static int _trace_callback(PyObject *, _frame *frame, int what, PyObject *)
  {
    // If we are in the main code and this is a line number event
    if( ROOT_CODE_OBJECT && frame->f_code->co_filename == ROOT_CODE_OBJECT
	&& what == PyTrace_LINE )
      {
	CURRENT_SCRIPT_OBJECT->broadcastNewLineNumber(frame->f_lineno);
      }
    // I don't care about the return value
    return 0;
  }

}

/**
 * Constructor
 */
PythonScript::PythonScript(PythonScripting *env, const QString &code, bool interactive, QObject *context, 
			   const QString &name)
  : Script(env, code, interactive, context, name), PyCode(NULL), localDict(NULL), 
    stdoutSave(NULL), stderrSave(NULL), isFunction(false)
{
  ROOT_CODE_OBJECT = NULL;
  CURRENT_SCRIPT_OBJECT = this;

  PyObject *pymodule = PyImport_AddModule("__main__");
  localDict = PyDict_Copy(PyModule_GetDict(pymodule));
  setQObject(Context, "self");
}

/**
 * Destructor
 */
PythonScript::~PythonScript()
{
  Py_DECREF(localDict);
  Py_XDECREF(PyCode);
}

/**
 * Update the current environment with the script's path
 */
void PythonScript::updatePath(const QString & filename, bool append)
{
  if( filename.isEmpty() ) return;
  QString scriptPath = QFileInfo(filename).absolutePath();
  QString pyCode;
  if( append )
  {
    pyCode = "sys.path.append(r'%1')";
  }
  else
  {
    pyCode = "if r'%1' in sys.path:\n"
      "    sys.path.remove(r'%1')";
  }
  setCode(pyCode.arg(scriptPath));
  exec();
}

/**
 * Compile the code
 */
bool PythonScript::compile(bool for_eval)
{
  // Support for the convenient col() and cell() functions.
  // This can't be done anywhere else, because we need access to the local
  // variables self, i and j.
  if( Context )
  {
    if( Context->inherits("Table") ) 
    {
      // A bit of a hack, but we need either IndexError or len() from __builtins__.
      PyDict_SetItemString(localDict, "__builtins__",
			   PyDict_GetItemString(env()->globalDict(), "__builtins__"));
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
    else if(Context->inherits("Matrix")) 
    {
      // A bit of a hack, but we need either IndexError or len() from __builtins__.
      PyDict_SetItemString(localDict, "__builtins__",
			   PyDict_GetItemString(env()->globalDict(), "__builtins__"));
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
  Py_XDECREF(PyCode);
  // Simplest case: Code is a single expression
  PyCode = Py_CompileString(Code, Name, Py_file_input);
	
  if( PyCode )
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
    fdef.append(Code);
    fdef.replace('\n',"\n\t");
    PyCode = Py_CompileString(fdef, Name, Py_file_input);
    if( PyCode )
    {
      PyObject *tmp = PyDict_New();
      Py_XDECREF(PyEval_EvalCode((PyCodeObject*)PyCode, env()->globalDict(), tmp));
      Py_DECREF(PyCode);
      PyCode = PyDict_GetItemString(tmp,"__doit__");
      Py_XINCREF(PyCode);
      Py_DECREF(tmp);
    }
    success = (PyCode != NULL);
  } 
  else 
  {
    // Code contains statements (or errors), but we do not need to get
    // a return value.
    PyErr_Clear(); // silently ignore errors
    PyCode = Py_CompileString(Code, Name, Py_file_input);
    success = (PyCode != NULL);
  }
	
  if(!success )
  {
    compiled = compileErr;
    emit_error(constructErrorMsg(), 0);
  } 
  else
  {
    compiled = isCompiled;
  }
  return success;
}

QVariant PythonScript::eval()
{	
  if (!isFunction) compiled = notCompiled;
  if (compiled != isCompiled && !compile(true))
    return QVariant();
  
  PyObject *pyret;
  beginStdoutRedirect();
  if (PyCallable_Check(PyCode)){
    PyObject *empty_tuple = PyTuple_New(0);
    pyret = PyObject_Call(PyCode, empty_tuple, localDict);
    Py_DECREF(empty_tuple);
  } else
    pyret = PyEval_EvalCode((PyCodeObject*)PyCode, env()->globalDict(), localDict);
  endStdoutRedirect();
  if (!pyret){
    if (PyErr_ExceptionMatches(PyExc_ValueError) ||
	PyErr_ExceptionMatches(PyExc_ZeroDivisionError)){				
      PyErr_Clear(); // silently ignore errors
      return  QVariant("");
    } else {
      emit_error(constructErrorMsg(), 0);
      return QVariant();
    }
  }

  QVariant qret = QVariant();
  /* None */
  if (pyret == Py_None)
    qret = QVariant("");
  /* numeric types */
  else if (PyFloat_Check(pyret))
    qret = QVariant(PyFloat_AS_DOUBLE(pyret));
  else if (PyInt_Check(pyret))
    qret = QVariant((qlonglong)PyInt_AS_LONG(pyret));
  else if (PyLong_Check(pyret))
    qret = QVariant((qlonglong)PyLong_AsLongLong(pyret));
  else if (PyNumber_Check(pyret)){
    PyObject *number = PyNumber_Float(pyret);
    if (number){
      qret = QVariant(PyFloat_AS_DOUBLE(number));
      Py_DECREF(number);
    }
    /* bool */
  } else if (PyBool_Check(pyret))
    qret = QVariant(pyret==Py_True, 0);
  // could handle advanced types (such as PyList->QValueList) here if needed
  /* fallback: try to convert to (unicode) string */
  if(!qret.isValid()) {
    PyObject *pystring = PyObject_Unicode(pyret);
    if (pystring) {
      PyObject *asUTF8 = PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(pystring), PyUnicode_GET_DATA_SIZE(pystring), 0);
      Py_DECREF(pystring);
      if (asUTF8) {
	qret = QVariant(QString::fromUtf8(PyString_AS_STRING(asUTF8)));
	Py_DECREF(asUTF8);
      } else if ((pystring = PyObject_Str(pyret))) {
	qret = QVariant(QString(PyString_AS_STRING(pystring)));
	Py_DECREF(pystring);
      }
    }
  }
  
  Py_DECREF(pyret);
  if (PyErr_Occurred()){
    if (PyErr_ExceptionMatches(PyExc_ValueError) ||
	PyErr_ExceptionMatches(PyExc_ZeroDivisionError)){
      PyErr_Clear(); // silently ignore errors
      return  QVariant("");
	} else {
      emit_error(constructErrorMsg(), 0);
	return QVariant();
	  }
  } else
    return qret;
}

bool PythonScript::exec()
{
  env()->setIsRunning(true);
  
  if (isFunction) compiled = notCompiled;
  if (compiled != Script::isCompiled && !compile(false))
  {
    env()->setIsRunning(false);
    return false;
  }

  // Redirect the output
  beginStdoutRedirect();

  if( isInteractive && env()->reportProgress() )
  {
    // This stores the address of the main file that is being executed so that
    // we can track line numbers from the main code only
    ROOT_CODE_OBJECT = ((PyCodeObject*)PyCode)->co_filename;
    PyEval_SetTrace((Py_tracefunc)&_trace_callback, PyCode);
  }
  else
  {
    ROOT_CODE_OBJECT = NULL;
    PyEval_SetTrace(NULL, NULL);
  }

  PyObject *pyret(NULL), *empty_tuple(NULL);
  if( PyCallable_Check(PyCode) )
  {
    empty_tuple = PyTuple_New(0);
    if (!empty_tuple) 
    {
      emit_error(constructErrorMsg(), 0);
      env()->setIsRunning(false);
      return false;
    }
  }
  /// Return value is NULL if everything succeeded
  pyret = executeScript(empty_tuple);

  // Restore output
  endStdoutRedirect();
	
  /// Disable trace
  PyEval_SetTrace(NULL, NULL);

  if( pyret ) 
  {
    Py_DECREF(pyret);
    env()->setIsRunning(false);
    return true;
  }
  emit_error(constructErrorMsg(), 0);
  env()->setIsRunning(false);
  return false;
}

/**
 * Perform the appropriate call to a Python eval command.
 * @param return_tuple :: If this is a valid pointer then the code object is called rather than executed and the return values are placed into this tuple
 * @returns A pointer to an object indicating the success/failure of the code execution
 */
PyObject* PythonScript::executeScript(PyObject* return_tuple)
{
  // Before requested code is executed we want to "uninstall" the modules 
  // containing Python algorithms so that a fresh import reloads them
  //
  env()->refreshAlgorithms(true);

  PyObject* pyret(NULL);
  //If an exception is thrown the thread state needs resetting so we need to save it
  PyThreadState *saved_tstate = PyThreadState_GET();
  try
  {
    if( return_tuple )
    {
      pyret = PyObject_Call(PyCode, return_tuple,localDict);
    }
    else
    {

      pyret = PyEval_EvalCode((PyCodeObject*)PyCode, localDict, localDict);
    }
  }
  // Given that C++ has no mechanism to move through a code block first if an exception is thrown, some code needs to
  // be repeated here 
  catch(const std::bad_alloc&)
  {
    PyThreadState_Swap(saved_tstate);// VERY VERY important. Bad things happen if this state is not reset after a throw
    pyret = NULL;
    PyErr_NoMemory();
  }
  catch(const std::out_of_range& x)
  {
    PyThreadState_Swap(saved_tstate);
    pyret = NULL;
    PyErr_SetString(PyExc_IndexError, x.what());
  }
  catch(const std::invalid_argument& x)
  {
    PyThreadState_Swap(saved_tstate);
    pyret = NULL;
    PyErr_SetString(PyExc_ValueError, x.what());
  }
  catch(const std::exception& x)
  {
    PyThreadState_Swap(saved_tstate);
    pyret = NULL;
    PyErr_SetString(PyExc_RuntimeError, x.what());
  }
  catch(...)
  {
    PyThreadState_Swap(saved_tstate);
    pyret = NULL;
    PyErr_SetString(PyExc_RuntimeError, "unidentifiable C++ exception");
  }
  
  // If we stole a reference, return it
  if( return_tuple )
  {
    Py_DECREF(return_tuple);
  }
  if( isInteractive && pyret )
  {
    emit keywordsChanged(createAutoCompleteList());
  }
  return pyret;
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
    //Py_DECREF(traceback);
  }


  //Exception value

  int msg_lineno(-1);
  int marker_lineno(-1);

  QString message("");
  QString exception_details("");
  if( PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError) )
  {
    msg_lineno = env()->toString(PyObject_GetAttrString(value, "lineno"),true).toInt();
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
    QString except_value(env()->toString(value));
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
    message = env()->toString(exception).section('.',1).remove("'>");
    exception_details = env()->toString(value) + QString(' ');
  }
  if( filename.isEmpty() && getLineOffset() >= 0 ) 
  {
    
    msg_lineno += getLineOffset();
  }
  if( getLineOffset() >= 0 && marker_lineno >= 0 )
  {
    marker_lineno += getLineOffset();
    message += " on line " + QString::number(marker_lineno);
  }
  
  message += ": \"" + exception_details.trimmed() + "\" ";
  if( marker_lineno >=0 
      && !PyErr_GivenExceptionMatches(exception, PyExc_SystemExit) 
      && !filename.isEmpty() 
      && filename != "<input>"
    )
  {
    message += "in file '" + QFileInfo(filename).fileName() 
      + "' at line " + QString::number(msg_lineno);
  }
  
  if( env()->reportProgress() )
  {
    emit currentLineChanged(marker_lineno, false);
  }

  // We're responsible for the reference count of these objects
  Py_XDECREF(traceback);
  Py_XDECREF(value);
  Py_XDECREF(exception);

  return message + QString("\n");
}


bool PythonScript::setQObject(QObject *val, const char *name)
{
  if (!PyDict_Contains(localDict, PyString_FromString(name)))
    compiled = notCompiled;
  return env()->setQObject(val, name, localDict);
}

bool PythonScript::setInt(int val, const char *name)
{
  if (!PyDict_Contains(localDict, PyString_FromString(name)))
    compiled = notCompiled;
  return env()->setInt(val, name, localDict);
}

bool PythonScript::setDouble(double val, const char *name)
{
  if (!PyDict_Contains(localDict, PyString_FromString(name)))
    compiled = notCompiled;
  return env()->setDouble(val, name, localDict);
}

void PythonScript::setContext(QObject *context)
{
  Script::setContext(context);
  setQObject(Context, "self");
}

/**
 * Create a list autocomplete keywords
 */
QStringList PythonScript::createAutoCompleteList() const
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
    return QStringList();
  }
  QStringList keyword_list;
  if( PyErr_Occurred() || !keywords )
  {
    PyErr_Print();
    return keyword_list;
  }

  keyword_list = env()->toStringList(keywords);
  Py_DECREF(keywords);
  Py_DECREF(method);
  return keyword_list;
}

//-------------------------------------------------------
// Private
//-------------------------------------------------------
PythonScripting * PythonScript::env() const
{ 
  //Env is protected in the base class
  return static_cast<PythonScripting*>(Env); 
}


/**
 * Redirect the std out to this object
 */
void PythonScript::beginStdoutRedirect()
{
  stdoutSave = PyDict_GetItemString(env()->sysDict(), "stdout");
  Py_XINCREF(stdoutSave);
  stderrSave = PyDict_GetItemString(env()->sysDict(), "stderr");
  Py_XINCREF(stderrSave);
  env()->setQObject(this, "stdout", env()->sysDict());
  env()->setQObject(this, "stderr", env()->sysDict());
}

/**
 * Restore the std out to this object to what it was before the last call to
 * beginStdouRedirect
 */
void PythonScript::endStdoutRedirect()
{
  PyDict_SetItemString(env()->sysDict(), "stdout", stdoutSave);
  Py_XDECREF(stdoutSave);
  PyDict_SetItemString(env()->sysDict(), "stderr", stderrSave);
  Py_XDECREF(stderrSave);
}
