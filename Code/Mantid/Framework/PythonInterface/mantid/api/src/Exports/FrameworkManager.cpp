#include "MantidAPI/FrameworkManager.h"

#include <boost/python/class.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::API::FrameworkManagerImpl;
using Mantid::API::FrameworkManager;
using namespace boost::python;

void export_FrameworkManager()
{
  class_<FrameworkManagerImpl,boost::noncopyable>("FrameworkManagerImpl", no_init)
    .def("setNumOMPThreadsToConfigValue", &FrameworkManagerImpl::setNumOMPThreadsToConfigValue,
         "Sets the number of OpenMP threads to the value specified in the config file")

     .def("setNumOMPThreads", &FrameworkManagerImpl::setNumOMPThreads,
         "Set the number of OpenMP threads to the given value")

     .def("getNumOMPThreads", &FrameworkManagerImpl::getNumOMPThreads,
         "Returns the number of OpenMP threads that will be used.")

    .def("clear", &FrameworkManagerImpl::clear, "Clear all memory held by Mantid")

    .def("clearAlgorithms", &FrameworkManagerImpl::clearAlgorithms, "Clear memory held by algorithms (does not include workspaces)")

    .def("clearData", &FrameworkManagerImpl::clearData, "Clear memory held by the data service (essentially all workspaces, including hidden)")

    .def("clearInstruments", &FrameworkManagerImpl::clearInstruments, "Clear memory held by the cached instruments")

    .def("Instance", &FrameworkManager::Instance, return_value_policy<reference_existing_object>(),
         "Returns a reference to the FrameworkManager singleton")
    .staticmethod("Instance")

    ;

}

