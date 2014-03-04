#include "MantidPythonInterface/kernel/DataServiceExporter.h"

#include "MantidAPI/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace boost::python;

/// Weak pointer to DataItem typedef
typedef boost::weak_ptr<PropertyManager> PropertyManager_wptr;

void export_PropertyManagerDataService()
{
  using Mantid::PythonInterface::DataServiceExporter;

  register_ptr_to_python<PropertyManager_wptr>();
  auto pmdType = DataServiceExporter<PropertyManagerDataServiceImpl,
                                     PropertyManager_sptr>::define("PropertyManagerDataServiceImpl");

 pmdType.def("Instance", &PropertyManagerDataService::Instance,
             return_value_policy<reference_existing_object>(),
             "Return a reference to the ADS singleton");
 pmdType.staticmethod("Instance");
}

