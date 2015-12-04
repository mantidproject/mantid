#include "MantidPythonInterface/kernel/DataServiceExporter.h"

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void export_AnalysisDataService() {
  typedef DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>
      ADSExporter;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");
  pythonClass.def("Instance", &AnalysisDataService::Instance,
                  return_value_policy<reference_existing_object>(),
                  "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
