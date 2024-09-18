// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidJson/Json.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/PropertyWithValueJSON.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidKernel/TimeROI.h"

#include <json/json.h>

namespace Mantid::Kernel {

using std::string;

namespace {
// static logger reference
Logger g_log("PropertyManager");

/**
 * Create a key for the Property
 * @param name The name of the property
 * @return The new key
 */
const std::string createKey(const std::string &name) {
  std::string key = name;
  std::transform(key.begin(), key.end(), key.begin(), toupper);
  return key;
}
} // namespace

const std::string PropertyManager::INVALID_VALUES_SUFFIX = "_invalid_values";
/// Gets the correct log name for the matching invalid values log for a given log name
std::string PropertyManager::getInvalidValuesFilterLogName(const std::string &logName) {
  return logName + PropertyManager::INVALID_VALUES_SUFFIX;
}
std::string PropertyManager::getLogNameFromInvalidValuesFilter(const std::string &logName) {
  std::string retVal = "";
  if (PropertyManager::isAnInvalidValuesFilterLog(logName)) {
    retVal = logName.substr(0, logName.size() - PropertyManager::INVALID_VALUES_SUFFIX.size());
  }
  return retVal;
}

/// Determine if the log's name has a substring indicating it should not be filtered
bool PropertyManager::isAnInvalidValuesFilterLog(const std::string &logName) {
  const auto ending = PropertyManager::INVALID_VALUES_SUFFIX;
  if (logName.length() >= ending.length()) {
    return (0 == logName.compare(logName.length() - ending.length(), ending.length(), ending));
  } else {
    return false;
  }
}
//-----------------------------------------------------------------------------------------------
/// Default constructor
PropertyManager::PropertyManager() : m_properties(), m_orderedProperties() {}

//-----------------------------------------------------------------------------------------------
/// copy constructor
/// @param other :: the PropertyManager to copy
PropertyManager::PropertyManager(const PropertyManager &other)
    : m_properties(), m_orderedProperties(other.m_orderedProperties.size()) {
  // We need to do a deep copy of the property pointers here
  for (unsigned int i = 0; i < m_orderedProperties.size(); ++i) {
    auto p = std::unique_ptr<Property>(other.m_orderedProperties[i]->clone());
    this->m_orderedProperties[i] = p.get();
    this->m_properties[createKey(p->name())] = std::move(p);
  }
}

//-----------------------------------------------------------------------------------------------
/// Assignment operator - performs a deep copy
/// @param other :: the PropertyManager to copy
/// @return pointer to this
PropertyManager &PropertyManager::operator=(const PropertyManager &other) {
  // We need to do a deep copy here
  if (this != &other) {
    this->m_properties.clear();
    this->m_orderedProperties.resize(other.m_orderedProperties.size());
    for (unsigned int i = 0; i < m_orderedProperties.size(); ++i) {
      auto p = std::unique_ptr<Property>(other.m_orderedProperties[i]->clone());
      this->m_orderedProperties[i] = p.get();
      this->m_properties[createKey(p->name())] = std::move(p);
    }
  }
  return *this;
}

//-----------------------------------------------------------------------------------------------
/// Virtual destructor
PropertyManager::~PropertyManager() { clear(); }

//-----------------------------------------------------------------------------------------------
/**
 * Addition operator
 * @param rhs :: The object that is being added to this.
 * @returns A reference to the summed object
 */
PropertyManager &PropertyManager::operator+=(const PropertyManager &rhs) {
  // Iterate through all properties on the RHS
  PropertyMap::const_iterator it;
  for (it = rhs.m_properties.begin(); it != rhs.m_properties.end(); ++it) {
    // The name on the rhs
    string rhs_name = it->first;
    try {
      Property *lhs_prop = this->getPointerToProperty(rhs_name);
      // Use the property's += operator to add THAT. Isn't abstraction fun?!
      (*lhs_prop) += it->second.get();
    } catch (Exception::NotFoundError &) {
      // The property isnt on the lhs.
      // Let's copy it
      auto copy = std::unique_ptr<Property>(it->second->clone());
      // And we add a copy of that property to *this
      this->declareProperty(std::move(copy), "");
    }
  }

  return *this;
}

//-----------------------------------------------------------------------------------------------
/**
 * Filter the managed properties by the given boolean property mask. It replaces
 * all time series properties with filtered time series properties
 * @param logFilter :: A LogFilter instance to filter each log on
 * @param excludedFromFiltering :: A string list of properties that
 * will be excluded from filtering
 */
void PropertyManager::filterByProperty(Mantid::Kernel::LogFilter *logFilter,
                                       const std::vector<std::string> &excludedFromFiltering) {
  auto filter = logFilter->filter();
  for (auto &orderedProperty : m_orderedProperties) {
    auto const propName = orderedProperty->name();
    if (std::find(excludedFromFiltering.cbegin(), excludedFromFiltering.cend(), propName) !=
        excludedFromFiltering.cend()) {
      // this log should be excluded from filtering
      continue;
    }

    if (auto doubleSeries = dynamic_cast<TimeSeriesProperty<double> *>(orderedProperty)) {
      // don't filter the invalid values filters
      if (PropertyManager::isAnInvalidValuesFilterLog(propName))
        break;
      std::unique_ptr<Property> filtered(nullptr);
      if (this->existsProperty(PropertyManager::getInvalidValuesFilterLogName(propName))) {

        // add the filter to the passed in filters
        auto filterProp = getPointerToProperty(PropertyManager::getInvalidValuesFilterLogName(propName));
        auto tspFilterProp = dynamic_cast<TimeSeriesProperty<bool> *>(filterProp);
        if (!tspFilterProp)
          break;
        logFilter->addFilter(*tspFilterProp);

        filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(doubleSeries, *logFilter->filter());
      } else if (filter->size() > 0) {
        // attach the filter to the TimeSeriesProperty, thus creating  the FilteredTimeSeriesProperty<double>
        filtered = std::make_unique<FilteredTimeSeriesProperty<double>>(doubleSeries, *filter);
      }
      if (filtered) {
        // Now replace in the map
        orderedProperty = filtered.get();
        this->m_properties[createKey(propName)] = std::move(filtered);
      }
    }
  }
}

/**
 * Create a partial copy of this object such that every time series property is cloned according to the input TimeROI.
 * A partially cloned time series property should include all time values enclosed by the ROI regions,
 * each defined as [roi_begin,roi_end], plus the values immediately before and after an ROI region, if available.
 * Properties that are not time series will be cloned with no changes.
 * @param timeROI :: time region of interest, i.e. time boundaries used to determine which time series values should be
 * included in the copy.
 */
PropertyManager *PropertyManager::cloneInTimeROI(const Kernel::TimeROI &timeROI) {
  PropertyManager *newMgr = new PropertyManager();
  newMgr->m_orderedProperties.reserve(m_orderedProperties.size());
  // We need to do a deep copy of the property pointers here
  for (const auto &prop : m_orderedProperties) {
    std::unique_ptr<Property> newProp;
    if (const auto tsp = dynamic_cast<const ITimeSeriesProperty *>(prop))
      newProp = std::unique_ptr<Property>(tsp->cloneInTimeROI(timeROI));
    else
      newProp = std::unique_ptr<Property>(prop->clone());
    newMgr->m_orderedProperties.emplace_back(newProp.get());
    newMgr->m_properties[createKey(newProp->name())] = std::move(newProp);
  }
  return newMgr;
}

/**
 * For time series properties, remove time values outside of TimeROI regions, each defined as [roi_begin,roi_end].
 * However, keep the values immediately before and after each ROI region, if available.
 * @param timeROI :: a series of time regions used to determine which values to remove or to keep
 */
void PropertyManager::removeDataOutsideTimeROI(const Kernel::TimeROI &timeROI) {
  for (auto prop : m_orderedProperties) {
    if (auto tsp = dynamic_cast<ITimeSeriesProperty *>(prop))
      tsp->removeDataOutsideTimeROI(timeROI);
  }
}

//-----------------------------------------------------------------------------------------------
/** Add a property to the list of managed properties
 *  @param p :: The property object to add (sinks the unique_ptr)
 *  @param doc :: A description of the property that may be displayed to users
 *  @throw Exception::ExistsError if a property with the given name already
 * exists
 *  @throw std::invalid_argument  if the property declared has an empty name.
 */
void PropertyManager::declareProperty(std::unique_ptr<Property> p, const std::string &doc) {
  p->setDocumentation(doc);

  const std::string key = createKey(p->name());
  auto existing = m_properties.find(key);
  if (existing == m_properties.end()) {
    m_orderedProperties.emplace_back(p.get());
    m_properties[key] = std::move(p);
  } else {
    // Don't error if this is actually the same property object!
    if (existing->second != p) {
      throw Exception::ExistsError("Property with given name already exists", key);
    }
  }
}

/** Add or replace a property in the list of managed properties
 *  @param p :: The property object to add (sinks the unique_ptr)
 *  @param doc :: A description of the property that may be displayed to users
 *  @throw std::invalid_argument  if the property declared has an empty name.
 */
void PropertyManager::declareOrReplaceProperty(std::unique_ptr<Property> p, const std::string &doc) {
  p->setDocumentation(doc);

  const std::string key = createKey(p->name());
  auto existing = m_properties.find(key);
  if (existing != std::end(m_properties)) {
    // replace it in the same position
    auto oldPropPtr = existing->second.get();
    auto ordereredPropPos = std::find(std::begin(m_orderedProperties), std::end(m_orderedProperties), oldPropPtr);
    // if the property exists it should be guaranteed to be in the ordered list
    // by declareProperty
    assert(ordereredPropPos != std::end(m_orderedProperties));
    *ordereredPropPos = p.get();
  } else {
    m_orderedProperties.emplace_back(p.get());
  }
  m_properties[key] = std::move(p);
}

/** Reset property values back to initial values (blank or default values)
 */
void PropertyManager::resetProperties() {
  for (auto &prop : getProperties()) {
    if (!prop->isDefault())
      prop->setValue(prop->getDefault());
  }
}

//-----------------------------------------------------------------------------------------------
/** Set the ordered list of properties by one string of values, separated by
 *semicolons.
 *
 * The string should be a json formatted collection of name value pairs
 *
 *  @param propertiesJson :: The string of property values
 *  @param ignoreProperties :: A set of names of any properties NOT to set
 *      from the propertiesArray
 *  @param createMissing :: If the property does not exist then create it
 *  @throw invalid_argument if error in parameters
 */
void PropertyManager::setProperties(const std::string &propertiesJson,
                                    const std::unordered_set<std::string> &ignoreProperties, bool createMissing) {
  setProperties(propertiesJson, this, ignoreProperties, createMissing);
}
//-----------------------------------------------------------------------------------------------
/** Set the ordered list of properties by one string of values, separated by
 *semicolons.
 *
 * The string should be a json formatted collection of name value pairs
 *
 *  @param propertiesJson :: The string of property values
 *  @param ignoreProperties :: A set of names of any properties NOT to set
 *      from the propertiesArray
 *  @param targetPropertyManager :: the propertymanager to make the changes to,
 *      most of the time this will be *this
 *  @param createMissing :: If the property does not exist then create it
 *  @throw invalid_argument if error in parameters
 */
void PropertyManager::setProperties(const std::string &propertiesJson, IPropertyManager *targetPropertyManager,
                                    const std::unordered_set<std::string> &ignoreProperties, bool createMissing) {
  ::Json::Value jsonValue;

  if (Mantid::JsonHelpers::parse(propertiesJson, &jsonValue)) {
    setProperties(jsonValue, targetPropertyManager, ignoreProperties, createMissing);
  } else {
    throw std::invalid_argument("propertiesArray was not valid json");
  }
}
//-----------------------------------------------------------------------------------------------
/** Set the ordered list of properties by a json value collection
 *
 *  @param jsonValue :: The jsonValue of property values
 *  @param ignoreProperties :: A set of names of any properties NOT to set
 *      from the propertiesArray
 *  @param createMissing :: If the property does not exist then create it
 */
void PropertyManager::setProperties(const ::Json::Value &jsonValue,
                                    const std::unordered_set<std::string> &ignoreProperties, bool createMissing) {
  setProperties(jsonValue, this, ignoreProperties, createMissing);
}

//-----------------------------------------------------------------------------------------------
/** Set the ordered list of properties by a json value collection
 *
 *  @param jsonValue :: The jsonValue of property values
 *  @param ignoreProperties :: A set of names of any properties NOT to set
 *      from the propertiesArray
 *  @param targetPropertyManager :: the propertymanager to make the changes to,
 *      most of the time this will be *this
 *  @param createMissing :: If the property does not exist then create it
 */
void PropertyManager::setProperties(const ::Json::Value &jsonValue, IPropertyManager *targetPropertyManager,
                                    const std::unordered_set<std::string> &ignoreProperties, bool createMissing) {
  if (jsonValue.type() != ::Json::ValueType::objectValue)
    return;

  // Some algorithms require Filename to be set first do that here
  static const std::string propFilename = "Filename";
  const ::Json::Value &filenameValue = jsonValue[propFilename];
  if (!filenameValue.isNull()) {
    const std::string value = filenameValue.asString();
    // Set it
    targetPropertyManager->setPropertyValue(propFilename, value);
  }
  const auto memberNames = jsonValue.getMemberNames();
  for (::Json::ArrayIndex i = 0; i < jsonValue.size(); i++) {
    const auto &propName = memberNames[i];
    if ((propFilename == propName) || (ignoreProperties.find(propName) != ignoreProperties.end())) {
      continue;
    }
    const ::Json::Value &propValue = jsonValue[propName];
    if (createMissing) {
      targetPropertyManager->declareOrReplaceProperty(decodeAsProperty(propName, propValue));
    } else {
      targetPropertyManager->setPropertyValueFromJson(propName, propValue);
    }
  }
}

/** Sets all the declared properties from a string.
  @param propertiesString :: Either a list of name = value pairs separated by a
    semicolon or a JSON code string.
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void PropertyManager::setPropertiesWithString(const std::string &propertiesString,
                                              const std::unordered_set<std::string> &ignoreProperties) {
  if (propertiesString.empty()) {
    return;
  }
  auto firstSymbol = propertiesString.find_first_not_of(" \n\t");
  if (firstSymbol == std::string::npos) {
    return;
  }
  if (propertiesString[firstSymbol] == '{') {
    setPropertiesWithJSONString(propertiesString, ignoreProperties);
  } else {
    setPropertiesWithSimpleString(propertiesString, ignoreProperties);
  }
}

/** Sets all the declared properties from a string.
  @param propertiesString :: A JSON code string.
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void PropertyManager::setPropertiesWithJSONString(const std::string &propertiesString,
                                                  const std::unordered_set<std::string> &ignoreProperties) {
  ::Json::Value propertyJson;

  if (Mantid::JsonHelpers::parse(propertiesString, &propertyJson)) {
    setProperties(propertyJson, ignoreProperties);
  } else {
    throw std::invalid_argument("Could not parse JSON string when trying to set a property from: " + propertiesString);
  }
}

/** Sets all the declared properties from a string.
  @param propertiesString :: A list of name = value pairs separated by a
    semicolon
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void PropertyManager::setPropertiesWithSimpleString(const std::string &propertiesString,
                                                    const std::unordered_set<std::string> &ignoreProperties) {
  ::Json::Value propertyJson;
  // Split up comma-separated properties
  using tokenizer = Mantid::Kernel::StringTokenizer;

  boost::char_separator<char> sep(";");
  tokenizer propPairs(propertiesString, ";", Mantid::Kernel::StringTokenizer::TOK_TRIM);
  // Iterate over the properties
  for (const auto &pair : propPairs) {
    size_t n = pair.find('=');
    if (n != std::string::npos) {
      // Normal "PropertyName=value" string.
      std::string propName;
      std::string value;

      // Extract the value string
      if (n < pair.size() - 1) {
        propName = pair.substr(0, n);
        value = pair.substr(n + 1, pair.size() - n - 1);
      } else {
        // String is "PropertyName="
        propName = pair.substr(0, n);
        value = "";
      }
      // Set it
      propertyJson[propName] = value;
    }
  }
  setProperties(propertyJson, ignoreProperties);
}

//-----------------------------------------------------------------------------------------------
/** Set the value of a property by string
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void PropertyManager::setPropertyValue(const std::string &name, const std::string &value) {
  auto *prop = getPointerToProperty(name);
  auto helpMsg = prop->setValue(value);
  afterPropertySet(name);
  if (!helpMsg.empty()) {
    helpMsg = "Invalid value for property " + prop->name() + " (" + prop->type() + ") from string \"" + value +
              "\": " + helpMsg;
    throw std::invalid_argument(helpMsg);
  }
}

/** Set the value of a property by Json::Value
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void PropertyManager::setPropertyValueFromJson(const std::string &name, const Json::Value &value) {
  auto *prop = getPointerToProperty(name);
  auto helpMsg = prop->setValueFromJson(value);
  afterPropertySet(name);
  if (!helpMsg.empty()) {
    helpMsg = "Invalid value for property " + prop->name() + " (" + prop->type() + ") from Json \"" +
              value.toStyledString() + "\": " + helpMsg;
    throw std::invalid_argument(helpMsg);
  }
}

//-----------------------------------------------------------------------------------------------
/** Set the value of a property by an index
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param index :: The index of the property to assign
 *  @param value :: The value to assign to the property
 *  @throw std::runtime_error if the property index is too high
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void PropertyManager::setPropertyOrdinal(const int &index, const std::string &value) {
  Property *p = getPointerToPropertyOrdinal(index); // throws runtime_error if property not in vector
  std::string errorMsg = p->setValue(value);
  this->afterPropertySet(p->name());
  if (!errorMsg.empty()) {
    errorMsg = "Invalid value for property " + p->name() + " (" + p->type() + ") \"" + value + "\" : " + errorMsg;
    throw std::invalid_argument(errorMsg);
  }
}

//-----------------------------------------------------------------------------------------------
/** Checks whether the named property is already in the list of managed
 * property.
 *  @param name :: The name of the property (case insensitive)
 *  @return True if the property is already stored
 */
bool PropertyManager::existsProperty(const std::string &name) const {
  const std::string key = createKey(name);
  auto it = m_properties.find(key);
  return (it != m_properties.end());
}

//-----------------------------------------------------------------------------------------------
/** Validates all the properties in the collection
 *  @return True if all properties have a valid value
 */
bool PropertyManager::validateProperties() const {
  bool allValid = true;
  for (const auto &property : m_properties) {
    // check for errors in each property
    std::string error = property.second->isValid();
    //"" means no error
    if (!error.empty()) {
      g_log.error() << "Property \"" << property.first << "\" is not set to a valid value: \"" << error << "\".\n";
      allValid = false;
    }
  }
  return allValid;
}

//-----------------------------------------------------------------------------------------------
/**
 * Count the number of properties under management
 * @returns The number of properties being managed
 */
size_t PropertyManager::propertyCount() const { return m_orderedProperties.size(); }

//-----------------------------------------------------------------------------------------------
/** Get the value of a property as a string
 *  @param name :: The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
std::string PropertyManager::getPropertyValue(const std::string &name) const {
  Property *p = getPointerToProperty(name); // throws NotFoundError if property not in vector
  return p->value();
}

//-----------------------------------------------------------------------------------------------
/** Return the property manager serialized as a string.
 *
 * The format is propName=value,propName=value,propName=value
 * @param withDefaultValues :: If true then the value of default parameters will
 *be included
 * @returns A serialized version of the manager
 */
std::string PropertyManager::asString(bool withDefaultValues) const {
  Json::StreamWriterBuilder builder;
  builder.settings_["indentation"] = "";
  const string output = Json::writeString(builder, asJson(withDefaultValues));

  return output;
}

//-----------------------------------------------------------------------------------------------
/** Return the property manager serialized as a json object.
 * Note that this method does not serialize WorkspaceProperties with workspaces not in the ADS.
 * @param withDefaultValues :: If true then the value of default parameters will be included
 * @returns A serialized version of the manager
 */
::Json::Value PropertyManager::asJson(bool withDefaultValues) const {
  ::Json::Value jsonMap;
  const auto count = static_cast<int>(propertyCount());
  for (int i = 0; i < count; ++i) {
    Property *p = getPointerToPropertyOrdinal(i);
    bool is_enabled = true;
    if (p->getSettings()) {
      is_enabled = p->getSettings()->isEnabled(this);
    }
    if (p->isValueSerializable() && (withDefaultValues || !p->isDefault()) && is_enabled) {
      jsonMap[p->name()] = p->valueAsJson();
    }
  }

  return jsonMap;
}

bool PropertyManager::operator==(const PropertyManager &other) const {
  if (other.m_properties.size() != m_properties.size())
    return false;
  for (const auto &[key, value] : m_properties) {
    if (other.m_properties.count(key) != 1)
      return false;
    if (*other.m_properties.at(key) != *value)
      return false;
  }
  return true;
}

bool PropertyManager::operator!=(const PropertyManager &other) const { return !this->operator==(other); }

//-----------------------------------------------------------------------------------------------
/** Get a property by name
 *  @param name :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
Property *PropertyManager::getPointerToProperty(const std::string &name) const {
  const std::string key = createKey(name);
  auto it = m_properties.find(key);
  if (it != m_properties.end()) {
    return it->second.get();
  }
  throw Exception::NotFoundError("Unknown property", name);
}

//-----------------------------------------------------------------------------------------------
/** Get a property by name
 *  @param name :: The name of the property (case insensitive)
 *  @return A pointer to the named property; NULL if not found
 */
Property *PropertyManager::getPointerToPropertyOrNull(const std::string &name) const {
  const std::string key = createKey(name);
  auto it = m_properties.find(key);
  if (it != m_properties.end()) {
    return it->second.get();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------------------------
/** Get a property by an index
 *  @param index :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw std::runtime_error if the property index is too high
 */
Property *PropertyManager::getPointerToPropertyOrdinal(const int &index) const {

  if (index < static_cast<int>(m_orderedProperties.size())) {
    return m_orderedProperties[index];
  }
  throw std::runtime_error("Property index too high");
}

//-----------------------------------------------------------------------------------------------
/** Get the list of managed properties.
 *  The properties will be stored in the order that they were declared.
 *  @return A vector holding pointers to the list of properties
 */
const std::vector<Property *> &PropertyManager::getProperties() const { return m_orderedProperties; }

//-----------------------------------------------------------------------------------------------
/**
 * Return the list of declared property names.
 * @return A vector holding strings of property names
 */
std::vector<std::string> PropertyManager::getDeclaredPropertyNames() const noexcept {
  std::vector<std::string> names;
  const auto &props = getProperties();
  names.reserve(props.size());
  std::transform(props.cbegin(), props.cend(), std::back_inserter(names),
                 [](auto &propPtr) { return propPtr->name(); });
  return names;
}

//-----------------------------------------------------------------------------------------------
/** Get the value of a property. Allows you to assign directly to a variable of
 *the property's type
 *  (if a supported type).
 *
 *  *** This method does NOT work for assigning to an existing std::string.
 *      In this case you have to use getPropertyValue() instead.
 *      Note that you can, though, construct a local string variable by writing,
 *      e.g. std::string s = getProperty("myProperty"). ***
 *
 *  @param name :: The name of the property
 *  @return The value of the property. Will be cast to the desired type (if a
 *supported type).
 *  @throw std::runtime_error If an attempt is made to assign a property to a
 *different type
 *  @throw Exception::NotFoundError If the property requested does not exist
 */
IPropertyManager::TypedValue PropertyManager::getProperty(const std::string &name) const {
  return TypedValue(*this, name);
}

//-----------------------------------------------------------------------------------------------
/** Removes the property from properties map.
 *  @param name ::  name of the property to be removed.
 *  @param delproperty :: if true, delete the named property
 */
void PropertyManager::removeProperty(const std::string &name, const bool delproperty) {
  if (existsProperty(name)) {
    // remove it
    Property *prop = getPointerToProperty(name);
    const std::string key = createKey(name);
    m_properties.erase(key);
    std::vector<Property *>::iterator itr;
    itr = find(m_orderedProperties.begin(), m_orderedProperties.end(), prop);
    m_orderedProperties.erase(itr);
    (void)delproperty; // not used
  }
}

/**
 * Removes a property from the properties map by index and return a pointer to it
 * @param index :: index of the property to be removed
 * @returns :: pointer to the removed property if found, NULL otherwise
 */
std::unique_ptr<Property> PropertyManager::takeProperty(const size_t index) {
  try {
    auto property = m_orderedProperties[index];
    const std::string key = createKey(property->name());
    auto propertyPtr = std::move(m_properties[key]);
    m_properties.erase(key);
    m_orderedProperties.erase(m_orderedProperties.cbegin() + index);
    return propertyPtr;
  } catch (const std::out_of_range &) {
    return NULL;
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Clears the whole property map
 */
void PropertyManager::clear() {
  m_orderedProperties.clear();
  m_properties.clear();
}

//-----------------------------------------------------------------------------------------------
/**
 * Creates a Json::Value of type objectValue to store the properties
 * @param propMgr A reference to a
 * @return A new Json::Value of type objectValue
 */
Json::Value encodeAsJson(const PropertyManager &propMgr) { return propMgr.asJson(true); }

} // namespace Mantid::Kernel
