// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_PROPERTYMANAGER_H_
#define MANTID_KERNEL_PROPERTYMANAGER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include <map>
#include <vector>

#include "MantidKernel/IPropertyManager.h"

namespace Mantid {
namespace Kernel {
//----------------------------------------------------------------------
// Forward Declaration
//----------------------------------------------------------------------
class SplittingInterval;
template <typename T> class TimeSeriesProperty;

/**
 Property manager helper class.
 This class is used by algorithms and services for helping to manage their own
 set of properties.

 N.B. ONCE YOU HAVE DECLARED A PROPERTY TO THE MANAGER IT IS OWNED BY THIS CLASS
 (declareProperty sinks the unique_ptr passed in.)

 Property values of any type except std::string can be extracted using
 getProperty().
 For assignment of string properties it is necessary to use getPropertyValue().

 @author Russell Taylor, Tessella Support Services plc
 @author Based on the Gaudi class PropertyMgr (see
 http://proj-gaudi.web.cern.ch/proj-gaudi/)
 @date 20/11/2007
 */
class MANTID_KERNEL_DLL PropertyManager : virtual public IPropertyManager {
public:
  PropertyManager();
  PropertyManager(const PropertyManager &);
  PropertyManager &operator=(const PropertyManager &);
  PropertyManager &operator+=(const PropertyManager &rhs);

  void filterByTime(const Types::Core::DateAndTime &start,
                    const Types::Core::DateAndTime &stop) override;
  void splitByTime(std::vector<SplittingInterval> &splitter,
                   std::vector<PropertyManager *> outputs) const override;
  void filterByProperty(const TimeSeriesProperty<bool> &filter) override;

  ~PropertyManager() override;

  // Function to declare properties (i.e. store them)
  void declareProperty(std::unique_ptr<Property> p,
                       const std::string &doc = "") override;
  using IPropertyManager::declareProperty;
  void declareOrReplaceProperty(std::unique_ptr<Property> p,
                                const std::string &doc = "") override;

  // Sets all the declare properties
  void setProperties(const std::string &propertiesJson,
                     const std::unordered_set<std::string> &ignoreProperties =
                         std::unordered_set<std::string>(),
                     bool createMissing = false) override;
  void setProperties(const std::string &propertiesJson,
                     IPropertyManager *targetPropertyManager,
                     const std::unordered_set<std::string> &ignoreProperties,
                     bool createMissing = false);
  void setProperties(const ::Json::Value &jsonValue,
                     const std::unordered_set<std::string> &ignoreProperties =
                         std::unordered_set<std::string>(),
                     bool createMissing = false) override;
  void setProperties(const ::Json::Value &jsonValue,
                     IPropertyManager *targetPropertyManager,
                     const std::unordered_set<std::string> &ignoreProperties =
                         std::unordered_set<std::string>(),
                     bool createMissing = false);
  void setPropertiesWithString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties =
          std::unordered_set<std::string>()) override;
  void setPropertyValue(const std::string &name,
                        const std::string &value) override;
  void setPropertyValueFromJson(const std::string &name,
                                const Json::Value &value) override;
  void setPropertyOrdinal(const int &index, const std::string &value) override;

  bool existsProperty(const std::string &name) const override;
  bool validateProperties() const override;
  size_t propertyCount() const override;
  std::string getPropertyValue(const std::string &name) const override;
  const std::vector<Property *> &getProperties() const override;

  /// removes the property from properties map
  void removeProperty(const std::string &name,
                      const bool delproperty = true) override;
  /// Clears the whole property map
  void clear() override;

  /// Get the value of a property
  TypedValue getProperty(const std::string &name) const override;
  /// Return the property manager serialized as a string.
  std::string asString(bool withDefaultValues = false) const override;
  /// Return the property manager serialized as a json object.
  ::Json::Value asJson(bool withDefaultValues = false) const override;

protected:
  friend class PropertyManagerOwner;

  Property *getPointerToProperty(const std::string &name) const override;
  Property *getPointerToPropertyOrdinal(const int &index) const override;
  Property *getPointerToPropertyOrNull(const std::string &name) const;

private:
  void setPropertiesWithSimpleString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties);
  void setPropertiesWithJSONString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties);

  /// typedef for the map holding the properties
  using PropertyMap = std::map<std::string, std::unique_ptr<Property>>;
  /// The properties under management
  PropertyMap m_properties;
  /// Stores the order in which the properties were declared.
  std::vector<Property *> m_orderedProperties;
};

/// Typedef for a shared pointer to a PropertyManager
using PropertyManager_sptr = boost::shared_ptr<PropertyManager>;

/// Return the value of the PropertyManager as a Json::Value
MANTID_KERNEL_DLL ::Json::Value encodeAsJson(const PropertyManager &propMgr);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_PROPERTYMANAGER_H_*/
