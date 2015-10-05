#ifndef MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_
#define MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_

/*
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
   National Laboratory & European Spallation Source

    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
#include "MantidKernel/Exception.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/extract.hpp>

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
  typedef boost::python::class_<SvcType, boost::noncopyable> PythonType;
  typedef boost::weak_ptr<typename SvcPtrType::element_type> WeakPtr;

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
            .def("add", &DataServiceExporter::addItem,
                 "Adds the given object to the service with the given name. If "
                 "the name/object exists it will raise an error.")
            .def("addOrReplace", &DataServiceExporter::addOrReplaceItem,
                 "Adds the given object to the service with the given name. "
                 "The the name exists the object is replaced.")
            .def("doesExist", &SvcType::doesExist,
                 "Returns True if the object is found in the service.")
            .def("retrieve", &DataServiceExporter::retrieveOrKeyError,
                 "Retrieve the named object. Raises an exception if the name "
                 "does not exist")
            .def("remove", &SvcType::remove, "Remove a named object")
            .def("clear", &SvcType::clear,
                 "Removes all objects managed by the service.")
            .def("size", &SvcType::size,
                 "Returns the number of objects within the service")
            .def("getObjectNames", &DataServiceExporter::getObjectNamesAsList,
                 "Return the list of names currently known to the ADS")

            // Make it act like a dictionary
            .def("__len__", &SvcType::size)
            .def("__getitem__", &DataServiceExporter::retrieveOrKeyError)
            .def("__setitem__", &DataServiceExporter::addOrReplaceItem)
            .def("__contains__", &SvcType::doesExist)
            .def("__delitem__", &SvcType::remove);

    return classType;
  }

  /**
   * Add an item into the service, if it exists then an error is raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  static void addItem(SvcType &self, const std::string &name,
                      const boost::python::object &item) {
    self.add(name, extractCppValue(item));
  }

  /**
   * Add or replace an item into the service, if it exists then an error is
   * raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  static void addOrReplaceItem(SvcType &self, const std::string &name,
                               const boost::python::object &item) {
    self.addOrReplace(name, extractCppValue(item));
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
    boost::python::extract<SvcPtrType &> extractShared(pyvalue);
    if (extractShared.check()) {
      return extractShared();
    } else {
      throw std::invalid_argument(
          "Cannot extract pointer from Python object argument. Incorrect type");
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
  static WeakPtr retrieveOrKeyError(SvcType &self, const std::string &name) {
    using namespace Mantid::Kernel;
    SvcPtrType item;
    try {
      item = self.retrieve(name);
    } catch (Exception::NotFoundError &) {
      // Translate into a Python KeyError
      std::string err = "'" + name + "' does not exist.";
      PyErr_SetString(PyExc_KeyError, err.c_str());
      throw boost::python::error_already_set();
    }
    return WeakPtr(item);
  }

  /**
   * Return a Python list of object names from the ADS as this is
   * far easier to work with than a set
   * @param self :: A reference to the ADS object that called this method
   * @returns A python list created from the set of strings
   */
  static boost::python::object getObjectNamesAsList(SvcType &self) {
    boost::python::list names;
    const std::set<std::string> keys = self.getObjectNames();
    std::set<std::string>::const_iterator iend = keys.end();
    for (std::set<std::string>::const_iterator itr = keys.begin(); itr != iend;
         ++itr) {
      names.append(*itr);
    }
    assert(names.attr("__len__")() == keys.size());
    return names;
  }
};
}
}

#endif /* MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_ */
