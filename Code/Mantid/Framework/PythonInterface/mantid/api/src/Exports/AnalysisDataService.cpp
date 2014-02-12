#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/DataItem.h"

#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/Policies/DowncastingPolicies.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/list.hpp>

using Mantid::API::AnalysisDataServiceImpl;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::DataItem;
using Mantid::Kernel::DataItem_sptr;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;

namespace
{
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
    .def("add", &AnalysisDataServiceImpl::add, "Adds the given object to the service with the given name. If the name/object exists it will raise an error.")
    .def("addOrReplace", &AnalysisDataServiceImpl::addOrReplace, "Adds the given object to the service with the given name. The the name exists the object is replaced.")
    .def("doesExist", &AnalysisDataServiceImpl::doesExist, "Returns True if the object is found in the service.")
    .def("retrieve", &AnalysisDataServiceImpl::retrieve, return_value_policy<Policies::ToWeakPtrWithDowncast>(),
         "Retrieve the named object. Raises an exception if the name does not exist")
    .def("remove", &AnalysisDataServiceImpl::remove, "Remove a named object")
    .def("clear", &AnalysisDataServiceImpl::clear, "Removes all objects managed by the service.")
    .def("size", &AnalysisDataServiceImpl::size, "Returns the number of objects within the service")
    .def("getObjectNames", &getObjectNamesAsList, "Return the list of names currently known to the ADS")
    .def("Instance", &AnalysisDataService::Instance, return_value_policy<reference_existing_object>(),
         "Return a reference to the ADS singleton")
    .staticmethod("Instance")
    // Make it act like a dictionary
    .def("__len__", &AnalysisDataServiceImpl::size)
    .def("__getitem__", &AnalysisDataServiceImpl::retrieve, return_value_policy<Policies::ToWeakPtrWithDowncast>())
    .def("__contains__", &AnalysisDataServiceImpl::doesExist)
    .def("__delitem__", &AnalysisDataServiceImpl::remove)
    ;
}

