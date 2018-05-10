#include "MantidAPI/FrameworkManager.h"

#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::FrameworkManager;
using Mantid::API::FrameworkManagerImpl;
using namespace boost::python;

void export_FrameworkManager() {
  class_<FrameworkManagerImpl, boost::noncopyable>("FrameworkManagerImpl",
                                                   no_init)
      .def("setNumOMPThreadsToConfigValue",
           &FrameworkManagerImpl::setNumOMPThreadsToConfigValue, arg("self"),
           "Sets the number of OpenMP threads to the value "
           "specified in the "
           "config file")

      .def("setNumOMPThreads", &FrameworkManagerImpl::setNumOMPThreads,
           (arg("self"), arg("nthread")),
           "Set the number of OpenMP threads to the given value")

      .def("getNumOMPThreads", &FrameworkManagerImpl::getNumOMPThreads,
           arg("self"),
           "Returns the number of OpenMP threads that will be used.")

      .def("clear", &FrameworkManagerImpl::clear, arg("self"),
           "Clear all memory held by Mantid")

      .def("clearAlgorithms", &FrameworkManagerImpl::clearAlgorithms,
           arg("self"),
           "Clear memory held by algorithms (does not include workspaces)")

      .def("clearData", &FrameworkManagerImpl::clearData, arg("self"),
           "Clear memory held by the data service (essentially all "
           "workspaces, "
           "including hidden)")

      .def("clearInstruments", &FrameworkManagerImpl::clearInstruments,
           arg("self"), "Clear memory held by the cached instruments")

      .def("clearPropertyManagers",
           &FrameworkManagerImpl::clearPropertyManagers, arg("self"),
           "Clear memory held by the PropertyManagerDataService")

      .def("shutdown", &FrameworkManagerImpl::shutdown, arg("self"),
           "Effectively shutdown this service")

      .def("Instance", &FrameworkManager::Instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
