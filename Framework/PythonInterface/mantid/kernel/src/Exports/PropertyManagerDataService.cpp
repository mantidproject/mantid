#include "MantidPythonInterface/kernel/DataServiceExporter.h"

#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManager.h"

#include <boost/python/register_ptr_to_python.hpp>
#include <boost/weak_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::PythonInterface::DataServiceExporter;
using namespace boost::python;

/// Weak pointer to DataItem typedef
typedef boost::weak_ptr<PropertyManager> PropertyManager_wptr;

void export_PropertyManagerDataService() {

  register_ptr_to_python<PropertyManager_wptr>();

  typedef DataServiceExporter<PropertyManagerDataServiceImpl,
                              PropertyManager_sptr> PMDExporter;
  auto pmdType = PMDExporter::define("PropertyManagerDataServiceImpl");

  pmdType.def("Instance", &PropertyManagerDataService::Instance,
              return_value_policy<reference_existing_object>(),
              "Return a reference to the singleton instance")
      .staticmethod("Instance");
}
