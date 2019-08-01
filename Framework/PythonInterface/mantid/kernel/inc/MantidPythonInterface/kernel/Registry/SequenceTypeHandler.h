// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_
#define MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_

#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * A specialisation of PropertyValueHander to handle coercing a Python
 * value into a C++ sequence/array property. The template type ContainerType
 * should contain a type called value_type indicating the element type.
 */
template <typename ContainerType>
struct DLLExport SequenceTypeHandler
    : TypedPropertyValueHandler<ContainerType> {

  /// Call to set a named property where the value is some container type
  void set(Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::object &value) const override;

  /// Call to create a name property where the value is some container type
  std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &defaultValue,
         const boost::python::object &validator,
         const unsigned int direction) const override;
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_SEQUENCETYPEHANDLER_H_ */
