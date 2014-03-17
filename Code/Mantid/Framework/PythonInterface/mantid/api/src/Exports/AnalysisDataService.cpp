#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/Registry/DowncastRegistry.h"
#include "MantidPythonInterface/kernel/TrackingInstanceMethod.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Workspace.h"

#include <boost/python/call_method.hpp>

using namespace Mantid::API;
using Mantid::PythonInterface::DataServiceExporter;
using Mantid::PythonInterface::TrackingInstanceMethod;
using namespace Mantid::PythonInterface::Registry;
using namespace boost::python;


namespace
{
  /**
   * Add an item into the ADS, if it exists then an error is raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  void addItem(AnalysisDataServiceImpl& self, const std::string & name, const boost::python::object& item)
  {
    const auto & entry = DowncastRegistry::retrieve(call_method<std::string>(item.ptr(), "id"));

    try
    {
      // It is VERY important that the extract type be a reference to SvcHeldType so that
      // boost.python doesn't create a new shared_ptr and instead simply extracts the embedded one.
      self.add(name, boost::dynamic_pointer_cast<Workspace>(entry.fromPythonAsSharedPtr(item)));
    }
    catch(std::exception& exc)
    {
      PyErr_SetString(PyExc_RuntimeError, exc.what()); // traditionally throws RuntimeError so don't break scripts
      throw boost::python::error_already_set();
    }
  }

  /**
   * Add or replace an item into the service, if it exists then an error is raised
   * @param self A reference to the calling object
   * @param name The name to assign to this in the service
   * @param item A boost.python wrapped SvcHeldType object
   */
  void addOrReplaceItem(AnalysisDataServiceImpl& self, const std::string & name,
                        const boost::python::object& item)
  {
    const auto & entry = DowncastRegistry::retrieve(call_method<std::string>(item.ptr(), "id"));

    try
    {
      // It is VERY important that the extract type be a reference to SvcHeldType so that
      // boost.python doesn't create a new shared_ptr and instead simply extracts the embedded one.
      self.addOrReplace(name, boost::dynamic_pointer_cast<Workspace>(entry.fromPythonAsSharedPtr(item)));
    }
    catch(std::exception& exc)
    {
      PyErr_SetString(PyExc_RuntimeError, exc.what()); // traditionally throws RuntimeError so don't break scripts
      throw boost::python::error_already_set();
    }
  }

}

void export_AnalysisDataService()
{
  typedef DataServiceExporter<AnalysisDataServiceImpl, Workspace_sptr> ADSExporter;
  auto pythonClass = ADSExporter::define("AnalysisDataServiceImpl");

  // -- special ADS behaviour --
  // replace the add/addOrReplace,__setitem__ methods as we need to exact the exact stored type
  pythonClass.def("add", &addItem,
       "Adds the given object to the service with the given name. If the name/object exists it will raise an error.");
  pythonClass.def("addOrReplace", &addOrReplaceItem,
       "Adds the given object to the service with the given name. The the name exists the object is replaced.");
  pythonClass.def("__setitem__", &addOrReplaceItem);

  // Instance method
  TrackingInstanceMethod<AnalysisDataService, ADSExporter::PythonType>::define(pythonClass);
}

