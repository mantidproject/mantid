// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * A specialisation of PropertyValueHandler to handle passing a python object directly
 * to a PythonObjectProperty
 */
struct DLLExport PythonObjectTypeHandler : public TypedPropertyValueHandler<boost::python::object> {

  /// Call to set a named property where the value is some container type
  /**
   * Set function to handle Python -> C++ calls to a property manager and get the
   * correct type
   * @param alg :: A pointer to an IPropertyManager
   * @param name :: The name of the property
   * @param value :: A boost python object that stores the value
   */
  void set(Kernel::IPropertyManager *alg, std::string const &name, boost::python::object const &value) const override {
    alg->setProperty(name, value);
  }

  /// Call to create a named property where the value is some container type
  std::unique_ptr<Kernel::Property> create(std::string const &name, boost::python::object const &defaultValue,
                                           boost::python::object const &validator,
                                           unsigned int const direction) const override;
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
