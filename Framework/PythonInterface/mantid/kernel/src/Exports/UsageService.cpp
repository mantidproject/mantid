// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UsageService.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/reference_existing_object.hpp>

#include <mutex>

using Mantid::Kernel::UsageService;
using Mantid::Kernel::UsageServiceImpl;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(UsageServiceImpl)

namespace {

std::once_flag INIT_FLAG;

/**
 * Returns a reference to the UsageService object, creating it
 * if necessary. In addition to creating the object the first call also:
 *   - register UsageService.shutdown as an atexit function
 * @return A reference to the UsageService instance
 */
UsageServiceImpl &instance() {
  // start the framework (if necessary)
  auto &svc = UsageService::Instance();
  std::call_once(INIT_FLAG, []() {
    PyRun_SimpleString("import atexit\n"
                       "from mantid.kernel import UsageService\n"
                       "atexit.register(lambda: UsageService.shutdown())");
  });
  return svc;
}
} // namespace

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
      .def("setApplicationName", &UsageServiceImpl::setApplicationName,
           (arg("self"), arg("name")),
           "Sets the application name that has invoked Mantid.")
      .def("getApplicationName", &UsageServiceImpl::getApplicationName,
           arg("self"), "Gets the application name that has invoked Mantid.")
      .def("registerStartup", &UsageServiceImpl::registerStartup, arg("self"),
           "Registers the startup of Mantid.")
      .def("registerFeatureUsage", &UsageServiceImpl::registerFeatureUsage,
           (arg("self"), arg("type"), arg("name"), arg("internal")),
           "Registers the use of a feature in Mantid.")
      .def("getStartTime", &UsageServiceImpl::getStartTime, (arg("self")),
           "Returns the time at which Mantid was started")
      .def("Instance", instance,
           return_value_policy<reference_existing_object>(),
           "Returns a reference to the UsageService")
      .staticmethod("Instance");
}
