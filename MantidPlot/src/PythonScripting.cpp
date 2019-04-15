// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2006 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/***************************************************************************
  File                 : PythonScripting.cpp
  Project              : QtiPlot
--------------------------------------------------------------------
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

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/VersionCompat.h"

#include <QApplication>
#include <Qsci/qscilexerpython.h>

#include <array>
#include <cassert>

#include "sipAPI_qti.h"

// Avoids a compiler warning about implicit 'const char *'->'char*' conversion
// under clang
#define STR_LITERAL(str) const_cast<char *>(str)

// The init functions are defined in the generated sip_qtipart0.cpp
#if defined(IS_PY3K)
PyMODINIT_FUNC PyInit__qti();
#else
PyMODINIT_FUNC init_qti();
#endif

using Mantid::PythonInterface::GlobalInterpreterLock;

namespace {
Mantid::Kernel::Logger g_log("PythonScripting");

bool checkAndPrintError() {
  if (PyErr_Occurred()) {
    PyErr_Print();
    return true;
  }
  return false;
}
} // namespace

// Factory function
ScriptingEnv *PythonScripting::constructor(ApplicationWindow *parent) {
  return new PythonScripting(parent);
}

/** Constructor */
PythonScripting::PythonScripting(ApplicationWindow *parent)
    : ScriptingEnv(parent, "Python"), m_globals(nullptr), m_math(nullptr),
      m_sys(nullptr), m_mainThreadState(nullptr) {}

PythonScripting::~PythonScripting() {}

/**
 * @param args A list of strings that denoting command line arguments
 */
void PythonScripting::setSysArgs(const QStringList &args) {
  GlobalInterpreterLock lock;
  PyObject *argv = toPyList(args);
  if (argv && m_sys) {
    PyDict_SetItemString(m_sys, "argv", argv);
  }
}

/**
 * Create a new script object that can execute code within this environment
 *
 * @param name :: A identifier of the script, mainly used in error messages
 * @param interact :: Whether the script is interactive defined by @see
 * Script::InteractionType
 * @param context :: An object that identifies the current context of the script
 * (NULL is allowed)
 * @return
 */
Script *
PythonScripting::newScript(const QString &name, QObject *context,
                           const Script::InteractionType interact) const {
  return new PythonScript(const_cast<PythonScripting *>(this), name, interact,
                          context);
}

/**
 * Create a code lexer for Python. Ownership of the created object is
 * transferred to the caller.
 */
QsciLexer *PythonScripting::createCodeLexer() const {
  return new QsciLexerPython;
}

/**
 * Turn redirects on/off
 * @return
 */
void PythonScripting::redirectStdOut(bool on) {
  if (on) {
    setQObject(this, "stdout", m_sys);
    setQObject(this, "stderr", m_sys);
  } else {
    PyDict_SetItem(m_sys, FROM_CSTRING("stdout"),
                   PyDict_GetItemString(m_sys, "__stdout__"));
    PyDict_SetItem(m_sys, FROM_CSTRING("stderr"),
                   PyDict_GetItemString(m_sys, "__stderr__"));
  }
}

/**
 * Start the Python environment
 */
bool PythonScripting::start() {
  if (Py_IsInitialized())
    return true;
// This must be called before initialize
#if defined(IS_PY3K)
  PyImport_AppendInittab("_qti", &PyInit__qti);
#else
  PyImport_AppendInittab("_qti", &init_qti);
#endif
  Py_Initialize();
  // Acquires the GIL
  PyEval_InitThreads();
  // Release GIL so that we can use our scoped lock types for management
  PyEval_SaveThread();

  GlobalInterpreterLock lock;
  // Keep a hold of the globals, math and sys dictionary objects
  PyObject *mainmod = PyImport_AddModule("__main__");
  if (!mainmod) {
    checkAndPrintError();
    finalize();
    return false;
  }
  m_globals = PyModule_GetDict(mainmod);
  if (!m_globals) {
    checkAndPrintError();
    finalize();
    return false;
  }

  // Create a new dictionary for the math functions
  m_math = PyDict_New();
  // Keep a hold of the sys dictionary for accessing stdout/stderr
  PyObject *sysmod = PyImport_ImportModule("sys");
  if (checkAndPrintError()) {
    finalize();
    return false;
  }
  m_sys = PyModule_GetDict(sysmod);
  // Configure python paths to find our modules
  setupPythonPath();
  // Set a smaller check interval so that it takes fewer 'ticks' to respond to
  // a KeyboardInterrupt
  // The choice of 5 is really quite arbitrary
  PyObject_CallMethod(sysmod, STR_LITERAL("setcheckinterval"), STR_LITERAL("i"),
                      5);
  Py_DECREF(sysmod);

  // Custom setup for sip/PyQt4 before import _qti
  setupSip();
  if (checkAndPrintError()) {
    finalize();
    return false;
  }
  // Setup _qti
  PyObject *qtimod = PyImport_ImportModule("_qti");
  if (qtimod) {
    PyDict_SetItemString(m_globals, "_qti", qtimod);
    PyObject *qti_dict = PyModule_GetDict(qtimod);
    setQObject(d_parent, "app", qti_dict);
    PyDict_SetItemString(qti_dict, "mathFunctions", m_math);
    Py_DECREF(qtimod);
  } else {
    checkAndPrintError();
    finalize();
    return false;
  }

  // Capture all stdout/stderr
  redirectStdOut(true);
  if (loadInitRCFile()) {
    d_initialized = true;
  } else {
    checkAndPrintError();
    d_initialized = false;
  }
  return d_initialized;
}

/**
 * Shutdown the interpreter
 */
void PythonScripting::shutdown() {
  // The scoped lock cannot be used here as after the
  // finalize call no Python code can execute.
  GlobalInterpreterLock::acquire();
  Py_XDECREF(m_math);
  Py_Finalize();
}

void PythonScripting::setupPythonPath() {
  using Mantid::Kernel::ConfigService;
  // First add the directory of the executable as a sitedir to process
  // any .pth files
  const auto appPath = ConfigService::Instance().getPropertiesDir();
  PyObject *sitemod = PyImport_ImportModule("site");
  Py_DECREF(PyObject_CallMethod(sitemod, "addsitedir", STR_LITERAL("(s)"),
                                appPath.c_str()));
  Py_DECREF(sitemod);

  // The python sys.path is then updated as follows:
  //   - the empty string is inserted as position 0 to mimic the
  //     behaviour of the vanilla python interpreter
  //   - the directory of MantidPlot is added after this to find any additional
  //     modules alongside the executable

#if PY_MAJOR_VERSION >= 3
  PyObject *syspath = PySys_GetObject("path");
#else
  std::string path("path");
  PyObject *syspath = PySys_GetObject(&path[0]);
#endif
  PyList_Insert(syspath, 0, FROM_CSTRING(""));

  // These should contain only / separators
  // Python paths required by VTK and ParaView
  const auto pvPythonPaths =
      ConfigService::Instance().getString("paraview.pythonpaths");

  if (!pvPythonPaths.empty()) {
    Mantid::Kernel::StringTokenizer tokenizer(
        pvPythonPaths, ";",
        Mantid::Kernel::StringTokenizer::TOK_IGNORE_EMPTY |
            Mantid::Kernel::StringTokenizer::TOK_TRIM);
    for (const auto &pvPath : tokenizer) {
      if (pvPath.substr(0, 3) == "../") {
        std::string fullPath = appPath + pvPath;
        PyList_Insert(syspath, 1, FROM_CSTRING(fullPath.c_str()));
      } else {
        PyList_Insert(syspath, 1, FROM_CSTRING(pvPath.c_str()));
      }
    }
  }

  // MantidPlot Directory
  PyList_Insert(syspath, 1, FROM_CSTRING(appPath.c_str()));
}

void PythonScripting::setupSip() {
  // Our use of the IPython console requires that we use the v2 api for these
  // PyQt types. This has to be set before the very first import of PyQt
  // which happens on importing _qti
  PyObject *sipmod = PyImport_ImportModule("sip");
  if (sipmod) {
    constexpr std::array<const char *, 7> v2Types = {
        {"QString", "QVariant", "QDate", "QDateTime", "QTextStream", "QTime",
         "QUrl"}};
    for (const auto &className : v2Types) {
      PyObject_CallMethod(sipmod, STR_LITERAL("setapi"), STR_LITERAL("(si)"),
                          className, 2);
    }
    Py_DECREF(sipmod);
  }
  // the global Python error handler is checked after this is called...
}

QString PythonScripting::toString(PyObject *object, bool decref) {
  QString ret;
  if (!object)
    return ret;
  PyObject *repr = PyObject_Str(object);
  if (decref) {
    Py_DECREF(object);
  }
  if (!repr)
    return ret;
  ret = TO_CSTRING(repr);
  Py_DECREF(repr);
  return ret;
}

QStringList PythonScripting::toStringList(PyObject *py_seq) {
  QStringList elements;
  if (PyList_Check(py_seq)) {
    Py_ssize_t nitems = PyList_Size(py_seq);
    for (Py_ssize_t i = 0; i < nitems; ++i) {
      PyObject *item = PyList_GetItem(py_seq, i);
      if (STR_CHECK(item)) {
        elements << TO_CSTRING(item);
      }
    }
  }
  return elements;
}

/**
 * Returns a new Python list from the given Qt QStringList
 * @param items A reference to a QStringList object
 * @return A new reference to a Python list. Caller is responsible for calling
 * Py_DECREF
 */
PyObject *PythonScripting::toPyList(const QStringList &items) {
  Py_ssize_t length = static_cast<Py_ssize_t>(items.length());
  PyObject *pylist = PyList_New((length));
  for (Py_ssize_t i = 0; i < length; ++i) {
    const QString &item = items.at(static_cast<int>(i));
    PyList_SetItem(pylist, i, FROM_CSTRING(item.toAscii()));
  }
  return pylist;
}

/**
 * Returns an integer representation of the object as a c long. No check is
 * performed to see if it is an integer
 * @param object :: A PyInt_Type instance
 * @param decref :: If true then the Py_DECREF will be called on the object.
 * Useful for converting
 * things straight from functions like PyObject_GetAttrString
 */
long PythonScripting::toLong(PyObject *object, bool decref) {
  assert(object);
  long cvalue = TO_LONG(object);
  if (decref) {
    Py_DECREF(object);
  }
  return cvalue;
}

/**
 * @brief Raise an exception in the target thread. The GIL must be held
 * @param id The associated Python thread id
 * @param exc The Python exception type
 */
void PythonScripting::raiseAsyncException(long id, PyObject *exc) {
  PyThreadState_SetAsyncExc(id, exc);
}

bool PythonScripting::setQObject(QObject *val, const char *name,
                                 PyObject *dict) {
  if (!val)
    return false;
  PyObject *pyobj = nullptr;

  if (!sipAPI__qti) {
    throw std::runtime_error("sipAPI_qti is undefined");
  }
  if (!sipAPI__qti->api_find_class) {
    throw std::runtime_error("sipAPI_qti->api_find_class is undefined");
  }
  const sipTypeDef *klass = sipFindType(val->metaObject()->className());
  if (!klass)
    return false;
  pyobj = sipConvertFromType(val, klass, nullptr);

  if (!pyobj)
    return false;

  if (dict)
    PyDict_SetItemString(dict, name, pyobj);
  else
    PyDict_SetItemString(m_globals, name, pyobj);
  Py_DECREF(pyobj);
  return true;
}

bool PythonScripting::setInt(int val, const char *name) {
  return setInt(val, name, nullptr);
}

bool PythonScripting::setInt(int val, const char *name, PyObject *dict) {
  PyObject *pyobj = Py_BuildValue("i", val);
  if (!pyobj)
    return false;
  if (dict)
    PyDict_SetItemString(dict, name, pyobj);
  else
    PyDict_SetItemString(m_globals, name, pyobj);
  Py_DECREF(pyobj);
  return true;
}

bool PythonScripting::setDouble(double val, const char *name) {
  return setDouble(val, name, nullptr);
}

bool PythonScripting::setDouble(double val, const char *name, PyObject *dict) {
  PyObject *pyobj = Py_BuildValue("d", val);
  if (!pyobj)
    return false;
  if (dict)
    PyDict_SetItemString(dict, name, pyobj);
  else
    PyDict_SetItemString(m_globals, name, pyobj);
  Py_DECREF(pyobj);
  return true;
}

const QStringList PythonScripting::mathFunctions() const {
  QStringList flist;
  PyObject *key, *value;
  Py_ssize_t i = 0;
  while (PyDict_Next(m_math, &i, &key, &value))
    if (PyCallable_Check(value))
      flist << TO_CSTRING(key);
  flist.sort();
  return flist;
}

const QString PythonScripting::mathFunctionDoc(const QString &name) const {
  PyObject *mathf =
      PyDict_GetItemString(m_math, name.toAscii().constData()); // borrowed
  if (!mathf)
    return "";
  PyObject *pydocstr = PyObject_GetAttrString(mathf, "__doc__"); // new
  QString qdocstr = TO_CSTRING(pydocstr);
  Py_XDECREF(pydocstr);
  return qdocstr;
}

const QStringList PythonScripting::fileExtensions() const {
  QStringList extensions;
  extensions << "py"
             << "PY";
  return extensions;
}

//------------------------------------------------------------
// Private member functions
//------------------------------------------------------------

bool PythonScripting::loadInitRCFile() {
  using Mantid::Kernel::ConfigService;
  // The file is expected to be next to the Mantid.properties file
  QDir propDir(
      QString::fromStdString(ConfigService::Instance().getPropertiesDir()));
  QString filename = propDir.absoluteFilePath("mantidplotrc.py");

  // The Python/C PyRun_SimpleFile function crashes on Windows when trying
  // to run a simple text so we read it manually and execute the string
  QFile file(filename);
  bool success(false);
  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QByteArray data = file.readAll();
    success = (PyRun_SimpleString(data.data()) == 0);
    if (!success) {
      g_log.error() << "Error running init file \""
                    << filename.toAscii().constData() << "\"\n";
      PyErr_Print();
    }
    file.close();
  } else {
    g_log.error() << "Error: Cannot open file \""
                  << filename.toAscii().constData() << "\"\n";
    success = false;
  }
  return success;
}
