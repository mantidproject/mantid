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
#include "Mantid/MantidUI.h"

#include <QObject>
#include <QStringList>
#include <QDir>
#include <QApplication>
#include <QTemporaryFile>
#include <QTextStream>

#include <Qsci/qscilexerpython.h> 
#include "MantidKernel/ConfigService.h"

#include "sipAPIqti.h"

// Function is defined in a sip object file that is linked in later. There is no header file
// so this is necessary
extern "C" void initqti();

// Language name
const char* PythonScripting::langName = "Python";

// Factory function
ScriptingEnv *PythonScripting::constructor(ApplicationWindow *parent) 
{ 
  return new PythonScripting(parent); 
}

/** Constructor */
PythonScripting::PythonScripting(ApplicationWindow *parent)
  : ScriptingEnv(parent, langName), m_globals(NULL), m_math(NULL),
    m_sys(NULL)
{
  // MG (Russell actually found this for OS X): We ship SIP and PyQt4 with Mantid and we need to
  // ensure that the internal import that sip does of PyQt picks up the correct version.
#if defined(Q_OS_DARWIN) || defined(Q_OS_LINUX)
 #if defined(Q_OS_DARWIN)
  const std::string sipLocation = "/Applications/MantidPlot.app/Contents/MacOS";
 #else
  const std::string sipLocation = Mantid::Kernel::ConfigService::Instance().getPropertiesDir();
 #endif
  // MG: The documentation claims that if the third argument to setenv is non zero then it will update the
  // environment variable. What this seems to mean is that it actually overwrites it. So here we'll have 
  // to save it and update it ourself.
  const char * envname = "PYTHONPATH";
  char * pythonpath = getenv(envname);
  std::string value("");
  if( pythonpath )
  {
    // Only doing this for Darwin and Linux so separator is always ":"
    value = std::string(pythonpath);
  }
  value = sipLocation + ":" + value;
  setenv(envname, value.c_str(), 1);
#endif
}

PythonScripting::~PythonScripting()
{
  shutdown();
}

bool PythonScripting::isRunning() const
{
  return (m_is_running || d_parent->mantidUI->runningAlgCount() > 0 );
}

/**
 * Create a code lexer for Python. Ownership of the created object is transferred to the caller.
 */
QsciLexer * PythonScripting::createCodeLexer() const
{
  return new QsciLexerPython;
}

/**
 * Start the Python environment
 */
bool PythonScripting::start()
{
  try
  {
    if( Py_IsInitialized() ) return true;
    // Initialize interpreter, disabling signal registration as we don't need it
    Py_InitializeEx(0);

    // Add in Mantid paths.
    QDir mantidbin(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getPropertiesDir()));
    QString pycode =
        "import sys\n"
        "mantidbin = '" +  mantidbin.absolutePath() + "'\n" +
        "if not mantidbin in sys.path:\n"
        "\tsys.path.insert(0,mantidbin)\n";

    QDir mantidoutput(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getUserPropertiesDir()));
    if( mantidoutput != mantidbin )
    {
      pycode += QString("sys.path.insert(1,'") + mantidoutput.absolutePath() + QString("')\n");
    }
    PyRun_SimpleString(pycode.toStdString().c_str());

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

    //Create a new dictionary for the math functions
    m_math = PyDict_New();

    pymodule = PyImport_ImportModule("sys");
    m_sys = PyModule_GetDict(pymodule);
    if( !m_sys )
    {
      shutdown();
      return false;
    }

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

    //Get the refresh protection flag
    Mantid::Kernel::ConfigService::Instance().getValue("pythonalgorithms.refresh.allowed", refresh_allowed);

    if( loadInitFile(mantidbin.absoluteFilePath("qtiplotrc.py")) &&
        loadInitFile(mantidbin.absoluteFilePath("mantidplotrc.py")) )
    {
      d_initialized = true;
    }
    else
    {
      d_initialized = false;
    }
    return d_initialized;
  }
  catch(std::exception & ex)
  {
    std::cerr << "Exception in PythonScripting.cpp: " << ex.what() << std::endl;
    return false;
  }
  catch(...)
  {
    std::cerr << "Exception in PythonScripting.cpp" << std::endl;
    return false;
  }
}

/**
 * Shutdown the interpreter
 */
void PythonScripting::shutdown()
{
  if( m_math )
  {
    Py_DECREF(m_math);
    m_math = NULL;
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

QStringList PythonScripting::toStringList(PyObject *py_seq)
{
  QStringList elements;
  if( PyList_Check(py_seq) )
  {
    Py_ssize_t nitems = PyList_Size(py_seq);
    for( Py_ssize_t i = 0; i < nitems; ++i )
    {
      PyObject *item = PyList_GetItem(py_seq, i);
      if( PyString_Check(item) )
      {
	elements << PyString_AsString(item);
      }
    }
  }
  return elements;
}

bool PythonScripting::setQObject(QObject *val, const char *name, PyObject *dict)
{
  if(!val) return false;
  PyObject *pyobj=NULL;
  
  if (!sipAPI_qti)
  {
    throw std::runtime_error("sipAPI_qti is undefined");
  }
  if (!sipAPI_qti->api_find_class)
  {
    throw std::runtime_error("sipAPI_qti->api_find_class is undefined");
  }
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
  Py_ssize_t i=0;
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

void PythonScripting::refreshAlgorithms(bool force)
{
  if( (force || !isRunning()) && refresh_allowed==1)
  {
    PyRun_SimpleString("mtd._refreshPyAlgorithms()");
  }
}

//------------------------------------------------------------
// Private member functions
//------------------------------------------------------------

bool PythonScripting::loadInitFile(const QString & filename)
{
  if( !filename.endsWith(".py") || !QFileInfo(filename).isReadable() )
  {
    return false;
  }
  this->write(QString("Loading init file: ") + filename + "\n");
  // MG: The Python/C PyRun_SimpleFile function crashes on Windows when trying to run
  // a simple text file which is why it is not used here
  QFile file(filename);
  bool success(false);
  if(file.open(QIODevice::ReadOnly | QIODevice::Text) )
  {
    QByteArray data = file.readAll();
    if( PyRun_SimpleString(data.data() ) == 0 )
    {
      success = true;
    }
    else
    {
      success = false;
    }
	  file.close();
  }
  else
  {
    this->write(QString("Error: Cannot open file \"") + filename + "\"\n");
    success = false;
  }
  if( !success )
  {
    this->write("Error running init file \"" + filename + "\"\n");
  }

  return success;
}

