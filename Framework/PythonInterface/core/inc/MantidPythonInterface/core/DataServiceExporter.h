// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DataService.h"
#include "MantidKernel/Exception.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"
#include "MantidPythonInterface/core/WeakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/list.hpp>
#include <boost/python/str.hpp>

#include <memory>
#include <set>

namespace Mantid {
namespace PythonInterface {
/**
 * A helper struct to export templated DataService<> types to Python.
 * @tparam SvcType Type of DataService to export
 * @tparam SvcHeldType The type held within the DataService map
 */
template <typename SvcType, typename SvcPtrType> struct DataServiceExporter {
  // typedef the type created by boost.python
  using PythonType = boost::python::class_<SvcType, boost::noncopyable>;
  using WeakPtr = std::weak_ptr<typename SvcPtrType::element_type>;

  /**
   * Define the necessary boost.python framework to expor the templated
   * DataService type
   * Note: This does not add the Instance method as that is on the
   * SingletonHolder typedef.
   * To add this call the following methods on the object this function returns
   *     .def("Instance", &SingletonType::Instance,
   * return_value_policy<reference_existing_object>(),
   *          "Return a reference to the ADS singleton")
   *     .staticmethod("Instance")
   * where SingletonType is the typedef of the SingletonHolder, e.g.
   * AnalysisDataService
   * @param pythonClassName The name that the class should have in Python
   * @returns The new class object that wraps the given C++ type
   */
  static PythonType define(const char *pythonClassName) {
    using namespace boost::python;
    using namespace Mantid::Kernel;

    auto classType =
        PythonType(pythonClassName, no_init)
            .def("add", &DataServiceExporter::addItem, (arg("self"), arg("name"), arg("item")),
                 "Adds the given object to the service with the given name. If "
                 "the name/object exists it will raise an error.")
            .def("addOrReplace", &DataServiceExporter::addOrReplaceItem, (arg("self"), arg("name"), arg("item")),
                 "Adds the given object to the service with the given name. "
                 "If the name exists the object is replaced.")
            .def("doesExist", &SvcType::doesExist, (arg("self"), arg("name")),
                 "Returns True if the object is found in the service.")
            .def("retrieve", &DataServiceExporter::retrieveOrKeyError, (arg("self"), arg("name")),
                 "Retrieve the named object. Raises an exception if the name "
                 "does not exist")
            .def("remove", &DataServiceExporter::removeItem, (arg("self"), arg("name")), "Remove a named object")
            .def("clear", &DataServiceExporter::clearItems, (arg("self"), arg("silent") = false),
                 "Removes all objects managed by the service.")
            .def("size", &SvcType::size, arg("self"), "Returns the number of objects within the service")
            .def("getObjectNames", &DataServiceExporter::getObjectNamesAsList, (arg("self"), arg("contain") = ""),
                 "Return the list of names currently known to the ADS")

            // Make it act like a dictionary
            .def("__len__", &SvcType::size, arg("self"))
            .def("__getitem__", &DataServiceExporter::retrieveOrKeyError, (arg("self"), arg("name")))
            .def("__setitem__", &DataServiceExporter::addOrReplaceItem, (arg("self"), arg("name"), arg("item")))
            .def("__contains__", &SvcType::doesExist, (arg("self"), arg("name")))
            .def("__delitem__", &DataServiceExporter::removeItem, (arg("self"), arg("name")));

    return classType;
  }

  /**
   * Add an item into the service, if it exists then an error is raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  static void addItem(SvcType &self, const std::string &name, const boost::python::object &item) {
    ReleaseGlobalInterpreterLock releaseGIL;
    self.add(name, extractCppValue(item));
  }

  /**
   * Add or replace an item into the service, if it exists then an error is
   * raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  static void addOrReplaceItem(SvcType &self, const std::string &name, const boost::python::object &item) {
    ReleaseGlobalInterpreterLock releaseGIL;
    self.addOrReplace(name, extractCppValue(item));
  }

  /**
   * Remove an item from the service
   * @param self A reference to the calling object
   * @param name The name of the item in the service to remove
   */
  static void removeItem(SvcType &self, const std::string &name) {
    ReleaseGlobalInterpreterLock releaseGIL;
    self.remove(name);
  }

  /**
   * Remove an item from the service
   * @param self A reference to the calling object
   * @param silent A flag to silence the warning message
   */
  static void clearItems(SvcType &self, const bool silent) {
    if (self.size() > 0 && !silent) {
      PyErr_Warn(PyExc_Warning, "Running ADS.clear() also removes all hidden workspaces.\n"
                                "Mantid interfaces might still need some of these, for instance, MSlice.");
    }

    ReleaseGlobalInterpreterLock releaseGIL;
    self.clear();
  }

  /**
   * Extract a SvcPtrType C++ value from the Python object
   * @param pyvalue Value of the
   * @return The extracted value or thows an std::invalid_argument error
   */
  static SvcPtrType extractCppValue(const boost::python::object &pyvalue) {
    // Test for a weak pointer first
    boost::python::extract<WeakPtr &> extractWeak(pyvalue);
    if (extractWeak.check()) {
      return extractWeak().lock();
    }
    boost::python::extract<SvcPtrType &> extractRefShared(pyvalue);
    if (extractRefShared.check()) {
      return extractRefShared();
    } else {
      throw std::invalid_argument("Cannot extract pointer from Python object argument. Incorrect type");
    }
  }

  /**
   * Retrieves a shared_ptr from the ADS and raises a Python KeyError if it does
   * not exist
   * @param self :: A pointer to the object calling this function. Allows it to
   * act
   * as a member function
   * @param name :: The name of the object to retrieve
   * @return A shared_ptr to the named object. If the name does not exist it
   * sets a KeyError error indicator.
   */
  static WeakPtr retrieveOrKeyError(const SvcType &self, const std::string &name) {
    using namespace Mantid::Kernel;

    SvcPtrType item;
    try {
      item = self.retrieve(name);
    } catch (Exception::NotFoundError &) {
      // Translate into a Python KeyError
//      std::string err = "'" + name + "' does not exist.";
//      PyErr_SetString(PyExc_KeyError, err.c_str());
      throw boost::python::error_already_set();
    }
    return WeakPtr(item);
  }

  /**
   * Return a Python list of object names from the ADS as this is
   * far easier to work with than a set
   * @param self :: A reference to the ADS object that called this method
   * @param contain :: If provided, the function will return only names that
   * contain this string
   * @returns A python list created from the set of strings
   */
  static boost::python::list getObjectNamesAsList(SvcType const *const self, const std::string &contain) {
    boost::python::list names;
    const auto keys = self->getObjectNames(Mantid::Kernel::DataServiceSort::Unsorted,
                                           Mantid::Kernel::DataServiceHidden::Auto, contain);
    for (auto itr = keys.begin(); itr != keys.end(); ++itr) {
      names.append(*itr);
    }
    assert(names.attr("__len__")() == keys.size());
    return names;
  }
};
} // namespace PythonInterface
} // namespace Mantid
