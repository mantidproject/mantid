// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyManagerFactory.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/python/dict.hpp>

using boost::python::dict;
using boost::python::object;

namespace Mantid {
using Kernel::PropertyManager_sptr;
namespace PythonInterface {
namespace Registry {

/**
 * Sets the named property in the PropertyManager by extracting a new
 * PropertyManager from the Python object
 * @param alg A pointer to the PropertyManager containing the named property
 * @param name The name of the property to update
 * @param mapping The new value of the property
 */
void MappingTypeHandler::set(Kernel::IPropertyManager *alg,
                             const std::string &name,
                             const object &mapping) const {
  if (!PyObject_TypeCheck(mapping.ptr(), &PyDict_Type)) {
    throw std::invalid_argument("Property " + name + " expects a dictionary");
  }
  alg->setProperty(name, createPropertyManager(dict(mapping)));
}

/**
 * Always throws a runtime_error. Use the PropertyManagerProperty directly
 * @param name The name of the property
 * @param defaultValue A default value for the property.
 * @param validator A python object pointing to a validator instance, which
 * can be None.
 * @param direction The direction of the property
 * @returns A pointer to a newly constructed property instance
 */
std::unique_ptr<Kernel::Property>
MappingTypeHandler::create(const std::string &name, const object &defaultValue,
                           const boost::python::api::object & /*validator*/,
                           const unsigned int direction) const {
  // Wrap the property manager in a PropertyManagerProperty instance.
  std::unique_ptr<Kernel::Property> valueProp =
      std::make_unique<Kernel::PropertyManagerProperty>(
          name, createPropertyManager(dict(defaultValue)), direction);
  return valueProp;
}
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
