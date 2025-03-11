// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FileLoaderRegistry.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/PythonObjectInstantiator.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"
#include "MantidPythonInterface/core/UninstallTrace.h"

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
using Mantid::PythonInterface::PythonObjectInstantiator;
using Mantid::PythonInterface::ReleaseGlobalInterpreterLock;
using Mantid::PythonInterface::UninstallTrace;

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
dict getRegisteredAlgorithms(AlgorithmFactoryImpl const *const self, bool includeHidden) {
  const auto keys = self->getKeys(includeHidden);
  dict inventory;
  for (const auto &key : keys) {
    std::pair<std::string, int> algInfo;
    {
      // We need to release the GIL here to prevent a deadlock when using Python log channels
      ReleaseGlobalInterpreterLock releaseGIL;
      algInfo = self->decodeName(key);
    }
    object name(handle<>(to_python_value<const std::string &>()(algInfo.first)));
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

/**
 * Return algorithm descriptors as a python list of lists.
 * @param self :: An instance of AlgorithmFactory.
 * @param includeHidden :: If true hidden algorithms are included.
 */
list getDescriptors(AlgorithmFactoryImpl const *const self, bool includeHidden = false, bool includeAlias = false) {
  const auto descriptors = self->getDescriptors(includeHidden, includeAlias);
  list pyDescriptors;
  for (const auto &descr : descriptors) {
    pyDescriptors.append(boost::python::object(descr));
  }
  return pyDescriptors;
}

/**
 * A Python friendly version of getCategoriesWithState
 * Return the categories of the algorithms. This includes those within the
 * Factory itself and any cleanly constructed algorithms stored here.
 * @param self :: Enables it to be called as a member function on the
 * AlgorithmFactory class
 * @returns The map of the categories, together with a true false value
 * defining if they are hidden
 */
dict getCategoriesandState(AlgorithmFactoryImpl const *const self) {
  const auto categories = self->getCategoriesWithState();
  dict pythonCategories;
  for (auto &it : categories) {
    object categoryName(handle<>(to_python_value<const std::string &>()(it.first)));
    pythonCategories[categoryName] = it.second;
  }

  return pythonCategories;
}

//------------------------------------------------------------------------------
// Python algorithm subscription
//------------------------------------------------------------------------------

// Python algorithm registration mutex in anonymous namespace (aka static)
std::recursive_mutex PYALG_REGISTER_MUTEX;

GNU_DIAG_OFF("cast-qual")

/**
 * A free function to subscribe a Python algorithm into the factory
 * @param obj :: A Python object that should either be a class type derived from
 * PythonAlgorithm
 *              or an instance of a class type derived from PythonAlgorithm
 */
void subscribe(AlgorithmFactoryImpl &self, const boost::python::object &obj) {
  UninstallTrace uninstallTrace;
  std::lock_guard<std::recursive_mutex> lock(PYALG_REGISTER_MUTEX);

  static auto *const pyAlgClass = (PyObject *)converter::registered<Algorithm>::converters.to_python_target_type();
  // obj could be or instance/class, check instance first
  PyObject *classObject(nullptr);
  if (PyObject_IsInstance(obj.ptr(), pyAlgClass)) {
    classObject = PyObject_GetAttrString(obj.ptr(), "__class__");
  } else if (PyObject_IsSubclass(obj.ptr(), pyAlgClass)) {
    classObject = obj.ptr(); // We need to ensure the type of lifetime
                             // management so grab the raw pointer
  } else {
    throw std::invalid_argument("Cannot register an algorithm that does not derive from Algorithm.");
  }
  boost::python::object classType(handle<>(borrowed(classObject)));
  // Takes ownership of instantiator and replaces any existing algorithm
  std::unique_ptr<Mantid::Kernel::AbstractInstantiator<Algorithm>> temp =
      std::make_unique<PythonObjectInstantiator<Algorithm>>(classType);
  auto descr = self.subscribe(std::move(temp), AlgorithmFactoryImpl::OverwriteCurrent);

  // Python algorithms cannot yet act as loaders so remove any registered ones
  // from the FileLoaderRegistry
  FileLoaderRegistry::Instance().unsubscribe(descr.first, descr.second);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")

// cppcheck-suppress unknownMacro
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(existsOverloader, exists, 1, 2)
BOOST_PYTHON_FUNCTION_OVERLOADS(getDescriptors_overloads, getDescriptors, 1, 3)

GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")

///@endcond
} // namespace
GNU_DIAG_ON("cast-qual")

void export_AlgorithmFactory() {

  class_<AlgorithmDescriptor>("AlgorithmDescriptor")
      .def_readonly("name", &AlgorithmDescriptor::name)
      .def_readonly("alias", &AlgorithmDescriptor::alias)
      .def_readonly("category", &AlgorithmDescriptor::category)
      .def_readonly("version", &AlgorithmDescriptor::version);

  class_<AlgorithmFactoryImpl, boost::noncopyable>("AlgorithmFactoryImpl", no_init)
      .def("exists", &AlgorithmFactoryImpl::exists,
           existsOverloader((arg("name"), arg("version") = -1), "Returns true if the given algorithm exists with "
                                                                "an option to specify the version"))

      .def("getRegisteredAlgorithms", &getRegisteredAlgorithms, (arg("self"), arg("include_hidden")),
           "Returns a Python dictionary of currently registered algorithms")
      .def("highestVersion", &AlgorithmFactoryImpl::highestVersion, (arg("self"), arg("algorithm_name")),
           "Returns the highest version of the named algorithm. Throws "
           "ValueError if no algorithm can be found")
      .def("subscribe", &subscribe, (arg("self"), arg("object")),
           "Register a Python class derived from "
           "PythonAlgorithm into the factory")
      .def("getDescriptors", &getDescriptors,
           getDescriptors_overloads((arg("self"), arg("include_hidden") = false, arg("include_alias") = false),
                                    "Return a list of descriptors of registered algorithms. Each "
                                    "descriptor is a list: [name, version, category, alias]."))
      .def("getCategoriesandState", &getCategoriesandState,
           "Return the categories of the algorithms. This includes those within "
           "the Factory itself and any cleanly constructed algorithms stored "
           "here")
      .def("unsubscribe", &AlgorithmFactoryImpl::unsubscribe, (arg("self"), arg("name"), arg("version")),
           "Returns the highest version of the named algorithm. Throws "
           "ValueError if no algorithm can be found")
      .def("enableNotifications", &AlgorithmFactoryImpl::enableNotifications)
      .def("disableNotifications", &AlgorithmFactoryImpl::disableNotifications)

      .def("Instance", &AlgorithmFactory::Instance, return_value_policy<reference_existing_object>(),
           "Returns a reference to the AlgorithmFactory singleton")
      .staticmethod("Instance");
}
