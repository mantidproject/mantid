// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/core/DataServiceExporter.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"

#include <boost/python/register_ptr_to_python.hpp>
#include <boost/weak_ptr.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::PythonInterface::DataServiceExporter;
using Mantid::PythonInterface::Registry::createPropertyManager;
using namespace boost::python;

/// Weak pointer to DataItem typedef
using PropertyManager_wptr = boost::weak_ptr<PropertyManager>;

namespace {
/**
 * Add a dictionary to the data service directly. It creates a PropertyManager
 * on the way in.
 * @param self A reference to the PropertyManagerDataService
 * @param name The name of the object
 * @param mapping A dict object
 */
void addFromDict(PropertyManagerDataServiceImpl &self, const std::string &name,
                 const dict &mapping) {
  self.add(name, createPropertyManager(mapping));
}
/**
 * Add or replace a dictionary to the data service directly. It creates
 * a PropertyManager on the way in.
 * @param self A reference to the PropertyManagerDataService
 * @param name The name of the object
 * @param mapping A dict object
 */
void addOrReplaceFromDict(PropertyManagerDataServiceImpl &self,
                          const std::string &name, const dict &mapping) {
  self.addOrReplace(name, createPropertyManager(mapping));
}
} // namespace

GET_POINTER_SPECIALIZATION(PropertyManagerDataServiceImpl)

void export_PropertyManagerDataService() {

  register_ptr_to_python<PropertyManager_wptr>();

  using PMDExporter =
      DataServiceExporter<PropertyManagerDataServiceImpl, PropertyManager_sptr>;
  auto pmdType = PMDExporter::define("PropertyManagerDataServiceImpl");

  pmdType
      .def("Instance", &PropertyManagerDataService::Instance,
           return_value_policy<reference_existing_object>(),
           "Return a reference to the singleton instance")
      .staticmethod("Instance")
      // adds an overload from a dictionary
      .def("add", &addFromDict)
      .def("addOrReplace", &addOrReplaceFromDict);
}
