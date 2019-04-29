// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/Converters/ToPyList.h"
#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

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
    PyRun_SimpleString("import atexit\n"
                       "from mantid.api import AnalysisDataService\n"
                       "atexit.register(lambda: AnalysisDataService.clear())");
  });
  return ads;
}

/**
 * @param self A reference to the AnalysisDataServiceImpl
 * @param names The list of names to extract
 * @param unrollGroups If true unroll the workspace groups
 * @return a python list of the workspaces in the ADS
 */
list retrieveWorkspaces(AnalysisDataServiceImpl &self, const list &names,
                        bool unrollGroups = false) {
  return Converters::ToPyList<Workspace_sptr>()(self.retrieveWorkspaces(
      Converters::PySequenceToVector<std::string>(names)(), unrollGroups));
}

GNU_DIAG_OFF("unused-local-typedef")
// Ignore -Wconversion warnings coming from boost::python
// Seen with GCC 7.1.1 and Boost 1.63.0
GNU_DIAG_OFF("conversion")
BOOST_PYTHON_FUNCTION_OVERLOADS(AdsRetrieveWorkspacesOverloads,
                                retrieveWorkspaces, 2, 3)
GNU_DIAG_ON("conversion")
GNU_DIAG_ON("unused-local-typedef")
} // namespace

void export_AnalysisDataService() {
  using ADSExporter =
      DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");
  pythonClass
      .def("Instance", instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance")
      .def("retrieveWorkspaces", retrieveWorkspaces,
           AdsRetrieveWorkspacesOverloads(
               "Retrieve a list of workspaces by name",
               (arg("self"), arg("names"), arg("unrollGroups") = false)))
      .def("addToGroup", &AnalysisDataServiceImpl::addToGroup,
           (arg("groupName"), arg("wsName")),
           "Add a workspace in the ADS to a group in the ADS")
      .def("removeFromGroup", &AnalysisDataServiceImpl::removeFromGroup,
           (arg("groupName"), arg("wsName")),
           "Remove a workspace from a group in the ADS");
}
