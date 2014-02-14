#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/DataItem.h"

#include "MantidPythonInterface/kernel/Policies/DowncastingPolicies.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/list.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Kernel::DataItem_sptr;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

namespace
{
  /**
     * Retrieves a shared_ptr from the ADS and raises a Python KeyError if it does
     * not exist
     * @param self :: A pointer to the object calling this function. Allows it to act
     * as a member function
     * @param name :: The name of the object to retrieve
     * @return A shared_ptr to the named object. If the name does not exist it
     * sets a KeyError error indicator.
     */
    DataItem_sptr retrieveOrKeyError(AnalysisDataServiceImpl& self, const std::string & name)
    {
      DataItem_sptr item;
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
  object getObjectNamesAsList(AnalysisDataServiceImpl& self)
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
}


void export_AnalysisDataService()
{

  class_<AnalysisDataServiceImpl,boost::noncopyable>("AnalysisDataServiceImpl", no_init)
    .def("add", &AnalysisDataServiceImpl::add,
         "Adds the given object to the service with the given name. If the name/object exists it will raise an error.")
    .def("addOrReplace", &AnalysisDataServiceImpl::addOrReplace,
         "Adds the given object to the service with the given name. The the name exists the object is replaced.")
    .def("doesExist", &AnalysisDataServiceImpl::doesExist,
         "Returns True if the object is found in the service.")
    .def("retrieve", &retrieveOrKeyError, return_value_policy<Policies::ToWeakPtrWithDowncast>(),
         "Retrieve the named object. Raises an exception if the name does not exist")
    .def("remove", &AnalysisDataServiceImpl::remove,
         "Remove a named object")
    .def("clear", &AnalysisDataServiceImpl::clear,
         "Removes all objects managed by the service.")
    .def("size", &AnalysisDataServiceImpl::size,
         "Returns the number of objects within the service")
    .def("getObjectNames", &getObjectNamesAsList,
         "Return the list of names currently known to the ADS")
    .def("Instance", &AnalysisDataService::Instance, return_value_policy<reference_existing_object>(),
         "Return a reference to the ADS singleton")
    .staticmethod("Instance")

    // Make it act like a dictionary
    .def("__len__", &AnalysisDataServiceImpl::size)
    .def("__getitem__", &retrieveOrKeyError, return_value_policy<Policies::ToWeakPtrWithDowncast>())
    .def("__contains__", &AnalysisDataServiceImpl::doesExist)
    .def("__delitem__", &AnalysisDataServiceImpl::remove)
    ;
}

