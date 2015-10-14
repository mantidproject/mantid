#ifdef _MSC_VER
#pragma warning(disable : 4250) // Disable warning regarding inheritance via
                                // dominance, we have no way around it with the
                                // design
#endif
#include "MantidAPI/IAlgorithm.h"
#include "MantidPythonInterface/api/AlgorithmIDProxy.h"
#ifdef _MSC_VER
#pragma warning(default : 4250)
#endif
#include "MantidKernel/Strings.h"
#include "MantidPythonInterface/kernel/IsNone.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <Poco/Thread.h>

#include <boost/python/arg_from_python.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::Property;
using Mantid::Kernel::Direction;
using Mantid::API::AlgorithmID;
using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::PythonInterface::AlgorithmIDProxy;
using Mantid::PythonInterface::isNone;
using Mantid::PythonInterface::Policies::VectorToNumpy;
using namespace boost::python;

namespace {

// Global map of the thread ID to the current algorithm object
dict THREAD_ID_MAP;

/**
 * Private method to add an algorithm reference to the thread id map.
 * It replaces any current reference with the same ID
 * @param threadID The current Python thread ID
 * @param alg A Python reference to the algorithm object
 */
void _trackAlgorithmInThread(long threadID, const object &alg) {
  THREAD_ID_MAP[threadID] = alg;
}

/**
 * Return the algorithm object for the given thread ID or None
 * if one doesn't exist. The entry is removed from the map
 * if it is found
 */
object _algorithmInThread(long threadID) {
  auto value = THREAD_ID_MAP.get(threadID);
  if (value)
    api::delitem(THREAD_ID_MAP, threadID);
  return value;
}

/// Functor for use with sorting algorithm to put the properties that do not
/// have valid values first
struct MandatoryFirst {
  /// Comparator operator for sort algorithm, places optional properties lower
  /// in the list
  bool operator()(const Property *p1, const Property *p2) const {
    // this is false, unless p1 is not valid and p2 is valid
    return (p1->isValid() != "") && (p2->isValid() == "");
  }
};

//----------------------- Property ordering ------------------------------
/// Vector of property pointers
typedef std::vector<Property *> PropertyVector;

/**
 * Returns the vector of properties ordered by the criteria defined in
 * MandatoryFirst.
 * A stable_sort is applied to the properties to guarantee the relative order
 * of with respect to the original list.
 * @return A list of Property pointers ordered by for the function API
 */
PropertyVector apiOrderedProperties(const IAlgorithm &propMgr) {
  PropertyVector properties(
      propMgr.getProperties()); // Makes a copy so that it can be sorted
  std::stable_sort(properties.begin(), properties.end(), MandatoryFirst());
  return properties;
}

/**
 * Returns a list of input property names that is ordered such that the
 * mandatory properties are first followed by the optional ones. The list
 * excludes output properties.
 * @param self :: A pointer to the python object wrapping and Algorithm.
 * @return A Python list of strings
 */

PyObject *getInputPropertiesWithMandatoryFirst(IAlgorithm &self) {
  PropertyVector properties(apiOrderedProperties(self));

  PropertyVector::const_iterator iend = properties.end();
  // Build a python list
  PyObject *names = PyList_New(0);
  for (PropertyVector::const_iterator itr = properties.begin(); itr != iend;
       ++itr) {
    Property *p = *itr;
    if (p->direction() != Direction::Output) {
      PyList_Append(names, PyString_FromString(p->name().c_str()));
    }
  }
  return names;
}

/**
* Returns a list of input property names that is ordered such that the
* mandatory properties are first followed by the optional ones. The list
* also includes InOut properties.
* @param self :: A pointer to the python object wrapping and Algorithm.
* @return A Python list of strings
*/
PyObject *getAlgorithmPropertiesOrdered(IAlgorithm &self) {
  PropertyVector properties(apiOrderedProperties(self));

  PropertyVector::const_iterator iend = properties.end();
  // Build a python list
  PyObject *names = PyList_New(0);
  for (PropertyVector::const_iterator itr = properties.begin(); itr != iend;
       ++itr) {
    Property *p = *itr;
    PyList_Append(names, PyString_FromString(p->name().c_str()));
  }
  return names;
}

/**
 * Returns a list of output property names in the order they were declared in
 * @param self :: A pointer to the python object wrapping and Algorithm.
 * @return A Python list of strings
 */
PyObject *getOutputProperties(IAlgorithm &self) {
  const PropertyVector &properties(self.getProperties()); // No copy
  PropertyVector::const_iterator iend = properties.end();
  // Build the list
  PyObject *names = PyList_New(0);
  for (PropertyVector::const_iterator itr = properties.begin(); itr != iend;
       ++itr) {
    Property *p = *itr;
    if (p->direction() == Direction::Output) {
      PyList_Append(names, PyString_FromString(p->name().c_str()));
    }
  }
  return names;
}

/**
 * Create a doc string for the simple API
 * @param self :: A pointer to the python object wrapping and Algorithm
 * @return A string that documents an algorithm
 */
std::string createDocString(IAlgorithm &self) {
  const std::string EOL = "\n";

  // Put in the quick overview message
  std::stringstream buffer;
  std::string temp = self.summary();
  if (temp.size() > 0)
    buffer << temp << EOL << EOL;

  // get a sorted copy of the properties
  PropertyVector properties(apiOrderedProperties(self));

  const size_t numProps(properties.size());

  buffer << "Property descriptions: " << EOL << EOL;
  // write the actual property descriptions
  for (size_t i = 0; i < numProps; ++i) {
    Mantid::Kernel::Property *prop = properties[i];
    buffer << prop->name() << "("
           << Mantid::Kernel::Direction::asText(prop->direction());
    if (!prop->isValid().empty())
      buffer << ":req";
    buffer << ") *" << prop->type() << "* ";
    std::vector<std::string> allowed = prop->allowedValues();
    if (!prop->documentation().empty() || !allowed.empty()) {
      buffer << "      " << prop->documentation();
      if (!allowed.empty()) {
        buffer << "[" << Mantid::Kernel::Strings::join(allowed.begin(),
                                                       allowed.end(), ", ");
        buffer << "]";
      }
      buffer << EOL;
      if (i < numProps - 1)
        buffer << EOL;
    }
  }

  return buffer.str();
}

/**
 * RAII struct to drop the GIL and reacquire it on destruction.
 * If the algorithm is not a child then it added to the map of
 * tracked algorithms. See executeProxy for a more
 * detailed explanation
 */
struct AllowCThreads {
  AllowCThreads(const object &algm)
      : m_tracefunc(NULL), m_tracearg(NULL), m_saved(NULL), m_tracking(false) {
    PyThreadState *curThreadState = PyThreadState_GET();
    m_tracefunc = curThreadState->c_tracefunc;
    m_tracearg = curThreadState->c_traceobj;
    Py_XINCREF(m_tracearg);
    PyEval_SetTrace(NULL, NULL);
    if (!isNone(algm)) {
      _trackAlgorithmInThread(curThreadState->thread_id, algm);
      m_tracking = true;
    }
    m_saved = PyEval_SaveThread();
  }
  ~AllowCThreads() {
    PyEval_RestoreThread(m_saved);
    if (m_tracking) {
      try {
        api::delitem(THREAD_ID_MAP, m_saved->thread_id);
      } catch (error_already_set &) {
        PyErr_Clear();
      }
    }
    PyEval_SetTrace(m_tracefunc, m_tracearg);
    Py_XDECREF(m_tracearg);
  }

private:
  Py_tracefunc m_tracefunc;
  PyObject *m_tracearg;
  PyThreadState *m_saved;
  bool m_tracking;
};

/**
 * Execute the algorithm
 * @param self :: A reference to the calling object
 */
bool executeProxy(object &self) {
  // We need to do 2 things before we execute the algorthm:
  //   1. store a reference to this algorithm mapped to the current thread ID to
  //      allow it to be looked up when an abort request is received
  //   2. release the GIL, drop the Python threadstate and reset anything
  //   installed
  //      via PyEval_SetTrace while we execute the C++ code - AllowCThreads()
  //      does this for us

  // Extract this before dropping GIL as I'm not sure if it calls any Python
  auto &calg = extract<IAlgorithm &>(self)();
  AllowCThreads threadStateHolder((!calg.isChild()) ? self : object());
  return calg.execute();
}

/**
 * @param self A reference to the calling object
 * @return An AlgorithmID wrapped in a AlgorithmIDProxy container or None if
 * there is no ID
 */
PyObject *getAlgorithmID(IAlgorithm &self) {
  AlgorithmID id = self.getAlgorithmID();
  if (id)
    return to_python_value<AlgorithmIDProxy>()(AlgorithmIDProxy(id));
  else
    Py_RETURN_NONE;
}

//--------------------------------------------------------------------------------------
// Deprecated wrappers
//--------------------------------------------------------------------------------------
/**
 * @param self Reference to the calling object
 * @return Algorithm summary
 */
std::string getOptionalMessage(IAlgorithm &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             ".getOptionalMessage() is deprecated. Use .summary() instead.");
  return self.summary();
}

/**
 * @param self Reference to the calling object
 * @return Algorithm summary
 */
std::string getWikiSummary(IAlgorithm &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             ".getWikiSummary() is deprecated. Use .summary() instead.");
  return self.summary();
}
}

void export_ialgorithm() {
  class_<AlgorithmIDProxy>("AlgorithmID", no_init).def(self == self);

  register_ptr_to_python<boost::shared_ptr<IAlgorithm>>();

  class_<IAlgorithm, bases<IPropertyManager>, boost::noncopyable>(
      "IAlgorithm", "Interface for all algorithms", no_init)
      .def("name", &IAlgorithm::name, "Returns the name of the algorithm")
      .def("alias", &IAlgorithm::alias, "Return the aliases for the algorithm")
      .def("version", &IAlgorithm::version,
           "Returns the version number of the algorithm")
      .def("cancel", &IAlgorithm::cancel,
           "Request that the algorithm stop running")
      .def("category", &IAlgorithm::category,
           "Returns the category containing the algorithm")
      .def("categories", &IAlgorithm::categories,
           "Returns the list of categories this algorithm belongs to")
      .def("summary", &IAlgorithm::summary,
           "Returns a summary message describing the algorithm")
      .def("workspaceMethodName", &IAlgorithm::workspaceMethodName,
           "Returns a name that will be used when attached as a workspace "
           "method. Empty string indicates do not attach")
      .def("workspaceMethodOn", &IAlgorithm::workspaceMethodOn,
           return_value_policy<VectorToNumpy>(), // creates a list for strings
           "Returns a set of class names that will have the method attached. "
           "Empty list indicates all types")
      .def("workspaceMethodInputProperty",
           &IAlgorithm::workspaceMethodInputProperty,
           "Returns the name of the input workspace property used by the "
           "calling object")
      .def("getAlgorithmID", &getAlgorithmID,
           "Returns a unique identifier for this algorithm object")
      .def("docString", &createDocString,
           "Returns a doc string for the algorithm")
      .def("mandatoryProperties", &getInputPropertiesWithMandatoryFirst,
           "Returns a list of input and in/out property names that is ordered "
           "such that the mandatory properties are first followed by the "
           "optional ones.")
      .def("orderedProperties", &getAlgorithmPropertiesOrdered,
           "Return a list of input, in/out and output properties "
           "such that the mandatory properties are first followed by the "
           "optional ones.")
      .def("outputProperties", &getOutputProperties,
           "Returns a list of the output properties on the algorithm")
      .def("isInitialized", &IAlgorithm::isInitialized,
           "Returns True if the algorithm is initialized, False otherwise")
      .def("isExecuted", &IAlgorithm::isExecuted,
           "Returns True if the algorithm has been executed successfully, "
           "False otherwise")
      .def("isLogging", &IAlgorithm::isLogging, "Returns True if the "
                                                "algorithm's logger is turned "
                                                "on, False otherwise")
      .def("isRunning", &IAlgorithm::isRunning, "Returns True if the algorithm "
                                                "is considered to be running, "
                                                "False otherwise")
      .def("setChild", &IAlgorithm::setChild,
           "If true this algorithm is run as a child algorithm. There will be "
           "no logging and nothing is stored in the Analysis Data Service")
      .def("enableHistoryRecordingForChild",
           &IAlgorithm::enableHistoryRecordingForChild,
           "If true then history will be recorded regardless of the child "
           "status")
      .def("setAlgStartupLogging", &IAlgorithm::setAlgStartupLogging,
           "If true then allow logging of start and end messages")
      .def("getAlgStartupLogging", &IAlgorithm::getAlgStartupLogging,
           "Returns true if logging of start and end messages")
      .def("setAlwaysStoreInADS", &IAlgorithm::setAlwaysStoreInADS,
           "If true then even child algorithms will have their workspaces "
           "stored in the ADS.")
      .def("isChild", &IAlgorithm::isChild,
           "Returns True if the algorithm has been marked to run as a child. "
           "If True then Output workspaces "
           "are NOT stored in the Analysis Data Service but must be retrieved "
           "from the property.")
      .def("setLogging", &IAlgorithm::setLogging, "Toggle logging on/off.")
      .def("setRethrows", &IAlgorithm::setRethrows)
      .def("initialize", &IAlgorithm::initialize, "Initializes the algorithm")
      .def("validateInputs", &IAlgorithm::validateInputs,
           "Cross-check all inputs and return any errors as a dictionary")
      .def("execute", &executeProxy,
           "Runs the algorithm and returns whether it has been successful")
      // 'Private' static methods
      .def("_algorithmInThread", &_algorithmInThread)
      .staticmethod("_algorithmInThread")
      // Special methods
      .def("__str__", &IAlgorithm::toString)

      // deprecated methods
      .def("getOptionalMessage", &getOptionalMessage,
           "Returns the optional user message attached to the algorithm")
      .def("getWikiSummary", &getWikiSummary,
           "Returns the summary found on the wiki page");
}
