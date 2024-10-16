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
 * A specialisation of PropertyValueHandler to handle passing a PyObject directly
 * to a PythonObjectProperty
 */
struct PythonObjectTypeHandler : public TypedPropertyValueHandler<boost::python::object> {

  /// Call to set a named property where the value is some container type
  void set(Kernel::IPropertyManager *alg, const std::string &name, const boost::python::object &value) const override;

  /// Call to create a name property where the value is some container type
  std::unique_ptr<Kernel::Property> create(const std::string &name, const boost::python::object &defaultValue,
                                           const boost::python::object &validator,
                                           const unsigned int direction) const override;
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid
