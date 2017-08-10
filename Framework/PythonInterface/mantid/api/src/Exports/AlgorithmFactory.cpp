#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidPythonInterface/kernel/PythonObjectInstantiator.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <mutex>

// Python frameobject. This is under the boost includes so that boost will have
// done the
// include of Python.h which it ensures is done correctly
#include <frameobject.h>

using namespace Mantid::API;
using namespace boost::python;
using Mantid::Kernel::AbstractInstantiator;
using Mantid::PythonInterface::PythonObjectInstantiator;

GET_POINTER_SPECIALIZATION(AlgorithmFactoryImpl)

namespace {
///@cond

//------------------------------------------------------------------------------------------------------
/**
 * A Python friendly version of get_keys that returns the registered algorithms
 * as
 * a dictionary where the key is the algorithm name and the value is a list
 * of version numbers
 * @param self :: Enables it to be called as a member function on the
 * AlgorithmFactory class
 * @param includeHidden :: If true hidden algorithms are included
 */
dict getRegisteredAlgorithms(AlgorithmFactoryImpl &self, bool includeHidden) {
  std::vector<std::string> keys = self.getKeys(includeHidden);
  const size_t nkeys = keys.size();
  dict inventory;
  for (size_t i = 0; i < nkeys; ++i) {
    auto algInfo = self.decodeName(keys[i]);
    object name(
        handle<>(to_python_value<const std::string &>()(algInfo.first)));
    object ver(handle<>(to_python_value<const int &>()(algInfo.second)));
    // There seems to be no way to "promote" the return of .get to a list
    // without copying it
    object versions;
    if (inventory.has_key(name)) {
      versions = inventory.get(name);
    } else {
      versions = list();
      inventory[name] = versions;
    }
    versions.attr("append")(ver);
  }
  return inventory;
}

//------------------------------------------------------------------------------
// Python algorithm subscription
//------------------------------------------------------------------------------

// Python algorithm registration mutex in anonymous namespace (aka static)
std::recursive_mutex PYALG_REGISTER_MUTEX;

// clang-format off
GCC_DIAG_OFF(cast-qual)
// clang-format on

/**
 * A free function to subscribe a Python algorithm into the factory
 * @param obj :: A Python object that should either be a class type derived from
 * PythonAlgorithm
 *              or an instance of a class type derived from PythonAlgorithm
 */
void subscribe(AlgorithmFactoryImpl &self, const boost::python::object &obj) {
  std::lock_guard<std::recursive_mutex> lock(PYALG_REGISTER_MUTEX);

  static PyObject *const pyAlgClass =
      (PyObject *)
          converter::registered<Algorithm>::converters.to_python_target_type();
  // obj could be or instance/class, check instance first
  PyObject *classObject(nullptr);
  if (PyObject_IsInstance(obj.ptr(), pyAlgClass)) {
    classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
  } else if (PyObject_IsSubclass(obj.ptr(), pyAlgClass)) {
    classObject = obj.ptr(); // We need to ensure the type of lifetime
                             // management so grab the raw pointer
  } else {
    throw std::invalid_argument(
        "Cannot register an algorithm that does not derive from Algorithm.");
  }
  boost::python::object classType(handle<>(borrowed(classObject)));
  // Takes ownership of instantiator and replaces any existing algorithm
  auto descr =
      self.subscribe(new PythonObjectInstantiator<Algorithm>(classType),
                     AlgorithmFactoryImpl::OverwriteCurrent);

  // Python algorithms cannot yet act as loaders so remove any registered ones
  // from the FileLoaderRegistry
  FileLoaderRegistry::Instance().unsubscribe(descr.first, descr.second);
}
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GCC_DIAG_OFF(conversion)
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(existsOverloader, exists, 1, 2)
GCC_DIAG_ON(conversion)
#ifdef __clang__
#pragma clang diagnostic pop
#endif

///@endcond
}
// clang-format off
GCC_DIAG_ON(cast-qual)
// clang-format on

void export_AlgorithmFactory() {

  class_<AlgorithmFactoryImpl, boost::noncopyable>("AlgorithmFactoryImpl",
                                                   no_init)
      .def("exists", &AlgorithmFactoryImpl::exists,
           existsOverloader((arg("name"), arg("version") = -1),
                            "Returns true if the given algorithm exists with "
                            "an option to specify the version"))

      .def("getRegisteredAlgorithms", &getRegisteredAlgorithms,
           (arg("self"), arg("include_hidden")),
           "Returns a Python dictionary of currently registered algorithms")
      .def("highestVersion", &AlgorithmFactoryImpl::highestVersion,
           (arg("self"), arg("algorithm_name")),
           "Returns the highest version of the named algorithm. Throws "
           "ValueError if no algorithm can be found")
      .def("subscribe", &subscribe, (arg("self"), arg("object")),
           "Register a Python class derived from "
           "PythonAlgorithm into the factory")

      .def("Instance", &AlgorithmFactory::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the AlgorithmFactory singleton")
      .staticmethod("Instance");
}
