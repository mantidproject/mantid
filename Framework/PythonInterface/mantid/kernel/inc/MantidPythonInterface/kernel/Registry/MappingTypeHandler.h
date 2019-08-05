// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_MAPPINGTYPEHANDLER_H
#define MANTID_PYTHONINTERFACE_MAPPINGTYPEHANDLER_H

#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"

namespace Mantid {
namespace PythonInterface {
namespace Registry {

/**
 *  Defines a handler class for converting a Python mapping type object
 * to a C++ PropertyManager type.
 */
class MappingTypeHandler final : public PropertyValueHandler {
  void set(Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::api::object &mapping) const override;
  std::unique_ptr<Kernel::Property>
  create(const std::string &name,
         const boost::python::api::object &defaultValue,
         const boost::python::api::object &validator,
         const unsigned int direction) const override;
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif // MANTID_PYTHONINTERFACE_MAPPINGTYPEHANDLER_H
