#include "MantidPythonInterface/kernel/Converters/PySequenceToVector.h"
#include "MantidPythonInterface/kernel/Converters/ToPyList.h"
#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include "MantidAPI/AnalysisDataService.h"
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
list retrieveWorkspaces(AnalysisDataServiceImpl &self, const list &names,
                        bool unrollGroups = false) {
  return Converters::ToPyList<Workspace_sptr>()(self.retrieveWorkspaces(
      Converters::PySequenceToVector<std::string>(names)(), unrollGroups));
}

BOOST_PYTHON_FUNCTION_OVERLOADS(AdsRetrieveWorkspacesOverloads,
                                retrieveWorkspaces, 2, 3)
} // namespace

void export_AnalysisDataService() {
  using ADSExporter =
      DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");
  pythonClass
      .def("Instance", &AnalysisDataService::Instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance")
      .def("retrieveWorkspaces", retrieveWorkspaces,
           AdsRetrieveWorkspacesOverloads(
               "Retrieve a list of workspaces by name",
               (arg("self"), arg("names"), arg("unrollGroups") = false)));
}
