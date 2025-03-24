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
class TimeROI;
class LogFilter;

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
  static const std::string INVALID_VALUES_SUFFIX;
  // Gets the correct log name for the matching invalid values log for a given
  // log name
  static std::string getInvalidValuesFilterLogName(const std::string &logName);
  static std::string getLogNameFromInvalidValuesFilter(const std::string &logName);
  /// Determine if the log's name has a substring indicating it should not be filtered
  static bool isAnInvalidValuesFilterLog(const std::string &logName);

  PropertyManager();
  PropertyManager(const PropertyManager &);
  PropertyManager &operator=(const PropertyManager &);
  PropertyManager &operator+=(const PropertyManager &rhs);

  void filterByProperty(Mantid::Kernel::LogFilter *logFilter,
                        const std::vector<std::string> &excludedFromFiltering = std::vector<std::string>()) override;

  // Create a new PropertyManager with a partial copy of its time series properties according to TimeROI
  PropertyManager *cloneInTimeROI(const Kernel::TimeROI &timeROI);

  // For the time series properties, remove values according to TimeROI
  void removeDataOutsideTimeROI(const Kernel::TimeROI &timeROI);

  virtual ~PropertyManager() override;

  // Function to declare properties (i.e. store them)
  void declareProperty(std::unique_ptr<Property> p, const std::string &doc = "") override;
  using IPropertyManager::declareProperty;
  void declareOrReplaceProperty(std::unique_ptr<Property> p, const std::string &doc = "") override;
  void resetProperties() override;
  // Sets all the declare properties
  void setProperties(const std::string &propertiesJson,
                     const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                     bool createMissing = false) override;
  void setProperties(const std::string &propertiesJson, IPropertyManager *targetPropertyManager,
                     const std::unordered_set<std::string> &ignoreProperties, bool createMissing = false);
  void setProperties(const ::Json::Value &jsonValue,
                     const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                     bool createMissing = false) override;
  void setProperties(const ::Json::Value &jsonValue, IPropertyManager *targetPropertyManager,
                     const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>(),
                     bool createMissing = false);
  void setPropertiesWithString(
      const std::string &propertiesString,
      const std::unordered_set<std::string> &ignoreProperties = std::unordered_set<std::string>()) override;
  void setPropertyValue(const std::string &name, const std::string &value) override;
  void setPropertyValueFromJson(const std::string &name, const Json::Value &value) override;
  void setPropertyOrdinal(const int &index, const std::string &value) override;

  bool existsProperty(const std::string &name) const override;
  bool validateProperties() const override;
  size_t propertyCount() const override;
  std::string getPropertyValue(const std::string &name) const override;
  const std::vector<Property *> &getProperties() const override;
  std::vector<std::string> getDeclaredPropertyNames() const noexcept override;

  /// removes the property from properties map
  void removeProperty(const std::string &name, const bool delproperty = true) override;
  /// removes the property from the properties map and returns a pointer to it
  std::unique_ptr<Property> takeProperty(const size_t index) override;
  /// Clears the whole property map
  void clear() override final;

  /// Get the value of a property
  TypedValue getProperty(const std::string &name) const override;
  /// Return the property manager serialized as a string.
  std::string asString(bool withDefaultValues = false) const override;
  /// Return the property manager serialized as a json object.
  ::Json::Value asJson(bool withDefaultValues = false) const override;

  bool operator==(const PropertyManager &other) const;
  bool operator!=(const PropertyManager &other) const;

  Property *getPointerToProperty(const std::string &name) const override;

protected:
  friend class PropertyManagerOwner;

  Property *getPointerToPropertyOrdinal(const int &index) const override;
  Property *getPointerToPropertyOrNull(const std::string &name) const;

private:
  void setPropertiesWithSimpleString(const std::string &propertiesString,
                                     const std::unordered_set<std::string> &ignoreProperties);
  void setPropertiesWithJSONString(const std::string &propertiesString,
                                   const std::unordered_set<std::string> &ignoreProperties);

  /// typedef for the map holding the properties
  using PropertyMap = std::map<std::string, std::unique_ptr<Property>>;
  /// The properties under management
  PropertyMap m_properties;
  /// Stores the order in which the properties were declared.
  std::vector<Property *> m_orderedProperties;
};

/// Typedef for a shared pointer to a PropertyManager
using PropertyManager_sptr = std::shared_ptr<PropertyManager>;

/// Return the value of the PropertyManager as a Json::Value
MANTID_KERNEL_DLL ::Json::Value encodeAsJson(const PropertyManager &propMgr);

} // namespace Kernel
} // namespace Mantid
