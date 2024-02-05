// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <vector>

#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class PropertyManager;

/** @class PropertyManagerOwner PropertyManagerOwner.h
 Kernel/PropertyManagerOwner.h

 Implementation of IPropertyManager which allows sharing the same set of
 Properties
 between several instances.

 @author Roman Tolchenov, Tessella plc
 @date 03/03/2009
 */
class MANTID_KERNEL_DLL PropertyManagerOwner : virtual public IPropertyManager {
public:
  PropertyManagerOwner();
  PropertyManagerOwner(const PropertyManagerOwner &);
  PropertyManagerOwner &operator=(const PropertyManagerOwner &);

  // Function to declare properties (i.e. store them)
  void declareProperty(std::unique_ptr<Property> p, const std::string &doc = "") override;

  // Function to declare properties (i.e. store them)
  void declareOrReplaceProperty(std::unique_ptr<Property> p, const std::string &doc = "") override;
  void resetProperties() override;
  using IPropertyManager::declareProperty;

  // Sets all the declared properties from
  void setProperties(const std::string &propertiesJson,
                     const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                     bool createMissing = false) override;

  // Sets all the declared properties from a json object
  void setProperties(const ::Json::Value &jsonValue,
                     const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                     bool createMissing = false) override;

  // sets all the declared properties using a simple string format
  void setPropertiesWithString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>()) override;

  void setPropertyValue(const std::string &name, const std::string &value) override;
  void setPropertyValueFromJson(const std::string &name, const Json::Value &value) override;
  void setPropertyOrdinal(const int &index, const std::string &value) override;

  /// Make m_properties point to the same PropertyManager as po.
  virtual void copyPropertiesFrom(const PropertyManagerOwner &po) { *this = po; }

  bool existsProperty(const std::string &name) const override;
  bool validateProperties() const override;
  size_t propertyCount() const override;

  std::string getPropertyValue(const std::string &name) const override;
  const std::vector<Property *> &getProperties() const override;
  std::vector<std::string> getDeclaredPropertyNames() const noexcept override;

  /// Get the value of a property
  TypedValue getProperty(const std::string &name) const override;

  /// Return the property manager serialized as a string.
  std::string asString(bool withDefaultValues = false) const override;

  /// Return the property manager serialized as a json object.
  ::Json::Value asJson(bool withDefaultValues = false) const override;

  bool isDefault(const std::string &name) const;

  /// Removes the property from management
  void removeProperty(const std::string &name, const bool delproperty = true) override;
  /// Removes the property from management returning a pointer to it
  std::unique_ptr<Property> takeProperty(const size_t index) override;
  /// Clears all properties under management
  void clear() override;
  /// Override this method to perform a custom action right after a property was
  /// set.
  /// The argument is the property name. Default - do nothing.
  void afterPropertySet(const std::string &) override;

public:
  Property *getPointerToProperty(const std::string &name) const override;
  Property *getPointerToPropertyOrdinal(const int &index) const override;

private:
  /// Shared pointer to the 'real' property manager
  std::shared_ptr<PropertyManager> m_properties;
};

} // namespace Kernel
} // namespace Mantid
