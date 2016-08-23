#include "MantidPythonInterface/api/PythonAlgorithm/AlgorithmAdapter.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"
#include "MantidPythonInterface/kernel/Environment/WrapperHelpers.h"
#include "MantidPythonInterface/kernel/Environment/CallMethod.h"
#include "MantidPythonInterface/kernel/Environment/GlobalInterpreterLock.h"
#include "MantidAPI/DataProcessorAlgorithm.h"

#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>

//-----------------------------------------------------------------------------
// AlgorithmAdapter definition
//-----------------------------------------------------------------------------
namespace Mantid {
namespace PythonInterface {
using namespace boost::python;
using Environment::callMethod;
using Environment::UndefinedAttributeError;

/**
 * Construct the "wrapper" and stores the reference to the PyObject
 * @param self A reference to the calling Python object
 */
template <typename BaseAlgorithm>
AlgorithmAdapter<BaseAlgorithm>::AlgorithmAdapter(PyObject *self)
    : BaseAlgorithm(), m_self(self), m_isRunningObj(nullptr),
      m_wikiSummary("") {
  // Only cache the isRunning attribute if it is overridden by the
  // inheriting type otherwise we end up with an infinite recursive call
  // as isRunning always exists from the interface
  if (Environment::typeHasAttribute(self, "isRunning"))
    m_isRunningObj = PyObject_GetAttrString(self, "isRunning");
}

/**
 * Returns the name of the algorithm. This cannot be overridden in Python.
 */
template <typename BaseAlgorithm>
const std::string AlgorithmAdapter<BaseAlgorithm>::name() const {
  return std::string(getSelf()->ob_type->tp_name);
}

/**
 * Returns the version of the algorithm. If not overridden
 * it returns 1
 */
template <typename BaseAlgorithm>
int AlgorithmAdapter<BaseAlgorithm>::version() const {
  try {
    return callMethod<int>(getSelf(), "version");
  } catch (UndefinedAttributeError &) {
    return 1;
  }
}

/**
 * Returns checkGroups. If false, workspace groups will be treated as a whole
 * If true, the algorithm will act on each component of the workspace group
 * individually
 */
template <typename BaseAlgorithm>
bool AlgorithmAdapter<BaseAlgorithm>::checkGroups() {
  try {
    return callMethod<bool>(getSelf(), "checkGroups");
  } catch (UndefinedAttributeError &) {
    return BaseAlgorithm::checkGroups();
  }
}

/**
 * Returns the category of the algorithm. If not overridden
 * it return defaultCategory()
 */
template <typename BaseAlgorithm>
const std::string AlgorithmAdapter<BaseAlgorithm>::category() const {
  const static std::string defaultCategory = "PythonAlgorithms";
  std::string category = defaultCategory;
  try {
    category = callMethod<std::string>(getSelf(), "category");
  } catch (UndefinedAttributeError &) {
  }
  if (category == defaultCategory) {
    // output a warning
    this->getLogger().warning()
        << "Python Algorithm " << this->name() << " v" << this->version()
        << " does not have a category defined. See "
           "http://www.mantidproject.org/Basic_PythonAlgorithm_Structure\n";
  }
  return category;
}

/**
 * Returns the summary of the algorithm. If not overridden
 * it returns defaultSummary
 */
template <typename BaseAlgorithm>
const std::string AlgorithmAdapter<BaseAlgorithm>::summary() const {
  try {
    return callMethod<std::string>(getSelf(), "summary");
  } catch (UndefinedAttributeError &) {
    return m_wikiSummary;
  }
}

/**
 * @return True if the algorithm is considered to be running
 */
template <typename BaseAlgorithm>
bool AlgorithmAdapter<BaseAlgorithm>::isRunning() const {
  if (!m_isRunningObj) {
    return SuperClass::isRunning();
  } else {
    Environment::GlobalInterpreterLock gil;
    PyObject *result = PyObject_CallObject(m_isRunningObj, nullptr);
    if (PyErr_Occurred())
      throw Environment::PythonException();
    if (PyBool_Check(result)) {
#if PY_MAJOR_VERSION >= 3
      return static_cast<bool>(PyLong_AsLong(result));
#else
      return static_cast<bool>(PyInt_AsLong(result));
#endif
    } else
      throw std::runtime_error(
          "Algorithm.isRunning - Expected bool return type.");
  }
}

/**
 */
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::cancel() {
  try {
    return callMethod<void>(getSelf(), "cancel");
  } catch (UndefinedAttributeError &) {
    SuperClass::cancel();
  }
}

/**
 * @copydoc Mantid::API::Algorithm::validateInputs
 */
template <typename BaseAlgorithm>
std::map<std::string, std::string>
AlgorithmAdapter<BaseAlgorithm>::validateInputs() {
  using boost::python::dict;
  std::map<std::string, std::string> resultMap;

  try {
    Environment::GlobalInterpreterLock gil;
    dict resultDict = callMethod<dict>(getSelf(), "validateInputs");
    // convert to a map<string,string>
    boost::python::list keys = resultDict.keys();
    size_t numItems = boost::python::len(keys);
    for (size_t i = 0; i < numItems; ++i) {
      boost::python::object value = resultDict[keys[i]];
      if (value) {
        try {
          std::string key = boost::python::extract<std::string>(keys[i]);
          std::string value =
              boost::python::extract<std::string>(resultDict[keys[i]]);
          resultMap[key] = value;
        } catch (boost::python::error_already_set &) {
          this->getLogger().error()
              << "In validateInputs(self): Invalid type for key/value pair "
              << "detected in dict.\n"
              << "All keys and values must be strings\n";
        }
      }
    }
  } catch (UndefinedAttributeError &) {
    return resultMap;
  }

  return resultMap;
}

/// Set the summary text
/// @param summary Wiki text
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::setWikiSummary(
    const std::string &summary) {
  std::string msg =
      "self.setWikiSummary() is deprecated and will be removed in a future "
      "release.\n"
      "To ensure continued functionality remove the line containing "
      "'self.setWikiSummary'\n"
      "and add a new function outside of the current one defined like so:\n"
      "def summary(self):\n"
      "    \"" +
      summary + "\"\n";

  PyErr_Warn(PyExc_DeprecationWarning, msg.c_str());
  m_wikiSummary = summary;
}

/**
 * Declare a preconstructed property.
 * @param self A reference to the calling Python object
 * @param prop :: A pointer to a property
 * @param doc :: An optional doc string
 */
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(
    boost::python::object &self, Kernel::Property *prop,
    const std::string &doc) {
  BaseAlgorithm &caller = extract<BaseAlgorithm &>(self);
  // We need to clone the property so that python doesn't own the object that
  // gets inserted
  // into the manager
  caller.declareProperty(std::unique_ptr<Kernel::Property>(prop->clone()), doc);
}

/**
 * Declare a property using the type of the defaultValue, a documentation
 * string
 * and validator
 * @param self A reference to the calling Python object
 * @param name :: The name of the new property
 * @param defaultValue :: A default value for the property. The type is mapped
 * to a C++ type
 * @param validator :: A validator object
 * @param doc :: The documentation string
 * @param direction :: The direction of the property
 */
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(
    boost::python::object &self, const std::string &name,
    const boost::python::object &defaultValue,
    const boost::python::object &validator, const std::string &doc,
    const int direction) {
  BaseAlgorithm &caller = extract<BaseAlgorithm &>(self);
  auto prop = std::unique_ptr<Kernel::Property>(
      Registry::PropertyWithValueFactory::create(name, defaultValue, validator,
                                                 direction));
  caller.declareProperty(std::move(prop), doc);
}

/**
 * Declare a property using the type of the defaultValue and a documentation
 * string
 * @param self A reference to the calling Python object
 * @param name :: The name of the new property
 * @param defaultValue :: A default value for the property. The type is mapped
 * to a C++ type
 * @param doc :: The documentation string
 * @param direction :: The direction of the property
 */
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(
    boost::python::object &self, const std::string &name,
    const boost::python::object &defaultValue, const std::string &doc,
    const int direction) {
  BaseAlgorithm &caller = extract<BaseAlgorithm &>(self);
  auto prop = std::unique_ptr<Kernel::Property>(
      Registry::PropertyWithValueFactory::create(name, defaultValue,
                                                 direction));
  caller.declareProperty(std::move(prop), doc);
}

/**
 * Declare a property using the type of the defaultValue
 * @param self A reference to the calling Python object
 * @param name :: The name of the new property
 * @param defaultValue :: A default value for the property. The type is mapped
 * to a C++ type
 * @param direction :: The direction of the property
 */
template <typename BaseAlgorithm>
void AlgorithmAdapter<BaseAlgorithm>::declarePyAlgProperty(
    boost::python::object &self, const std::string &name,
    const boost::python::object &defaultValue, const int direction) {
  declarePyAlgProperty(self, name, defaultValue, "", direction);
}

//---------------------------------------------------------------------------------------------
// Private members
//---------------------------------------------------------------------------------------------

/**
 * Private init for this algorithm. Expected to be
 * overridden in the subclass by a function named PyInit
 */
template <typename BaseAlgorithm> void AlgorithmAdapter<BaseAlgorithm>::init() {
  callMethod<void>(getSelf(), "PyInit");
}

/**
 * Private exec for this algorithm. Expected to be
 * overridden in the subclass by a function named PyExec
 */
template <typename BaseAlgorithm> void AlgorithmAdapter<BaseAlgorithm>::exec() {
  callMethod<void>(getSelf(), "PyExec");
}

//-----------------------------------------------------------------------------------------------------------------------------
// Concete instantiations (avoids definitions being all in the headers)
//-----------------------------------------------------------------------------------------------------------------------------
/// API::Algorithm as base
template class AlgorithmAdapter<API::Algorithm>;
/// API::DataProcesstor as base
template class AlgorithmAdapter<API::DataProcessorAlgorithm>;
}
}
