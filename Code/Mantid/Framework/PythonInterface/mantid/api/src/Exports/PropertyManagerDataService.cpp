#include "MantidPythonInterface/kernel/DataServiceExporter.h"
#include "MantidPythonInterface/kernel/TrackingInstanceMethod.h"

#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::PythonInterface::DataServiceExporter;
using Mantid::PythonInterface::TrackingInstanceMethod;
using namespace boost::python;

/// Weak pointer to DataItem typedef
typedef boost::weak_ptr<PropertyManager> PropertyManager_wptr;

void export_PropertyManagerDataService()
{

  register_ptr_to_python<PropertyManager_wptr>();

  typedef DataServiceExporter<PropertyManagerDataServiceImpl, PropertyManager_sptr> PMDExporter;
  auto pmdType = PMDExporter::define("PropertyManagerDataServiceImpl");
  
  // Instance method
  TrackingInstanceMethod<PropertyManagerDataService, PMDExporter::PythonType>::define(pmdType);

}

