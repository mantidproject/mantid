// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "MantidPythonInterface/kernel/Registry/PythonObjectTypeHandler.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidPythonInterface/core/PythonObjectProperty.h"
#include <boost/python/extract.hpp>

namespace Mantid::PythonInterface::Registry {

/**
 * Set function to handle Python -> C++ calls to a property manager and get the
 * correct type
 * @param alg :: A pointer to an IPropertyManager
 * @param name :: The name of the property
 * @param value :: A boost python object that stores the value
 */
void PythonObjectTypeHandler::set(Kernel::IPropertyManager *alg, const std::string &name,
                                  const boost::python::object &value) const {
  alg->setProperty(name, value);
}

/**
 * Create a PropertyWithValue from the given python object value
 * @param name :: The name of the property
 * @param defaultValue :: The defaultValue of the property. The object attempts
 * to extract
 * a value of type ContainerType from the python object
 * @param validator :: A python object pointing to a validator instance, which
 * can be None.
 * @param direction :: The direction of the property
 * @returns A pointer to a newly constructed property instance
 */
std::unique_ptr<Kernel::Property> PythonObjectTypeHandler::create(const std::string &name,
                                                                  const boost::python::object &defaultValue,
                                                                  const boost::python::object &validator,
                                                                  const unsigned int direction) const {
  using boost::python::extract;
  using Kernel::IValidator;

  std::unique_ptr<Kernel::Property> valueProp;
  if (isNone(validator)) {
    valueProp = std::make_unique<PythonObjectProperty>(name, defaultValue, direction);
  } else {
    const IValidator *propValidator = extract<IValidator *>(validator);
    valueProp = std::make_unique<PythonObjectProperty>(name, defaultValue, propValidator->clone(), direction);
  }
  return valueProp;
}

} // namespace Mantid::PythonInterface::Registry
