// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2011 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_
#define MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_

#include "MantidPythonInterface/api/ExtractWorkspace.h"
#include "MantidPythonInterface/core/IsNone.h" // includes object.hpp
#include "MantidPythonInterface/kernel/Registry/PropertyValueHandler.h"

#include "MantidAPI/Workspace.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/PropertyWithValue.h"

#include <boost/python/call_method.hpp>
#include <boost/python/converter/arg_from_python.hpp>
#include <boost/python/extract.hpp>
#include <boost/weak_ptr.hpp>

#include <string>

namespace Mantid {
namespace PythonInterface {
namespace Registry {
/**
 * This class provides a templated class object that is able to take a
 * python object and perform operations with a given C type.
 */
template <typename ValueType, typename Enable = void>
struct DLLExport TypedPropertyValueHandler : public PropertyValueHandler {
  /// Type required by TypeRegistry framework
  using HeldType = ValueType;

  /**
   * Set function to handle Python -> C++ calls and get the correct type
   * @param alg :: A pointer to an IPropertyManager
   * @param name :: The name of the property
   * @param value :: A boost python object that stores the value
   */
  void set(Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::object &value) const override {
    alg->setProperty<ValueType>(name, boost::python::extract<ValueType>(value));
  }
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
  std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &defaultValue,
         const boost::python::object &validator,
         const unsigned int direction) const override {
    using Mantid::Kernel::IValidator;
    using Mantid::Kernel::PropertyWithValue;
    using boost::python::extract;
    const ValueType valueInC = extract<ValueType>(defaultValue)();
    std::unique_ptr<Kernel::Property> valueProp;
    if (isNone(validator)) {
      valueProp = std::make_unique<PropertyWithValue<ValueType>>(name, valueInC,
                                                                 direction);
    } else {
      const IValidator *propValidator = extract<IValidator *>(validator);
      valueProp = std::make_unique<PropertyWithValue<ValueType>>(
          name, valueInC, propValidator->clone(), direction);
    }
    return valueProp;
  }
};

//
// Specialization for shared_ptr types. They need special handling for
// workspaces
//
template <typename T>
struct DLLExport TypedPropertyValueHandler<
    boost::shared_ptr<T>,
    typename std::enable_if<std::is_base_of<API::Workspace, T>::value>::type>
    : public PropertyValueHandler {
  /// Type required by TypeRegistry framework
  using HeldType = boost::shared_ptr<T>;

  /// Convenience typedef
  using PointeeType = T;
  /// Convenience typedef
  using PropertyValueType = boost::shared_ptr<T>;

  /**
   * Set function to handle Python -> C++ calls and get the correct type
   * @param alg :: A pointer to an IPropertyManager
   * @param name :: The name of the property
   * @param value :: A boost python object that stores the value
   */
  void set(Kernel::IPropertyManager *alg, const std::string &name,
           const boost::python::object &value) const override {
    if (value == boost::python::object())
      alg->setProperty<HeldType>(name, boost::shared_ptr<T>(nullptr));
    else
      alg->setProperty<HeldType>(
          name, boost::dynamic_pointer_cast<T>(ExtractWorkspace(value)()));
  }

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
  std::unique_ptr<Kernel::Property>
  create(const std::string &name, const boost::python::object &defaultValue,
         const boost::python::object &validator,
         const unsigned int direction) const override {
    using Kernel::IValidator;
    using Kernel::Property;
    using Kernel::PropertyWithValue;
    using boost::python::extract;
    const PropertyValueType valueInC =
        extract<PropertyValueType>(defaultValue)();
    std::unique_ptr<Property> valueProp;
    if (isNone(validator)) {
      valueProp = std::make_unique<PropertyWithValue<PropertyValueType>>(
          name, valueInC, direction);
    } else {
      const IValidator *propValidator = extract<IValidator *>(validator);
      valueProp = std::make_unique<PropertyWithValue<PropertyValueType>>(
          name, valueInC, propValidator->clone(), direction);
    }
    return valueProp;
  }
};
} // namespace Registry
} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_TYPEDPROPERTYVALUEHANDLER_H_ */
