// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/api/Algorithms/RunPythonScript.h"
#include "MantidPythonInterface/core/ReleaseGlobalInterpreterLock.h"

#include <boost/python/class.hpp>
#include <boost/python/import.hpp>
#include <boost/python/reference_existing_object.hpp>
#include <boost/python/return_value_policy.hpp>

#include <mutex>

using Mantid::API::AlgorithmFactory;
using Mantid::API::FrameworkManager;
using Mantid::API::FrameworkManagerImpl;
using Mantid::Kernel::ConfigService;
using namespace boost::python;
using Mantid::PythonInterface::ReleaseGlobalInterpreterLock;

namespace {

std::once_flag INIT_FLAG;
bool INSTANCE_CALLED = false;
constexpr auto PYTHONPATHS_KEY = "pythonscripts.directories";

/**
 * We don't want to register the C++ algorithms on loading the api python
 * module since we want then be able to control when the various singletons
 * are created if we are being imported from vanilla Python. This function
 * registers the any C++ algorithms and should be called once.
 */
void declareCPPAlgorithms() { AlgorithmFactory::Instance().subscribe<Mantid::PythonInterface::RunPythonScript>(); }

/**
 * @brief Append to the sys.path any paths defined in the config key
 * pythonscripts.directories
 */
void updatePythonPaths() {
  auto packagesetup = import("mantid.kernel.packagesetup");
  packagesetup.attr("update_sys_paths")(ConfigService::Instance().getValue<std::string>(PYTHONPATHS_KEY).value_or(""));
}

/**
 * Returns a reference to the FrameworkManager object, creating it
 * if necessary. In addition to creating the object the first call also:
 *   - registers the C++ algorithms declared in this library
 *   - updates the Python paths with any user-defined directories
 *     declared in the `pythonscripts.directories`
 *   - import mantid.simpleapi (if not already imported) to load python plugins
 *   - register FrameworkManager.shutdown as an atexit function
 * @param importSimpleApi If true the mantid.simpleapi module is imported on
 * first access
 * @return A reference to the FrameworkManagerImpl instance
 */
FrameworkManagerImpl &instance() {
  // start the framework (if necessary)
  auto &frameworkMgr = []() -> auto & {
    // We need to release the GIL here to prevent a deadlock when using Python log channels
    ReleaseGlobalInterpreterLock releaseGIL;
    return FrameworkManager::Instance();
  }
  ();
  std::call_once(INIT_FLAG, []() {
    INSTANCE_CALLED = true;
    declareCPPAlgorithms();
    updatePythonPaths();
    import("mantid.simpleapi");
    // Without a python-based exit handler the singletons are only cleaned
    // up after main() and this is too late to acquire the GIL to be able to
    // delete any python objects still stored in other singletons like the
    // ADS or AlgorithmManager.
    PyRun_SimpleString("import atexit\n"
                       "def cleanupFrameworkManager():\n"
                       "    from mantid.api import FrameworkManager\n"
                       "    FrameworkManager.shutdown()\n"
                       "atexit.register(cleanupFrameworkManager)");
  });
  return frameworkMgr;
}

/**
 * @return True if .instance has been called, false otherwise
 */
bool hasInstance() { return INSTANCE_CALLED; }
} // namespace

void export_FrameworkManager() {
  class_<FrameworkManagerImpl, boost::noncopyable>("FrameworkManagerImpl", no_init)
      .def("setNumOMPThreadsToConfigValue", &FrameworkManagerImpl::setNumOMPThreadsToConfigValue, arg("self"),
           "Sets the number of OpenMP threads to the value "
           "specified in the "
           "config file")

      .def("setNumOMPThreads", &FrameworkManagerImpl::setNumOMPThreads, (arg("self"), arg("nthread")),
           "Set the number of OpenMP threads to the given value")

      .def("getNumOMPThreads", &FrameworkManagerImpl::getNumOMPThreads, arg("self"),
           "Returns the number of OpenMP threads that will be used.")

      .def("clear", &FrameworkManagerImpl::clear, arg("self"), "Clear all memory held by Mantid")

      .def("clearAlgorithms", &FrameworkManagerImpl::clearAlgorithms, arg("self"),
           "Clear memory held by algorithms (does not include workspaces)")

      .def("clearData", &FrameworkManagerImpl::clearData, arg("self"),
           "Clear memory held by the data service (essentially all "
           "workspaces, "
           "including hidden)")

      .def("clearInstruments", &FrameworkManagerImpl::clearInstruments, arg("self"),
           "Clear memory held by the cached instruments")

      .def("clearPropertyManagers", &FrameworkManagerImpl::clearPropertyManagers, arg("self"),
           "Clear memory held by the PropertyManagerDataService")

      .def("shutdown", &FrameworkManagerImpl::shutdown, arg("self"), "Effectively shutdown this service")

      .def("hasInstance", hasInstance, "Returns True if Instance has been called, false otherwise")
      .staticmethod("hasInstance")

      .def("Instance", instance, "Return a reference to the singleton instance",
           return_value_policy<reference_existing_object>())
      .staticmethod("Instance");
}
