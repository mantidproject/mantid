#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidKernel/UsageService.h"
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::Kernel::UsageService;
using Mantid::Kernel::UsageServiceImpl;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(UsageServiceImpl)

void export_UsageService() {

  class_<UsageServiceImpl, boost::noncopyable>("UsageServiceImpl", no_init)
      .def("flush", &UsageServiceImpl::flush, arg("self"),
           "Sends any pending usage information.")

      .def("shutdown", &UsageServiceImpl::shutdown, arg("self"),
           "Sends any pending usage information, and disables the usage "
           "service.")

      .def("getUpTime", &UsageServiceImpl::getUpTime, arg("self"),
           "Returns the time that the instance of mantid has been running")

      .def("isEnabled", &UsageServiceImpl::isEnabled, arg("self"),
           "Returns if the usage service is enabled.")

      .def("setEnabled", &UsageServiceImpl::setEnabled,
           (arg("self"), arg("enabled")),
           "Enables or disables the usage service.")

      .def("setInterval", &UsageServiceImpl::setEnabled,
           (arg("self"), arg("seconds")),
           "Sets the interval that the timer checks for tasks.")

      .def("setApplication", &UsageServiceImpl::setApplication,
           (arg("self"), arg("name")),
           "Sets the application name that has invoked Mantid.")

      .def("getApplication", &UsageServiceImpl::getApplication, arg("self"),
           "Gets the application name that has invoked Mantid.")

      .def("registerStartup", &UsageServiceImpl::registerStartup, arg("self"),
           "Registers the startup of Mantid.")

      .def("registerFeatureUsage", &UsageServiceImpl::registerFeatureUsage,
           (arg("self"), arg("type"), arg("name"), arg("internal")),
           "Registers the use of a feature in Mantid.")

      .def("Instance", &UsageService::Instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the UsageService")
      .staticmethod("Instance");
}
