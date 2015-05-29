#include "MantidPythonInterface/api/ExtractWorkspace.h"
#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/TrackingInstanceMethod.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::PythonInterface;
using namespace boost::python;

namespace {


/**
 * Add an item into the ADS, if it exists then an error is raised
 * @param self A reference to the calling object
 * @param name The name to assign to this in the service
 * @param item A boost.python wrapped SvcHeldType object
 */
void addItem(AnalysisDataServiceImpl &self, const std::string &name,
             const boost::python::object &item) {
  ExtractWorkspace extractWS(item);
  if(extractWS.check()) {
    self.add(name, extractWS());
  } else {
    throw std::runtime_error("Unable to add unknown object type to ADS");
  }
}

/**
 * Add or replace an item into the service, if it exists then an error is raised
 * @param self A reference to the calling object
 * @param name The name to assign to this in the service
 * @param item A boost.python wrapped SvcHeldType object
 */
void addOrReplaceItem(AnalysisDataServiceImpl &self, const std::string &name,
                      const boost::python::object &item) {
  ExtractWorkspace extractWS(item);
  if(extractWS.check()) {
    self.addOrReplace(name, extractWS());
  } else {
    throw std::runtime_error("Unable to add/replace unknown object type to ADS");
  }
}

}
// clang-format off
void export_AnalysisDataService()
// clang-format on
{
  typedef DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr> ADSExporter;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");

  // -- special ADS behaviour --
  // replace the add/addOrReplace,__setitem__ methods as we need to extract the
  // exact stored type
  pythonClass.def("add", &addItem, "Adds the given object to the service with "
                                   "the given name. If the name/object exists "
                                   "it will raise an error.");
  pythonClass.def("addOrReplace", &addOrReplaceItem,
                  "Adds the given object to the service with the given name. "
                  "The the name exists the object is replaced.");
  pythonClass.def("__setitem__", &addOrReplaceItem);

  // Instance method
  TrackingInstanceMethod<AnalysisDataService, ADSExporter::PythonType>::define(pythonClass);
}
