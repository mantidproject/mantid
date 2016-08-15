#include "MantidPythonInterface/kernel/Registry/MappingTypeHandler.h"
#include "MantidPythonInterface/kernel/Registry/PropertyWithValueFactory.h"

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/make_shared.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/extract.hpp>

using boost::python::dict;
using boost::python::extract;
using boost::python::handle;
using boost::python::len;
using boost::python::object;

namespace Mantid {
using Kernel::Direction;
using Kernel::PropertyManager;
using Kernel::PropertyManager_sptr;
using Kernel::PropertyWithValue;
namespace PythonInterface {
namespace Registry {

namespace {
/**
 * Create a new PropertyManager from the given dict
 * @param mapping A wrapper around a Python dict instance
 * @return A shared_ptr to a new PropertyManager
 */
PropertyManager_sptr createPropertyManager(const dict &mapping) {
  auto pmgr = boost::make_shared<PropertyManager>();
#if PY_MAJOR_VERSION >= 3
  object view(mapping.attr("items")());
  object itemIter(handle<>(PyObject_GetIter(view.ptr())));
#else
  object itemIter(mapping.attr("iteritems")());
#endif
  auto length = len(mapping);
  for (ssize_t i = 0; i < length; ++i) {
    const object keyValue(handle<>(PyIter_Next(itemIter.ptr())));
    const std::string cppkey = extract<std::string>(keyValue[0])();
    pmgr->declareProperty(PropertyWithValueFactory::create(cppkey, keyValue[1],
                                                           Direction::Input));
  }
  return pmgr;
}
}

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
                           const boost::python::api::object &,
                           const unsigned int direction) const {
  // Wrap the property manager in a PropertyManagerProperty instance.
  std::unique_ptr<Kernel::Property> valueProp =
      Kernel::make_unique<Kernel::PropertyManagerProperty>(
          name, createPropertyManager(dict(defaultValue)), direction);
  return valueProp;
}
}
}
}
