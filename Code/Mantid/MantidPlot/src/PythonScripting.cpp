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
#include "MantidQtAPI/InterfaceManager.h"

#include <cassert>

#include "sipAPI_qti.h"

// Avoids a compiler warning about implicit 'const char *'->'char*' conversion under clang
#define STR_LITERAL(str) const_cast<char*>(str)

// Function is defined in a sip object file that is linked in later. There is no header file
// so this is necessary
extern "C" void init_qti();

// Factory function
ScriptingEnv *PythonScripting::constructor(ApplicationWindow *parent)
{
  return new PythonScripting(parent);
}

/** Constructor */
PythonScripting::PythonScripting(ApplicationWindow *parent)
  : ScriptingEnv(parent, "Python"), m_globals(NULL), m_math(NULL),
    m_sys(NULL), m_mainThreadState(NULL)
{
  // MG (Russell actually found this for OS X): We ship SIP and PyQt4 with Mantid and we need to
  // ensure that the internal import that sip does of PyQt picks up the correct version.
#if defined(Q_OS_DARWIN) || defined(Q_OS_LINUX)
  const std::string sipLocation = Mantid::Kernel::ConfigService::Instance().getPropertiesDir();
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
}

/**
 * @param args A list of strings that denoting command line arguments
 */
void PythonScripting::setSysArgs(const QStringList &args)
{
  ScopedPythonGIL gil;

  PyObject *argv = toPyList(args);
  if(argv && m_sys)
  {
    PyDict_SetItemString(m_sys, "argv", argv);
  }
}

/**
 * Create a new script object that can execute code within this environment
 *
 * @param name :: A identifier of the script, mainly used in error messages
 * @param interact :: Whether the script is interactive defined by @see Script::InteractionType
 * @param context :: An object that identifies the current context of the script (NULL is allowed)
 * @return
 */
Script *PythonScripting::newScript(const QString &name, QObject * context,
                                   const Script::InteractionType interact) const
{
  return new PythonScript(const_cast<PythonScripting*>(this), name, interact, context);
}

/**
 * Create a code lexer for Python. Ownership of the created object is transferred to the caller.
 */
QsciLexer * PythonScripting::createCodeLexer() const
{
  return new QsciLexerPython;
}

/**
 * Turn redirects on/off
 * @return
 */
void PythonScripting::redirectStdOut(bool on)
{
  if(on)
  {
    setQObject(this, "stdout", m_sys);
    setQObject(this, "stderr", m_sys);
  }
  else
  {
    PyDict_SetItemString(m_sys, "stdout",PyDict_GetItemString(m_sys, "__stdout__"));
    PyDict_SetItemString(m_sys, "stderr",PyDict_GetItemString(m_sys, "__stderr__"));

  }
}

/**
 * Start the Python environment
 */
bool PythonScripting::start()
{
  try
  {
    if( Py_IsInitialized() ) return true;
    Py_Initialize();
    // Assume this is called at startup by the the main thread so no GIL required...yet

    //Keep a hold of the globals, math and sys dictionary objects
    PyObject *mainmod = PyImport_AddModule("__main__");
    if( !mainmod )
    {
      finalize();
      return false;
    }
    m_globals = PyModule_GetDict(mainmod);
    if( !m_globals )
    {
      finalize();
      return false;
    }

    //Create a new dictionary for the math functions
    m_math = PyDict_New();
    // Keep a hold of the sys dictionary for accessing stdout/stderr
    PyObject *sysmod = PyImport_ImportModule("sys");
    m_sys = PyModule_GetDict(sysmod);
    if( !m_sys )
    {
      finalize();
      return false;
    }
    // Set a smaller check interval so that it takes fewer 'ticks' to respond to a KeyboardInterrupt
    // The choice of 5 is really quite arbitrary
    PyObject_CallMethod(sysmod, STR_LITERAL("setcheckinterval"), STR_LITERAL("i"), 5);
    Py_DECREF(sysmod);

    // Our use of the IPython console requires that we use the v2 api for these PyQt types
    // This has to be set before the very first import of PyQt (which happens in init_qti)
    PyRun_SimpleString("import sip\nsip.setapi('QString',2)\nsip.setapi('QVariant',2)");
    //Embedded qti module needs sip definitions initializing before it can be used
    init_qti();

    PyObject *qtimod = PyImport_ImportModule("_qti");
    if( qtimod )
    {
      PyDict_SetItemString(m_globals, "_qti", qtimod);
      PyObject *qti_dict = PyModule_GetDict(qtimod);
      setQObject(d_parent, "app", qti_dict);
      PyDict_SetItemString(qti_dict, "mathFunctions", m_math);
      Py_DECREF(qtimod);
    }
    else
    {
      finalize();
      return false;
    }

    redirectStdOut(true);
    // Add in Mantid paths so that the framework will be found
    // Linux has the libraries in the lib directory at bin/../lib
    using namespace Mantid::Kernel;
    ConfigServiceImpl &configSvc = ConfigService::Instance();
    QDir mantidbin(QString::fromStdString(configSvc.getPropertiesDir()));
    QString pycode =
      "import sys\n"
      "import os\n"
      "mantidbin = '%1'\n"
      "if not mantidbin in sys.path:\n"
      "    sys.path.insert(0,mantidbin)\n"
      "sys.path.insert(1, os.path.join(mantidbin,'..','lib'))";
    pycode = pycode.arg(mantidbin.absolutePath());
    PyRun_SimpleString(pycode.toStdString().c_str());

    if( loadInitFile(mantidbin.absoluteFilePath("mantidplotrc.py")) )
    {
      d_initialized = true;
    }
    else
    {
      d_initialized = false;
    }
  }
  catch(std::exception & ex)
  {
    std::cerr << "Exception in PythonScripting.cpp: " << ex.what() << std::endl;
    d_initialized = false;
  }
  catch(...)
  {
    std::cerr << "Exception in PythonScripting.cpp" << std::endl;
    d_initialized = false;
  }
  if(d_initialized) {
    // We will be using C threads created outside of the Python threading module
    // so we need the GIL. This creates and acquires the lock for this thread
    PyEval_InitThreads();
    // We immediately release the lock and threadstate so that other points in
    // the code can simply use the PyGILstate_Ensure/PyGILstate_Release()
    // mechanism (through the ScopedPythonGIL class) and they don't
    // need to worry about swapping out the threadstate before hand.
    // It would be better if the ScopedPythonGIL handled this but
    // PyEval_SaveThread() needs to be called in the thread that spawns the
    // new C thread meaning that ScopedPythonGIL could no longer
    // be used as a simple RAII class on the stack from within the new thread.
    m_mainThreadState = PyEval_SaveThread();
  }
  return d_initialized;
}

/**
 * Shutdown the interpreter
 */
void PythonScripting::shutdown()
{
  PyEval_RestoreThread(m_mainThreadState);
  Py_XDECREF(m_math);
  Py_Finalize();
}

QString PythonScripting::toString(PyObject *object, bool decref)
{
  QString ret;
  if (!object) return ret;
  PyObject *repr = PyObject_Str(object);
  if (decref)
  {
    Py_DECREF(object);
  }
  if (!repr) return ret;
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

/**
 * Returns a new Python list from the given Qt QStringList
 * @param items A reference to a QStringList object
 * @return A new reference to a Python list. Caller is responsible for calling Py_DECREF
 */
PyObject *PythonScripting::toPyList(const QStringList &items)
{
  Py_ssize_t length = static_cast<Py_ssize_t>(items.length());
  PyObject * pylist = PyList_New((length));
  for(Py_ssize_t i = 0; i < length; ++i)
  {
    QString item = items.at(static_cast<int>(i));
    PyList_SetItem(pylist, i,
                   PyString_FromString(item.ascii()));
  }
  return pylist;
}

/**
 * Returns an integer representation of the object as a c long. No check is performed to see if it is an integer
 * @param object :: A PyInt_Type instance
 * @param decref :: If true then the Py_DECREF will be called on the object. Useful for converting
 * things straight from functions like PyObject_GetAttrString
 */
long PythonScripting::toLong(PyObject *object, bool decref)
{
  assert(object);
  long cvalue = PyInt_AsLong(object);
  if(decref)
  {
    Py_DECREF(object);
  }
  return cvalue;
}

/**
 * @brief Raise an exception in the target thread. The GIL must be held
 * @param id The associated Python thread id
 * @param exc The Python exception type
 */
void PythonScripting::raiseAsyncException(long id, PyObject *exc)
{
  PyThreadState_SetAsyncExc(id, exc);
}


bool PythonScripting::setQObject(QObject *val, const char *name, PyObject *dict)
{
  if(!val) return false;
  PyObject *pyobj=NULL;

  if (!sipAPI__qti)
  {
    throw std::runtime_error("sipAPI_qti is undefined");
  }
  if (!sipAPI__qti->api_find_class)
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

bool PythonScripting::setInt(int val, const char* name)
{
  return setInt(val,name,NULL);
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

bool PythonScripting::setDouble(double val, const char *name)
{
  return setDouble(val,name,NULL);
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

//------------------------------------------------------------
// Private member functions
//------------------------------------------------------------

bool PythonScripting::loadInitFile(const QString & filename)
{
  if( !filename.endsWith(".py") || !QFileInfo(filename).isReadable() )
  {
    return false;
  }
  //this->write(QString("Loading init file: ") + filename + "\n");
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
