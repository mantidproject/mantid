#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"
#include "MantidPythonInterface/kernel/Policies/DowncastReturnedValue.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/list.hpp>

using Mantid::API::PropertyManagerDataServiceImpl;
using Mantid::API::PropertyManagerDataService;
using Mantid::Kernel::PropertyManager;
namespace Policies = Mantid::PythonInterface::Policies;
using namespace boost::python;


/// Weak pointer to DataItem typedef
typedef boost::weak_ptr<PropertyManager> PropertyManager_wptr;

namespace
{
  /**
   * Retrieves a shared_ptr from the PMDS and converts it into a weak_ptr
   * @param self :: A pointer to the object calling this function. Allows it to act
   * as a member function
   * @param name :: The name of the property manager to retrieve
   * @return A weak pointer to the named property manager. If the name does not exist it
   * sets a KeyError error indicator.
   */
  PropertyManager_wptr retrieveAsWeakPtr(PropertyManagerDataServiceImpl& self, const std::string & name)
  {
    PropertyManager_wptr item;
    try
    {
      item = self.retrieve(name);
    }
    catch(Mantid::Kernel::Exception::NotFoundError&)
    {
      // Translate into a Python KeyError
      std::string err = "'" + name + "' does not exist.";
      PyErr_SetString(PyExc_KeyError, err.c_str());
      throw boost::python::error_already_set();
    }
    return item;
  }

  void addPtr(PropertyManagerDataServiceImpl& self, const std::string & name,
      boost::shared_ptr<PropertyManager> pm)
  {
    try
    {
        self.addOrReplace(name, pm);
    }
    catch(...)
    {
        PyErr_SetString(PyExc_ReferenceError, "Something went wrong adding the property manager.");
        throw boost::python::error_already_set();
    }
  }

  /**
   * Return a Python list of object names from the PMDS as this is
   * far easier to work with than a set
   * @param self :: A reference to the PMDS object that called this method
   * @returns A python list created from the set of strings
   */
  object getObjectNamesAsList(PropertyManagerDataServiceImpl& self)
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


void export_PropertyManagerDataService()
{
  register_ptr_to_python<PropertyManager_wptr>();

  class_<PropertyManagerDataServiceImpl,boost::noncopyable>("PropertyManagerDataServiceImpl", no_init)
    .def("doesExist", &PropertyManagerDataServiceImpl::doesExist, "Returns True if the property manager is found in the service.")
    .def("retrieve", &retrieveAsWeakPtr, return_value_policy<Policies::DowncastReturnedValue>(),
         "Retrieve the named property manager. Raises an exception if the name does not exist")
    .def("remove", &PropertyManagerDataServiceImpl::remove, "Remove a named property manager")
    .def("clear", &PropertyManagerDataServiceImpl::clear, "Removes all property managers managed by the service.")
    .def("size", &PropertyManagerDataServiceImpl::size, "Returns the number of objects within the service")
    .def("getObjectNames", &getObjectNamesAsList, "Return the list of names currently known to the PMDS")
    .def("Instance", &PropertyManagerDataService::Instance, return_value_policy<reference_existing_object>(),
         "Return a reference to the PMDS singleton")
    .staticmethod("Instance")
    .def("add", &PropertyManagerDataServiceImpl::add, "Add a property manager to the service.")
    .def("addOrReplace", &PropertyManagerDataServiceImpl::add, "Add a property manager to the service or replace an existing one.")
    // Make it act like a dictionary
    .def("__len__", &PropertyManagerDataServiceImpl::size)
    .def("__getitem__", &retrieveAsWeakPtr, return_value_policy<Policies::DowncastReturnedValue>())
    .def("__contains__", &PropertyManagerDataServiceImpl::doesExist)
    .def("__delitem__", &PropertyManagerDataServiceImpl::remove)
    .def("__setitem__", &addPtr)
    ;

}

