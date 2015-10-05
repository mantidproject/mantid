#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/TrackingInstanceMethod.h"

#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace boost::python;

void export_AnalysisDataService() {
  typedef DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr>
      ADSExporter;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");

  // Instance method
  TrackingInstanceMethod<AnalysisDataService, ADSExporter::PythonType>::define(
      pythonClass);
}
