#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/DataItem.h"
#include "MantidPythonInterface/kernel/PropertyMarshal.h"
#include "MantidPythonInterface/kernel/WeakPtr.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/register_ptr_to_python.hpp>


using Mantid::API::AnalysisDataServiceImpl;
using Mantid::API::AnalysisDataService;
using Mantid::Kernel::DataItem;
using Mantid::Kernel::DataItem_sptr;
using namespace boost::python;

/// Weak pointer to DataItem typedef
typedef boost::weak_ptr<DataItem> DataItem_wptr;

namespace
{
  ///@cond

  //----------------------------------------------------------------------------
  //
  // The next two methods work in conjunction with each other. The first
  // adds a method to the Python wrapped ADS that returns the requested
  // object as a DataItem pointer. The second method calls this and
  // attempts to upcast the object to one of the most exported interface types
  // e.g. IEventWorkspace, MatrixWorkspace.
  // A custom return_value_policy would be nice but they seem set up for a 1:1
  // mapping between C++ type and Python object type whereas we would want a
  // 1:n DataItem:<Interface> policy.

  /**
   * From Python retrieval of a DataItem needs to return a weak_ptr to avoid
   * holding on to memory
   * @param self :: A pointer to the object calling this function. Allows it to act
   * as a member function
   * @param name :: The name of the object to retrieve
   * @return A weak pointer to the named object. If the name does not exist it
   * sets a KeyError error indicator.
   */
  DataItem_wptr retrieveAsDataItem(AnalysisDataServiceImpl& self, const std::string & name)
  {
    DataItem_wptr item;
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
    return DataItem_wptr(item);
  }

  /**
   * Upcast a Python object to one of the most exported type
   * @param self :: A pointer to the object calling this function. Allows it to act
   * as a member function
   * @param name The name of the object to retrieve
   * @return A boost python object of the upcasted type
   */
  object retrieveUpcastedPtr(object self, const std::string & name)
  {
    object dataItem = self.attr("retrieveAsDataItem")(name);
    Mantid::PythonInterface::PropertyMarshal::upcastFromDataItem(dataItem);
    return dataItem;
  }
  ///@endcond
}


void export_AnalysisDataService()
{
  register_ptr_to_python<DataItem_wptr>();

  class_<AnalysisDataServiceImpl,boost::noncopyable>("AnalysisDataService", no_init)
    .def("Instance", &AnalysisDataService::Instance, return_value_policy<reference_existing_object>(),
         "Return a reference to the ADS singleton")
    .staticmethod("Instance")
    .def("retrieveAsDataItem", &retrieveAsDataItem, "Retrieve the named object as data item. Raises an exception if the name does not exist")
    .def("retrieve", &retrieveUpcastedPtr, "Retrieve the named object. Raises an exception if the name does not exist")
    .def("remove", &AnalysisDataServiceImpl::remove, "Remove a named object")
    .def("clear", &AnalysisDataServiceImpl::clear, "Removes all objects managed by the service.")
    .def("size", &AnalysisDataServiceImpl::size, "Returns the number of objects within the service")
    // Make it act like a dictionary
    .def("__len__", &AnalysisDataServiceImpl::size)
    .def("__getitem__", &retrieveUpcastedPtr)
    .def("__contains__", &AnalysisDataServiceImpl::doesExist)
    .def("__delitem__", &AnalysisDataServiceImpl::remove)
    ;

}

