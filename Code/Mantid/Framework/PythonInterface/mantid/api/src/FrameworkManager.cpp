#include "MantidAPI/FrameworkManager.h"
#include <boost/python/class.hpp>
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FrameworkManagerImpl;
using Mantid::API::FrameworkManager;
using boost::python::class_;
using boost::python::def;
using boost::python::no_init;
using boost::python::return_value_policy;
using boost::python::reference_existing_object;

namespace
{
  ///@cond

  // A factory function returning a reference to the AlgorithmManager instance so that
  // Python can use it
  FrameworkManagerImpl & getFrameworkManager()
  {
    return FrameworkManager::Instance();
  }
  ///@endcond
}


void export_FrameworkManager()
{
  class_<FrameworkManagerImpl,boost::noncopyable>("FrameworkManager", no_init)
    .def("clear", &FrameworkManagerImpl::clear, "Clear all memory held by Mantid")
    .def("clear_algorithms", &FrameworkManagerImpl::clearAlgorithms, "Clear memory held by algorithms (does not include workspaces)")
    .def("clear_data", &FrameworkManagerImpl::clearData, "Clear memory held by the data service (essentially all workspaces, including hidden)")
    .def("clear_instruments", &FrameworkManagerImpl::clearInstruments, "Clear memory held by the cached instruments")
    ;

  // Create a factory function to return this in Python
  def("get_framework_mgr", &getFrameworkManager, return_value_policy<reference_existing_object>(),
        "Returns a reference to the FrameworkManager singleton")
  ;


}

