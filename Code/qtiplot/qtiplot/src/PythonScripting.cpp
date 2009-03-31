/***************************************************************************
	File                 : PythonScripting.cpp
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
#include <Python.h>
#include <compile.h>
#include <eval.h>
#include <frameobject.h>
#include <traceback.h>

#if PY_VERSION_HEX < 0x020400A1
typedef struct _traceback {
	PyObject_HEAD
		struct _traceback *tb_next;
	PyFrameObject *tb_frame;
	int tb_lasti;
	int tb_lineno;
} PyTracebackObject;
#endif

#include "PythonScript.h"
#include "PythonScripting.h"
#include "ApplicationWindow.h"

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

#include <Qsci/qscilexerpython.h> //Mantid
#include <sstream>

// includes sip.h, which undefines Qt's "slots" macro since SIP 4.6
#include "sipAPIqti.h"
extern "C" void initqti();

const char* PythonScripting::langName = "Python";
ScriptingEnv *PythonScripting::constructor(ApplicationWindow *parent) { return new PythonScripting(parent); }

//Mantid - Creates the correct code lexer for syntax highlighting
QsciLexer* PythonScripting::scriptCodeLexer() const
{
  QsciLexer* lexer = new QsciLexerPython;
  return lexer;
}

QString PythonScripting::toString(PyObject *object, bool decref)
{
	QString ret;
	if (!object) return "";
	PyObject *repr = PyObject_Str(object);
	if (decref) 
	{
	  Py_DECREF(object);
	}
	if (!repr) return "";
	ret = PyString_AsString(repr);
	Py_DECREF(repr);
	return ret;
}

PyObject *PythonScripting::eval(const QString &code, PyObject *argDict, const char *name)
{
	PyObject *args;
	if (argDict)
	{
		Py_INCREF(argDict);
		args = argDict;
	} else
		args = PyDict_New();
	PyObject *ret=NULL;
	PyObject *co = Py_CompileString(code.ascii(), name, Py_eval_input);
	if (co)
	{
		ret = PyEval_EvalCode((PyCodeObject*)co, globals, args);
		Py_DECREF(co);
	}
	Py_DECREF(args);
	return ret;
}

bool PythonScripting::exec (const QString &code, PyObject *argDict, const char *name)
{
	PyObject *args;
	if (argDict)
	{
		Py_INCREF(argDict);
		args = argDict;
	} else
		args = PyDict_New();
	PyObject *tmp = NULL;
	PyObject *co = Py_CompileString(code.ascii(), name, Py_file_input);
	if (co)
	{
	  tmp = PyEval_EvalCode((PyCodeObject*)co, globals, args);
	  Py_DECREF(co);
	}
	Py_DECREF(args);
	if (!tmp) return false;
	Py_DECREF(tmp);
	return true;
}

QString PythonScripting::errorMsg()
{
	PyObject *exception=0, *value=0, *traceback=0;
	PyTracebackObject *excit=0;
	//	PyFrameObject *frame; // Mantid
	//char *fname; // Mantid
	QString msg;
	if (!PyErr_Occurred()) return "";

	PyErr_Fetch(&exception, &value, &traceback);
	PyErr_NormalizeException(&exception, &value, &traceback);

	//----- Mantid - Altered error message formatting -----
	int start = getFirstLineNumber();
	msg.append("Error:");
	if(PyErr_GivenExceptionMatches(exception, PyExc_SyntaxError))
	{
	  msg.append(" " + toString(PyObject_GetAttrString(value, "msg"), true));
	  msg.append(" '").append(toString(PyObject_GetAttrString(value, "filename"), true).remove("\n"));
	  msg.append("' on line ");
	  int offset = toString(PyObject_GetAttrString(value, "lineno"), true).toInt();
	  msg.append(QString::number(start+offset));
	  Py_DECREF(exception);
	  Py_DECREF(value);
	} 
 	else {
	  msg.append(" " + toString(exception,true)).remove("exceptions.").append(" - ");
	  msg.append(toString(value,true));
 	}

	if (traceback) {
	  excit = (PyTracebackObject*)traceback;
	  if( excit )
	  {
	    msg.append(" on line ");
	    msg.append(QString::number(start + excit->tb_lineno));
	  }
	  Py_DECREF(traceback);
	}
	//	msg.append("\n");
	//----------------------------------------------
	return msg;
}

PythonScripting::PythonScripting(ApplicationWindow *parent)
	: ScriptingEnv(parent, langName)
{
	PyObject *mainmod=NULL, *qtimod=NULL, *sysmod=NULL;
	math = NULL;
	sys = NULL;
	d_initialized = false;
	if (Py_IsInitialized())
	{
//		PyEval_AcquireLock();
		mainmod = PyImport_ImportModule("__main__");
		if (!mainmod)
		{
			PyErr_Print();
//			PyEval_ReleaseLock();
			return;
		}
		globals = PyModule_GetDict(mainmod);
		Py_DECREF(mainmod);
	} else {
//		PyEval_InitThreads ();
		Py_Initialize ();
		if (!Py_IsInitialized ())
			return;
		initqti();

		mainmod = PyImport_AddModule("__main__");
		if (!mainmod)
		{
//			PyEval_ReleaseLock();
			PyErr_Print();
			return;
		}
		globals = PyModule_GetDict(mainmod);
	}

	if (!globals)
	{
		PyErr_Print();
//		PyEval_ReleaseLock();
		return;
	}
	Py_INCREF(globals);

	math = PyDict_New();
	if (!math)
		PyErr_Print();

	qtimod = PyImport_ImportModule("qti");
	if (qtimod)
	{
		PyDict_SetItemString(globals, "qti", qtimod);
		PyObject *qtiDict = PyModule_GetDict(qtimod);
		setQObject(d_parent, "app", qtiDict);
		PyDict_SetItemString(qtiDict, "mathFunctions", math);
		Py_DECREF(qtimod);
	} else
		PyErr_Print();

	sysmod = PyImport_ImportModule("sys");
	if (sysmod)
	{
		sys = PyModule_GetDict(sysmod);
		Py_INCREF(sys);
	} else
		PyErr_Print();

//	PyEval_ReleaseLock();
	d_initialized = true;
	
}

bool PythonScripting::initialize()
{
	if (!d_initialized) return false;
//	PyEval_AcquireLock();

	// Redirect output to the print(const QString&) signal.
	// Also see method write(const QString&) and Python documentation on
	// sys.stdout and sys.stderr.
	setQObject(this, "stdout", sys);
	setQObject(this, "stderr", sys);


	// Changed initialization to include a script which also loads the 
	// MantidPythonAPI - M. Gigg
	bool initglob = loadInitFile(d_parent->d_python_config_folder + "/qtiplotrc");
	if(!initglob)
		initglob = loadInitFile("./qtiplotrc");

	bool initmtd = loadInitFile(d_parent->d_python_config_folder + "/mantidplotrc");
	if(!initmtd)
	  initmtd = loadInitFile("./mantidplotrc");

//	PyEval_ReleaseLock();
    return (initglob && initmtd);

}

PythonScripting::~PythonScripting()
{
	Py_XDECREF(globals);
	Py_XDECREF(math);
	Py_XDECREF(sys);
}

bool PythonScripting::loadInitFile(const QString &path)
{
	QFileInfo pyFile(path+".py"), pycFile(path+".pyc");
	bool success = false;
	if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) {
		// if we have a recent pycFile, use it
		FILE *f = fopen(pycFile.filePath(), "rb");
		success = PyRun_SimpleFileEx(f, pycFile.filePath(), false) == 0;
		fclose(f);
	} else if (pyFile.isReadable() && pyFile.exists()) {
		// try to compile pyFile to pycFile
		PyObject *compileModule = PyImport_ImportModule("py_compile");
		if (compileModule) {
			PyObject *compile = PyDict_GetItemString(PyModule_GetDict(compileModule), "compile");
			if (compile) {
				PyObject *tmp = PyObject_CallFunctionObjArgs(compile,
						PyString_FromString(pyFile.filePath()),
						PyString_FromString(pycFile.filePath()),
						NULL);
				if (tmp)
					Py_DECREF(tmp);
				else
					PyErr_Print();
			} else
				PyErr_Print();
			Py_DECREF(compileModule);
		} else
			PyErr_Print();
		pycFile.refresh();
		if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) {
			// run the newly compiled pycFile
			FILE *f = fopen(pycFile.filePath(), "rb");
			success = PyRun_SimpleFileEx(f, pycFile.filePath(), false) == 0;
			fclose(f);
		} else {
			// fallback: just run pyFile
			/*FILE *f = fopen(pyFile.filePath(), "r");
			success = PyRun_SimpleFileEx(f, pyFile.filePath(), false) == 0;
			fclose(f);*/
			//TODO: code above crashes on Windows - bug in Python?
			QFile f(pyFile.filePath());
			if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QByteArray data = f.readAll();
				success = PyRun_SimpleString(data.data());
				f.close();
			}
		}
	}
	return success;
}

bool PythonScripting::isRunning() const
{
	return Py_IsInitialized();
}

bool PythonScripting::setQObject(QObject *val, const char *name, PyObject *dict)
{
  if(!val) return false;
  PyObject *pyobj=NULL;
  
  sipWrapperType *klass = sipFindClass(val->className());
  if ( !klass ) return false;
  pyobj = sipConvertFromInstance(val, klass, NULL);
  
  if (!pyobj) return false;
  
  if (dict)
    PyDict_SetItemString(dict,name,pyobj);
  else
    PyDict_SetItemString(globals,name,pyobj);
  Py_DECREF(pyobj);
  return true;
}

bool PythonScripting::setInt(int val, const char *name, PyObject *dict)
{
	PyObject *pyobj = Py_BuildValue("i",val);
	if (!pyobj) return false;
	if (dict)
		PyDict_SetItemString(dict,name,pyobj);
	else
		PyDict_SetItemString(globals,name,pyobj);
	Py_DECREF(pyobj);
	return true;
}

bool PythonScripting::setDouble(double val, const char *name, PyObject *dict)
{
	PyObject *pyobj = Py_BuildValue("d",val);
	if (!pyobj) return false;
	if (dict)
		PyDict_SetItemString(dict,name,pyobj);
	else
		PyDict_SetItemString(globals,name,pyobj);
	Py_DECREF(pyobj);
	return true;
}

const QStringList PythonScripting::mathFunctions() const
{
	QStringList flist;
	PyObject *key, *value;
#if PY_VERSION_HEX >= 0x02050000
	Py_ssize_t i=0;
#else
	int i=0;
#endif
	while(PyDict_Next(math, &i, &key, &value))
		if (PyCallable_Check(value))
			flist << PyString_AsString(key);
	flist.sort();
	return flist;
}

const QString PythonScripting::mathFunctionDoc(const QString &name) const
{
	PyObject *mathf = PyDict_GetItemString(math,name); // borrowed
	if (!mathf) return "";
	PyObject *pydocstr = PyObject_GetAttrString(mathf, "__doc__"); // new
	QString qdocstr = PyString_AsString(pydocstr);
	Py_XDECREF(pydocstr);
	return qdocstr;
}

const QStringList PythonScripting::fileExtensions() const
{
	QStringList extensions;
	extensions << "py" << "PY";
	return extensions;
}
