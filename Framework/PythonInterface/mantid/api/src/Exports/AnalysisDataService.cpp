#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(AnalysisDataServiceImpl)

void export_AnalysisDataService() {
  using ADSExporter =
      DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");
  pythonClass.def("Instance", &AnalysisDataService::Instance,
                  return_value_policy<reference_existing_object>(),
                  "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
