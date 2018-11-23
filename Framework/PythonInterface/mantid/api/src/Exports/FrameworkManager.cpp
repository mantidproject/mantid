// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidPythonInterface/api/Algorithms/RunPythonScript.h"

#include <boost/python/class.hpp>
#include <boost/python/import.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

#include <iostream>
#include <mutex>

using Mantid::API::AlgorithmFactory;
using Mantid::API::FrameworkManager;
using Mantid::API::FrameworkManagerImpl;
using Mantid::Kernel::ConfigService;
using namespace boost::python;

namespace {

std::once_flag INIT_FLAG;
constexpr auto PYTHONPATHS_KEY = "pythonscripts.directories";

/**
 * We don't want to register the C++ algorithms on loading the api python
 * module since we want then be able to control when the various singletons
 * are created if we are being imported from vanilla Python. This function
 * registers the any C++ algorithms and should be called once.
 */
void declareCPPAlgorithms() {
  AlgorithmFactory::Instance()
      .subscribe<Mantid::PythonInterface::RunPythonScript>();
}

/**
 * @brief Append to the sys.path any paths defined in the config key
 * pythonscripts.directories
 */
void updatePythonPaths() {
  auto packagesetup = import("mantid.kernel.packagesetup");
  packagesetup.attr("update_sys_paths")(
      ConfigService::Instance()
          .getValue<std::string>(PYTHONPATHS_KEY)
          .get_value_or(""));
}

/**
 * Returns a reference to the FrameworkManager object, creating it
 * if necessary. In addition to creating the object the first call also:
 *   - registers the C++ algorithms declared in this library
 *   - updates the Python paths with any user-defined directories
 *     declared in the `pythonscripts.directories`
 *   - register FrameworkManager.shutdown as an atexit function
 * @return A reference to the FrameworkManagerImpl instance
 */
FrameworkManagerImpl &instance() {
  // start the framework (if necessary)
  auto &frameworkMgr = FrameworkManager::Instance();
  std::call_once(INIT_FLAG, []() {
    declareCPPAlgorithms();
    updatePythonPaths();
    // Without a python-based exit handler the singletons are only cleaned
    // up after main() and this is too late to acquire the GIL to be able to
    // delete any python objects still stored in other singletons like the
    // ADS or AlgorithmManager.
    Py_AtExit([]() { FrameworkManager::Instance().shutdown(); });
  });
  return frameworkMgr;
}
} // namespace

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

      .def("Instance", instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
