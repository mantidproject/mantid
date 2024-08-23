// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Workspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/WeakPtr.h"
#include "MantidPythonInterface/kernel/Registry/TypeRegistry.h"
#include "MantidPythonInterface/kernel/Registry/TypedPropertyValueHandler.h"

#include <boost/python/register_ptr_to_python.hpp>

namespace Mantid::PythonInterface::Registry {

// Specialization for shared_ptr<Workspace> and derived types.
template <typename T>
struct DLLExport TypedPropertyValueHandler<std::shared_ptr<T>,
                                           typename std::enable_if<std::is_base_of<API::Workspace, T>::value>::type>
    : public PropertyValueHandler {
  /// Type required by TypeRegistry framework
  using HeldType = std::shared_ptr<T>;

  /// Convenience typedef
  using PointeeType = T;
  /// Convenience typedef
  using PropertyValueType = std::shared_ptr<T>;

  /**
   * Set function to handle Python -> C++ calls and get the correct type
   * @param alg :: A pointer to an IPropertyManager
   * @param name :: The name of the property
   * @param value :: A boost python object that stores the value
   */
  void set(Kernel::IPropertyManager *alg, const std::string &name, const boost::python::object &value) const override {
    if (value == boost::python::object())
      alg->setProperty<HeldType>(name, std::shared_ptr<T>(nullptr));
    else
      alg->setProperty<HeldType>(name, std::dynamic_pointer_cast<T>(ExtractSharedPtr<API::Workspace>(value)()));
  }

  GNU_DIAG_OFF("maybe-uninitialized")
  /**
   * Create a PropertyWithValue from the given python object value
   * @param name :: The name of the property
   * @param defaultValue :: The defaultValue of the property. The object
   * attempts to extract
   * a value of type ValueType from the python object
   * @param validator :: A python object pointing to a validator instance, which
   * can be None.
   * @param direction :: The direction of the property
   * @returns A pointer to a newly constructed property instance
   */
  std::unique_ptr<Kernel::Property> create(const std::string &name, const boost::python::object &defaultValue,
                                           const boost::python::object &validator,
                                           const unsigned int direction) const override {
    using boost::python::extract;
    using Kernel::IValidator;
    using Kernel::Property;
    using Kernel::PropertyWithValue;
    const PropertyValueType valueInC = extract<PropertyValueType>(defaultValue)();
    std::unique_ptr<Property> valueProp;
    if (isNone(validator)) {
      valueProp = std::make_unique<PropertyWithValue<PropertyValueType>>(name, valueInC, direction);
    } else {
      const IValidator *propValidator = extract<IValidator *>(validator);
      valueProp =
          std::make_unique<PropertyWithValue<PropertyValueType>>(name, valueInC, propValidator->clone(), direction);
    }
    return valueProp;
  }
  GNU_DIAG_ON("maybe-uninitialized")
};

/**
 * Encapsulates the registration required for an interface type T
 * that sits on top of a Kernel::DataItem object. The constructor
 * does 3 things:
 *    - Calls register_ptr_to_python<boost::shared_ptr<T>>
 *    - Calls register_ptr_to_python<boost::weak_ptr<T>>
 *    - Registers a new PropertyValueHandler for a boost::shared_ptr<T>
 */
template <typename IType> struct DLLExport RegisterWorkspacePtrToPython {
  using IType_sptr = std::shared_ptr<IType>;
  using IType_wptr = std::weak_ptr<IType>;
  /// Constructor
  RegisterWorkspacePtrToPython() {
    using namespace boost::python;
    using namespace Registry;

    register_ptr_to_python<IType_sptr>();
    register_ptr_to_python<IType_wptr>();
    // properties can only ever store pointers to these
    TypeRegistry::subscribe<TypedPropertyValueHandler<IType_sptr>>();
  }
};
} // namespace Mantid::PythonInterface::Registry
