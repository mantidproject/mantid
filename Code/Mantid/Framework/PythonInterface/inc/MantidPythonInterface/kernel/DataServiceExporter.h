#ifndef MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_
#define MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_

/*
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
#include "MantidPythonInterface/kernel/Policies/DowncastingPolicies.h"
#include "MantidKernel/Exception.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>

#include <set>

namespace Mantid
{
  namespace PythonInterface
  {
    /**
     * A helper struct to export templated DataService<> types to Python.
     * @tparam SvcType Type of DataService to export
     * @tparam SvcHeldType The type held within the DataService map
     */
    template<typename SvcType, typename SvcHeldType>
    struct DataServiceExporter
    {
      /// typedef the type created by boost.python
      typedef boost::python::class_<SvcType,boost::noncopyable> PythonType;

      /**
       * Define the necessary boost.python framework to expor the templated DataService type
       * Note: This does not add the Instance method as that is on the SingletonHolder typedef.
       * To add this call the following methods on the object this function returns
       *     .def("Instance", &SingletonType::Instance, return_value_policy<reference_existing_object>(),
       *          "Return a reference to the ADS singleton")
       *     .staticmethod("Instance")
       * where SingletonType is the typedef of the SingletonHolder, e.g. AnalysisDataService
       * @param pythonClassName The name that the class should have in Python
       * @returns The new class object that wraps the given C++ type
       */
      static PythonType define(const char * pythonClassName)
      {
        using namespace boost::python;
        using namespace Mantid::Kernel;
        namespace Policies = Mantid::PythonInterface::Policies;

        auto classType = PythonType(pythonClassName, no_init)
          .def("add", &SvcType::add,
               "Adds the given object to the service with the given name. If the name/object exists it will raise an error.")
          .def("addOrReplace", &SvcType::addOrReplace,
               "Adds the given object to the service with the given name. The the name exists the object is replaced.")
          .def("doesExist", &SvcType::doesExist,
               "Returns True if the object is found in the service.")
          .def("retrieve", &DataServiceExporter::retrieveOrKeyError,
               return_value_policy<Policies::ToWeakPtrWithDowncast>(),
               "Retrieve the named object. Raises an exception if the name does not exist")
          .def("remove", &SvcType::remove,
               "Remove a named object")
          .def("clear", &SvcType::clear,
               "Removes all objects managed by the service.")
          .def("size", &SvcType::size,
               "Returns the number of objects within the service")
          .def("getObjectNames", &DataServiceExporter::getObjectNamesAsList,
               "Return the list of names currently known to the ADS")

          // Make it act like a dictionary
          .def("__len__", &SvcType::size)
          .def("__getitem__", &DataServiceExporter::retrieveOrKeyError,
               return_value_policy<Policies::ToWeakPtrWithDowncast>())
          .def("__setitem__", &SvcType::addOrReplace)
          .def("__contains__", &SvcType::doesExist)
          .def("__delitem__", &SvcType::remove)
          ;

        return classType;
      }

      /**
       * Retrieves a shared_ptr from the ADS and raises a Python KeyError if it does
       * not exist
       * @param self :: A pointer to the object calling this function. Allows it to act
       * as a member function
       * @param name :: The name of the object to retrieve
       * @return A shared_ptr to the named object. If the name does not exist it
       * sets a KeyError error indicator.
       */
      static SvcHeldType retrieveOrKeyError(SvcType& self, const std::string & name)
      {
        using namespace Mantid::Kernel;
        SvcHeldType item;
        try
        {
          item = self.retrieve(name);
        }
        catch(Exception::NotFoundError&)
        {
          // Translate into a Python KeyError
          std::string err = "'" + name + "' does not exist.";
          PyErr_SetString(PyExc_KeyError, err.c_str());
          throw boost::python::error_already_set();
        }
        return item;
      }

      /**
       * Return a Python list of object names from the ADS as this is
       * far easier to work with than a set
       * @param self :: A reference to the ADS object that called this method
       * @returns A python list created from the set of strings
       */
      static boost::python::object getObjectNamesAsList(SvcType& self)
      {
        boost::python::list names;
        const std::set<std::string> keys = self.getObjectNames();
        std::set<std::string>::const_iterator iend = keys.end();
        for(std::set<std::string>::const_iterator itr = keys.begin(); itr != iend; ++itr)
        {
          names.append(*itr);
        }
        assert(names.attr("__len__")() == keys.size());
        return names;
      }

    };
  }
}

#endif /* MANTID_PYTHONINTERFACE_DATASERVICEEXPORTER_H_ */
