// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
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
#include "MantidPythonInterface/core/Converters/MapToPyDictionary.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/GlobalInterpreterLock.h"
#include "MantidPythonInterface/core/IsNone.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <Poco/ActiveResult.h>
#include <Poco/Thread.h>

#include <boost/python/arg_from_python.hpp>
#include <boost/python/bases.hpp>
#include <boost/python/class.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>

#include <unordered_map>

using Mantid::API::AlgorithmID;
using Mantid::API::IAlgorithm;
using Mantid::API::IAlgorithm_sptr;
using Mantid::Kernel::Direction;
using Mantid::Kernel::IPropertyManager;
using Mantid::Kernel::Property;
using Mantid::PythonInterface::AlgorithmIDProxy;
using Mantid::PythonInterface::GlobalInterpreterLock;
using Mantid::PythonInterface::isNone;
using Mantid::PythonInterface::Policies::VectorToNumpy;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IAlgorithm)

namespace {

/// Converter for std::string to python string
using ToPyString = to_python_value<const std::string &>;

// Global map of the thread ID to the current algorithm object
using ThreadIDObjectMap = std::unordered_map<long, object>;

ThreadIDObjectMap &threadIDMap() {
  static ThreadIDObjectMap threadIDs;
  return threadIDs;
}

/**
 * Private method to add an algorithm reference to the thread id map.
 * It replaces any current reference with the same ID
 * @param threadID The current Python thread ID
 * @param alg A Python reference to the algorithm object
 */
void _trackAlgorithm(long threadID, const object &alg) { threadIDMap()[threadID] = alg; }

/**
 * Private method to remove an algorithm reference from the thread id map.
 * @param threadID The current Python thread ID
 */
void _forgetAlgorithm(long threadID) { threadIDMap().erase(threadID); }

/**
 * Return the algorithm object for the given thread ID or None
 * if one doesn't exist. The entry is removed from the map
 * if it is found
 */
object _algorithmInThread(long threadID) {
  auto &threadMap = threadIDMap();
  auto it = threadMap.find(threadID);
  if (it != threadMap.end()) {
    auto value = it->second;
    threadMap.erase(it);
    return value;
  } else {
    return object();
  }
}

/// Functor for use with sorting algorithm to put the properties that do not
/// have valid values first
struct MandatoryFirst {
  /// Comparator operator for sort algorithm, places optional properties lower
  /// in the list
  bool operator()(const Property *p1, const Property *p2) const {
    // this is false, unless p1 is not valid and p2 is valid
    return (!p1->isValid().empty()) && (p2->isValid().empty());
  }
};

//----------------------- Property ordering ------------------------------
/// Vector of property pointers
using PropertyVector = std::vector<Property *>;

/**
 * Returns the vector of properties ordered by the criteria defined in
 * MandatoryFirst.
 * A stable_sort is applied to the properties to guarantee the relative order
 * of with respect to the original list.
 * @return A list of Property pointers ordered by for the function API
 */
PropertyVector apiOrderedProperties(const IAlgorithm &propMgr) {
  PropertyVector properties(propMgr.getProperties()); // Makes a copy so that it can be sorted
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

list getInputPropertiesWithMandatoryFirst(const IAlgorithm &self) {
  PropertyVector properties(apiOrderedProperties(self));

  GlobalInterpreterLock gil;
  list names;
  ToPyString toPyStr;
  for (const auto &prop : properties) {
    if (prop->direction() != Direction::Output) {
      names.append(handle<>(toPyStr(prop->name())));
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
list getAlgorithmPropertiesOrdered(const IAlgorithm &self) {
  PropertyVector properties(apiOrderedProperties(self));

  GlobalInterpreterLock gil;
  list names;
  ToPyString toPyStr;
  for (const auto &prop : properties) {
    names.append(handle<>(toPyStr(prop->name())));
  }
  return names;
}

/**
 * Returns a list of output property names in the order they were declared in
 * @param self :: A pointer to the python object wrapping and Algorithm.
 * @return A Python list of strings
 */
list getOutputProperties(const IAlgorithm &self) {
  const PropertyVector &properties(self.getProperties()); // No copy

  GlobalInterpreterLock gil;
  list names;
  ToPyString toPyStr;
  for (const auto &p : properties) {
    if (p->direction() == Direction::Output) {
      names.append(handle<>(toPyStr(p->name())));
    }
  }
  return names;
}

/**
 * Returns a list of inout property names in the order they were declared
 * @param self :: A pointer to the python object wrapping and Algorithm.
 * @return A Python list of strings
 */
list getInOutProperties(const IAlgorithm &self) {
  const PropertyVector &properties(self.getProperties()); // No copy

  GlobalInterpreterLock gil;
  list names;
  ToPyString toPyStr;
  for (const auto &p : properties) {
    if (p->direction() == Direction::InOut) {
      names.append(handle<>(toPyStr(p->name())));
    }
  }
  return names;
}

/**
 * Create a doc string for the simple API
 * @param self :: A pointer to the python object wrapping and Algorithm
 * @return A string that documents an algorithm
 */
std::string createDocString(const IAlgorithm &self) {
  const std::string EOL = "\n";

  // Put in the quick overview message
  std::stringstream buffer;
  std::string temp = self.summary();
  if (!temp.empty())
    buffer << temp << EOL << EOL;

  // get a sorted copy of the properties
  PropertyVector properties(apiOrderedProperties(self));

  const size_t numProps(properties.size());

  buffer << "Property descriptions: " << EOL << EOL;
  // write the actual property descriptions
  for (size_t i = 0; i < numProps; ++i) {
    const Mantid::Kernel::Property *prop = properties[i];
    buffer << prop->name() << "(" << Mantid::Kernel::Direction::asText(prop->direction());
    if (!prop->isValid().empty())
      buffer << ":req";
    buffer << ") *" << prop->type() << "* ";
    std::vector<std::string> allowed = prop->allowedValues();
    if (!prop->documentation().empty() || !allowed.empty()) {
      buffer << "      " << prop->documentation();
      if (!allowed.empty()) {
        buffer << "[" << Mantid::Kernel::Strings::join(allowed.begin(), allowed.end(), ", ");
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
  explicit AllowCThreads(const object &algm)
      : m_tracefunc(nullptr), m_tracearg(nullptr), m_saved(nullptr), m_tracking(false) {
    const PyThreadState *curThreadState = PyThreadState_GET();
    m_tracefunc = curThreadState->c_tracefunc;
    m_tracearg = curThreadState->c_traceobj;
    Py_XINCREF(m_tracearg);
    PyEval_SetTrace(nullptr, nullptr);
    if (!isNone(algm)) {
      _trackAlgorithm(curThreadState->thread_id, algm);
      m_tracking = true;
    }
    m_saved = PyEval_SaveThread();
  }
  ~AllowCThreads() {
    PyEval_RestoreThread(m_saved);
    if (m_tracking) {
      _forgetAlgorithm(m_saved->thread_id);
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
 * Execute the algorithm asynchronously
 * @param self :: A reference to the calling object
 */
void executeAsync(const object &self) {
  auto &calg = extract<IAlgorithm &>(self)();
  calg.executeAsync();
}

/**
 * @param self A reference to the calling object
 * @return An AlgorithmID wrapped in a AlgorithmIDProxy container or None if
 * there is no ID
 */
PyObject *getAlgorithmID(const IAlgorithm &self) {
  AlgorithmID id = self.getAlgorithmID();
  if (id)
    return to_python_value<AlgorithmIDProxy>()(AlgorithmIDProxy(id));
  Py_RETURN_NONE;
}

//--------------------------------------------------------------------------------------
// Deprecated wrappers
//--------------------------------------------------------------------------------------
/**
 * @param self Reference to the calling object
 * @return Algorithm summary
 */
std::string getOptionalMessage(const IAlgorithm &self) {
  PyErr_Warn(PyExc_DeprecationWarning, ".getOptionalMessage() is deprecated. Use .summary() instead.");
  return self.summary();
}

/**
 * @param self Reference to the calling object
 * @return Algorithm summary
 */
std::string getWikiSummary(const IAlgorithm &self) {
  PyErr_Warn(PyExc_DeprecationWarning, ".getWikiSummary() is deprecated. Use .summary() instead.");
  return self.summary();
}

/**
 * @param self Reference to the calling object
 * @return validation error dictionary
 */
boost::python::dict validateInputs(IAlgorithm &self) {
  auto map = self.validateInputs();
  using MapToPyDictionary = Mantid::PythonInterface::Converters::MapToPyDictionary<std::string, std::string>;
  return MapToPyDictionary(map)();
}

void initializeProxy(IAlgorithm &self) {
  Mantid::PythonInterface::ReleaseGlobalInterpreterLock releaseGlobalInterpreterLock;
  self.initialize();
}

} // namespace

void export_ialgorithm() {
  class_<AlgorithmIDProxy>("AlgorithmID", no_init).def(self == self);

  register_ptr_to_python<std::shared_ptr<IAlgorithm>>();
  register_ptr_to_python<std::shared_ptr<const IAlgorithm>>();

  class_<IAlgorithm, bases<IPropertyManager>, boost::noncopyable>("IAlgorithm", "Interface for all algorithms", no_init)
      .def("name", &IAlgorithm::name, arg("self"), "Returns the name of the algorithm")
      .def("alias", &IAlgorithm::alias, arg("self"), "Return the aliases for the algorithm")
      .def("aliasDeprecated", &IAlgorithm::aliasDeprecated, arg("self"),
           "Deprecation date (in ISO8601 format) for the algorithm aliases. "
           "Returns empty string if no deprecation date")
      .def("version", &IAlgorithm::version, arg("self"), "Returns the version number of the algorithm")
      .def("cancel", &IAlgorithm::cancel, arg("self"), "Request that the algorithm stop running")
      .def("category", &IAlgorithm::category, arg("self"), "Returns the category containing the algorithm")
      .def("categories", &IAlgorithm::categories, arg("self"), return_value_policy<VectorToNumpy>(),
           "Returns the list of categories this algorithm belongs to")
      .def("seeAlso", &IAlgorithm::seeAlso, arg("self"), return_value_policy<VectorToNumpy>(),
           "Returns the list of similar algorithms")
      .def("summary", &IAlgorithm::summary, arg("self"), "Returns a summary message describing the algorithm")
      .def("helpURL", &IAlgorithm::helpURL, arg("self"), "Returns optional URL for algorithm documentation")
      .def("workspaceMethodName", &IAlgorithm::workspaceMethodName, arg("self"),
           "Returns a name that will be used when attached as a workspace "
           "method. Empty string indicates do not attach")
      .def("workspaceMethodOn", &IAlgorithm::workspaceMethodOn, arg("self"),
           return_value_policy<VectorToNumpy>(), // creates a list for strings
           "Returns a set of class names that will have the method attached. "
           "Empty list indicates all types")
      .def("workspaceMethodInputProperty", &IAlgorithm::workspaceMethodInputProperty, arg("self"),
           "Returns the name of the input workspace property used by the "
           "calling object")
      .def("getAlgorithmID", &getAlgorithmID, arg("self"), "Returns a unique identifier for this algorithm object")
      .def("docString", &createDocString, arg("self"), "Returns a doc string for the algorithm")
      .def("mandatoryProperties", &getInputPropertiesWithMandatoryFirst, arg("self"),
           "Returns a list of input and in/out property names that is ordered "
           "such that the mandatory properties are first followed by the "
           "optional ones.")
      .def("orderedProperties", &getAlgorithmPropertiesOrdered, arg("self"),
           "Return a list of input, in/out and output properties "
           "such that the mandatory properties are first followed by the "
           "optional ones.")
      .def("outputProperties", &getOutputProperties, arg("self"),
           "Returns a list of the output properties on the algorithm")
      .def("inoutProperties", &getInOutProperties, arg("self"),
           "Returns a list of the inout properties on the algorithm")
      .def("isInitialized", &IAlgorithm::isInitialized, arg("self"),
           "Returns True if the algorithm is initialized, False otherwise")
      .def("isExecuted", &IAlgorithm::isExecuted, arg("self"),
           "Returns True if the algorithm has been executed successfully, "
           "False otherwise")
      .def("isLogging", &IAlgorithm::isLogging, arg("self"),
           "Returns True if the "
           "algorithm's logger is turned "
           "on, False otherwise")
      .def("isRunning", &IAlgorithm::isRunning, arg("self"),
           "Returns True if the algorithm "
           "is considered to be running, "
           "False otherwise")
      .def("setChild", &IAlgorithm::setChild, (arg("self"), arg("is_child")),
           "If true this algorithm is run as a child algorithm. There will be "
           "no logging and nothing is stored in the Analysis Data Service")
      .def("enableHistoryRecordingForChild", &IAlgorithm::enableHistoryRecordingForChild, (arg("self"), arg("on")),
           "If true then history will be recorded regardless of the child "
           "status")
      .def("setAlgStartupLogging", &IAlgorithm::setAlgStartupLogging, (arg("self"), arg("enabled")),
           "If true then allow logging of start and end messages")
      .def("getAlgStartupLogging", &IAlgorithm::getAlgStartupLogging, arg("self"),
           "Returns true if logging of start and end messages")
      .def("setAlwaysStoreInADS", &IAlgorithm::setAlwaysStoreInADS, (arg("self"), arg("do_store")),
           "If true then even child algorithms will have their workspaces "
           "stored in the ADS.")
      .def("isChild", &IAlgorithm::isChild, arg("self"),
           "Returns True if the algorithm has been marked to run as a child. "
           "If True then Output workspaces "
           "are NOT stored in the Analysis Data Service but must be retrieved "
           "from the property.")
      .def("setLogging", &IAlgorithm::setLogging, (arg("self"), arg("value")), "Toggle logging on/off.")
      .def("setRethrows", &IAlgorithm::setRethrows, (arg("self"), arg("rethrow")),
           "To query whether an algorithm "
           "should rethrow exceptions when "
           "executing.")
      .def("initialize", &initializeProxy, arg("self"), "Initializes the algorithm")
      .def("validateInputs", &validateInputs, arg("self"),
           "Cross-check all inputs and return any errors as a dictionary")
      .def("execute", &executeProxy, arg("self"), "Runs the algorithm and returns whether it has been successful")
      .def("executeAsync", &executeAsync, arg("self"),
           "Starts the algorithm in a separate thread and returns immediately")
      // 'Private' static methods
      .def("_algorithmInThread", &_algorithmInThread, arg("thread_id"))
      .staticmethod("_algorithmInThread")
      // Special methods
      .def("__str__", &IAlgorithm::toString, arg("self"))

      // deprecated methods
      .def("getOptionalMessage", &getOptionalMessage, arg("self"),
           "Returns the optional user message attached to the algorithm")
      .def("getWikiSummary", &getWikiSummary, arg("self"), "Returns the summary found on the wiki page");
}
