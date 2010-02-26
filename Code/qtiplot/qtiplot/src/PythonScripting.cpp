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

#include "PythonScripting.h"
#include "ApplicationWindow.h"

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QCoreApplication>

#include <Qsci/qscilexerpython.h> //Mantid
#include "MantidKernel/ConfigService.h" //Mantid

// includes sip.h, which undefines Qt's "slots" macro since SIP 4.6
#include "../sipAPIqti.h"
// Function is defined in a sip object file that is linked in later. There is no header file
// so this is necessary
extern "C" void initqti();

// Language name
const char* PythonScripting::langName = "Python";

//Factory function
ScriptingEnv *PythonScripting::constructor(ApplicationWindow *parent) 
{ 
  return new PythonScripting(parent); 
}

//Constructor
PythonScripting::PythonScripting(ApplicationWindow *parent)
  : ScriptingEnv(parent, langName), m_globals(NULL), m_math(NULL),
    m_sys(NULL)
{
}

PythonScripting::~PythonScripting()
{
  shutdown();
}

/**
 * Start the Python environment
 */
bool PythonScripting::start()
{
  if( Py_IsInitialized() ) return true;
  // Initialize interpreter, disabling signal registration as we don't need it
  Py_InitializeEx(0);
  //Keep a hold of the globals, math and sys dictionary objects
  PyObject *pymodule = PyImport_AddModule("__main__");
  if( !pymodule )
  {
    shutdown();
    return false;
  }
  m_globals = PyModule_GetDict(pymodule);
  if( !m_globals )
  {
    shutdown();
    return false;
  }
  Py_INCREF(m_globals);

  //Create a new dictionary for the math functions
  m_math = PyDict_New();

  pymodule = PyImport_ImportModule("sys"); 
  m_sys = PyModule_GetDict(pymodule);
  if( !m_sys )
  {
    shutdown();
    return false;
  }
  Py_INCREF(m_sys);
  //Embedded qti module needs sip definitions initializing before it can be used
  initqti();

  pymodule = PyImport_ImportModule("qti");
  if( pymodule )
  {
    PyDict_SetItemString(m_globals, "qti", pymodule);
    PyObject *qti_dict = PyModule_GetDict(pymodule);
    setQObject(d_parent, "app", qti_dict);
    PyDict_SetItemString(qti_dict, "mathFunctions", m_math);
    Py_DECREF(pymodule);
  } 
  else
  {
    shutdown();
    return false;
  }

  setQObject(this, "stdout", m_sys);
  setQObject(this, "stderr", m_sys);

  QDir mantidbin(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getBaseDir()));
  QString pycode = 
    QString("import sys; sys.path.append('") + mantidbin.absolutePath() + QString("');");
  QDir mantidoutput(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getOutputDir()));
  if( mantidoutput != mantidbin )
  {
      pycode += QString("sys.path.append('") + mantidoutput.absolutePath() + QString("');");
  }
  PyRun_SimpleString(pycode.toStdString().c_str());

  // Changed initialization to include a script which also loads the 
  // MantidPythonAPI - M. Gigg
  if( loadInitFile(mantidbin.absoluteFilePath("qtiplotrc")) && 
      loadInitFile(mantidbin.absoluteFilePath("mantidplotrc")) )
  {
    d_initialized = true;
  }
  else
  {
    d_initialized = false;
  }

  // If all previous initialization has been successful load the auto complete information
  if( !d_initialized ) return false;
  
  QsciLexer *lexer = new QsciLexerPython;
  setCodeLexer(lexer);
  //Fixed API
  QDir bindir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getBaseDir()));
  // Load the fixed API for Mantid Python
  this->updateCodeCompletion(bindir.absoluteFilePath(completionSourceName()), false);
  // Generated simple API 
  QDir outputdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getOutputDir()));
  this->updateCodeCompletion(outputdir.absoluteFilePath("mtdpyalgorithm_keywords.txt"), true);
  return true;
}

/**
 * Shutdown the interpreter
 */
void PythonScripting::shutdown()
{
  if( m_globals )
  {
    Py_XDECREF(m_globals);
    m_globals = NULL;
  }
  if( m_math )
  {
    Py_XDECREF(m_math);
    m_math = NULL;
  }
  if( m_sys )
  {
    Py_XDECREF(m_sys);

    m_sys = NULL;
  }
  Py_Finalize();
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
    PyDict_SetItemString(m_globals,name,pyobj);
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
		PyDict_SetItemString(m_globals,name,pyobj);
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
		PyDict_SetItemString(m_globals,name,pyobj);
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
	while(PyDict_Next(m_math, &i, &key, &value))
		if (PyCallable_Check(value))
			flist << PyString_AsString(key);
	flist.sort();
	return flist;
}

const QString PythonScripting::mathFunctionDoc(const QString &name) const
{
	PyObject *mathf = PyDict_GetItemString(m_math,name); // borrowed
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

void PythonScripting::refreshAlgorithms()
{
  PyRun_SimpleString("mtd._refreshPyAlgorithms()");
}

void PythonScripting::refreshCompletion()
{
  QDir outputdir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getOutputDir()));
  this->updateCodeCompletion(outputdir.absoluteFilePath("mtdpyalgorithm_keywords.txt"), true);
}

//------------------------------------------------------------
// Private member functions
//------------------------------------------------------------

bool PythonScripting::loadInitFile(const QString &path)
{
  QFileInfo pyFile(path+".py"), pycFile(path+".pyc");
  bool success = false;
  if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) 
  {
    // if we have a recent pycFile, use it
    FILE *f = fopen(pycFile.filePath(), "rb");
    success = (PyRun_SimpleFileEx(f, pycFile.filePath(), false) == 0);
  } 
  else if (pyFile.isReadable() && pyFile.exists())
  {
    // try to compile pyFile to pycFile if the current location is writable
    QString testfile(QFileInfo(path).absoluteDir().absoluteFilePath("UNLIKELYFILENAME"));
    QFile tester(testfile);
    if( tester.open(QIODevice::WriteOnly) )
    {
      PyObject *compileModule = PyImport_ImportModule("py_compile");
      if (compileModule)
      {
	PyObject *compile = PyDict_GetItemString(PyModule_GetDict(compileModule), "compile");
	if (compile) 
	{
	  PyObject *tmp = PyObject_CallFunctionObjArgs(compile,
						       PyString_FromString(pyFile.filePath()),
						       PyString_FromString(pycFile.filePath()),
						       NULL);
	  if (tmp)
	    Py_DECREF(tmp);
	  else
	    PyErr_Print();
	} 
	else
	{
	  PyErr_Print();
	}
	Py_DECREF(compileModule);
      } 
      else
      {
	PyErr_Print();
      }
      pycFile.refresh();
    }
    //Remove the testing file   
    tester.remove();
    if (pycFile.isReadable() && (pycFile.lastModified() >= pyFile.lastModified())) 
    {
      // run the newly compiled pycFile
      FILE *f = fopen(pycFile.filePath(), "rb");
      success = (PyRun_SimpleFileEx(f, pycFile.filePath(), false) == 0);
      fclose(f);
    } 
    else 
    {
      // fallback: just run pyFile
	/*FILE *f = fopen(pyFile.filePath(), "r");
	  success = PyRun_SimpleFileEx(f, pyFile.filePath(), false) == 0;
	  fclose(f);*/
	//TODO: code above crashes on Windows - bug in Python?
      QFile f(pyFile.filePath());
      if (f.open(QIODevice::ReadOnly | QIODevice::Text))
      {
	QByteArray data = f.readAll();
	success = (PyRun_SimpleString(data.data()) == 0);
	f.close();
      }
    }
  }
  return success;
}
