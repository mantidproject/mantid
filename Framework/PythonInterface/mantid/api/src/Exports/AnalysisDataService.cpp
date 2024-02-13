// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/core/Converters/ToPyList.h"
#include "MantidPythonInterface/core/DataServiceExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/core/WeakPtr.h"

#include <boost/python/enum.hpp>

#include <boost/python/list.hpp>
#include <boost/python/overloads.hpp>
#include <boost/python/return_value_policy.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(AnalysisDataServiceImpl)

namespace {
std::once_flag INIT_FLAG;

/**
 * Returns a reference to the AnalysisDataService object, creating it
 * if necessary. In addition to creating the object the first call also:
 *   - register AnalysisDataService.clear as an atexit function
 * @return A reference to the FrameworkManagerImpl instance
 */
AnalysisDataServiceImpl &instance() {
  // start the framework (if necessary)
  auto &ads = AnalysisDataService::Instance();
  std::call_once(INIT_FLAG, []() {
    // Passing True as an argument suppresses a warning that is normally
    // displayed when calling AnalysisDataService.clear()
    PyRun_SimpleString("import atexit\n"
                       "def cleanup_ADS():\n"
                       "    from mantid.api import AnalysisDataService\n"
                       "    AnalysisDataService.clear(True)\n"
                       "atexit.register(cleanup_ADS)");
  });
  return ads;
}

/**
 * @param self A reference to the AnalysisDataServiceImpl
 * @param names The list of names to extract
 * @param unrollGroups If true unroll the workspace groups
 * @return a python list of the workspaces in the ADS
 */

list retrieveWorkspaces(AnalysisDataServiceImpl const *const self, const list &names, bool unrollGroups = false) {
  using WeakPtr = std::weak_ptr<Workspace>;
  const auto wsSharedPtrs =
      self->retrieveWorkspaces(Converters::PySequenceToVector<std::string>(names)(), unrollGroups);
  std::vector<WeakPtr> wsWeakPtrs;
  wsWeakPtrs.reserve(wsSharedPtrs.size());
  std::transform(wsSharedPtrs.cbegin(), wsSharedPtrs.cend(), std::back_inserter(wsWeakPtrs),
                 [](const Workspace_sptr &wksp) -> WeakPtr { return WeakPtr(wksp); });
  return Converters::ToPyList<WeakPtr>()(wsWeakPtrs);
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
BOOST_PYTHON_FUNCTION_OVERLOADS(AdsRetrieveWorkspacesOverloads, retrieveWorkspaces, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

void export_AnalysisDataService() {
  using ADSExporter = DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");
  pythonClass
      .def("Instance", instance, return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance")
      .def("retrieveWorkspaces", retrieveWorkspaces,
           AdsRetrieveWorkspacesOverloads("Retrieve a list of workspaces by name",
                                          (arg("self"), arg("names"), arg("unrollGroups") = false)))
      .def("addToGroup", &AnalysisDataServiceImpl::addToGroup, (arg("groupName"), arg("wsName")),
           "Add a workspace in the ADS to a group in the ADS")
      .def("removeFromGroup", &AnalysisDataServiceImpl::removeFromGroup, (arg("groupName"), arg("wsName")),
           "Remove a workspace from a group in the ADS")
      .def("unique_name", &AnalysisDataServiceImpl::uniqueName,
           (arg("self"), arg("n") = 5, arg("prefix") = "", arg("suffix") = ""),
           "Return a randomly generated unique name for a workspace\n"
           "\n"
           ":param str n: length of string of random numbers\n"
           ":param str prefix: String to be prepended to the generated string\n"
           ":param str suffix: String to be appended to the generated string\n"
           ":return: prefix + n*random characters + suffix\n"
           ":rtype: str\n")
      .def("unique_hidden_name", &AnalysisDataServiceImpl::uniqueHiddenName, arg("self"),
           "Return a randomly generated unique hidden workspace name.");
}
