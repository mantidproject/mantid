// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYMANAGEROWNER_H_
#define MANTID_KERNEL_PROPERTYMANAGEROWNER_H_

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
  void declareProperty(std::unique_ptr<Property> p,
                       const std::string &doc = "") override;

  // Function to declare properties (i.e. store them)
  void declareOrReplaceProperty(std::unique_ptr<Property> p,
                                const std::string &doc = "") override;

  using IPropertyManager::declareProperty;

  // Sets all the declared properties from
  void setProperties(const std::string &propertiesJson,
                     const std::unordered_set<std::string> &ignoreProperties =
                         std::unordered_set<std::string>(),
                     bool createMissing = false) override;

  // Sets all the declared properties from a json object
  void setProperties(const ::Json::Value &jsonValue,
                     const std::unordered_set<std::string> &ignoreProperties =
                         std::unordered_set<std::string>(),
                     bool createMissing = false) override;

  // sets all the declared properties using a simple string format
  void setPropertiesWithString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties =
          std::unordered_set<std::string>()) override;

  void setPropertyValue(const std::string &name,
                        const std::string &value) override;
  void setPropertyValueFromJson(const std::string &name,
                                const Json::Value &value) override;
  void setPropertyOrdinal(const int &index, const std::string &value) override;

  /// Make m_properties point to the same PropertyManager as po.
  virtual void copyPropertiesFrom(const PropertyManagerOwner &po) {
    *this = po;
  }

  bool existsProperty(const std::string &name) const override;
  bool validateProperties() const override;
  size_t propertyCount() const override;

  std::string getPropertyValue(const std::string &name) const override;
  const std::vector<Property *> &getProperties() const override;

  /// Get the value of a property
  TypedValue getProperty(const std::string &name) const override;

  /// Return the property manager serialized as a string.
  std::string asString(bool withDefaultValues = false) const override;

  /// Return the property manager serialized as a json object.
  ::Json::Value asJson(bool withDefaultValues = false) const override;

  bool isDefault(const std::string &name) const;

  /// Removes the property from management
  void removeProperty(const std::string &name,
                      const bool delproperty = true) override;
  /// Clears all properties under management
  void clear() override;
  /// Override this method to perform a custom action right after a property was
  /// set.
  /// The argument is the property name. Default - do nothing.
  void afterPropertySet(const std::string &) override;

  void filterByTime(const Types::Core::DateAndTime & /*start*/,
                    const Types::Core::DateAndTime & /*stop*/) override {
    throw(std::runtime_error("Not yet implmented"));
  }
  void splitByTime(std::vector<SplittingInterval> & /*splitter*/,
                   std::vector<PropertyManager *> /* outputs*/) const override {
    throw(std::runtime_error("Not yet implmented"));
  }
  void filterByProperty(const TimeSeriesProperty<bool> & /*filte*/) override {
    throw(std::runtime_error("Not yet implmented"));
  }

public:
  Property *getPointerToProperty(const std::string &name) const override;
  Property *getPointerToPropertyOrdinal(const int &index) const override;

private:
  /// Shared pointer to the 'real' property manager
  boost::shared_ptr<PropertyManager> m_properties;
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYMANAGEROWNER_H_*/
