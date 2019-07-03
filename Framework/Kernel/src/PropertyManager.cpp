// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/PropertyWithValueJSON.h"
#include "MantidKernel/StringTokenizer.h"

#include <json/json.h>

namespace Mantid {
namespace Kernel {

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
 * Filter out a run by time. Takes out any TimeSeriesProperty log entries
 *outside of the given
 *  absolute time range.
 *
 * @param start :: Absolute start time. Any log entries at times >= to this time
 *are kept.
 * @param stop :: Absolute stop time. Any log entries at times < than this time
 *are kept.
 */
void PropertyManager::filterByTime(const Types::Core::DateAndTime &start,
                                   const Types::Core::DateAndTime &stop) {
  // Iterate through all properties
  PropertyMap::const_iterator it;
  for (it = this->m_properties.begin(); it != this->m_properties.end(); ++it) {
    // Filter out the property
    auto prop = it->second.get();
    prop->filterByTime(start, stop);
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Split a run by time (splits the TimeSeriesProperties contained).
 *
 * Total proton charge will get re-integrated after filtering.
 *
 * @param splitter :: TimeSplitterType with the intervals and destinations.
 * @param outputs :: Vector of output runs.
 */
void PropertyManager::splitByTime(
    std::vector<SplittingInterval> &splitter,
    std::vector<PropertyManager *> outputs) const {
  size_t n = outputs.size();

  // Iterate through all properties
  PropertyMap::const_iterator it;
  for (it = this->m_properties.begin(); it != this->m_properties.end(); ++it) {
    // Filter out the property
    Property *prop = it->second.get();

    // Make a vector of the output properties contained in the other property
    // managers.
    //  NULL if it was not found.
    std::vector<Property *> output_properties;
    for (size_t i = 0; i < n; i++) {
      if (outputs[i])
        output_properties.push_back(
            outputs[i]->getPointerToPropertyOrNull(prop->name()));
      else
        output_properties.push_back(nullptr);
    }

    // Now the property does the splitting.
    bool isProtonCharge = prop->name() == "proton_charge";
    prop->splitByTime(splitter, output_properties, isProtonCharge);

  } // for each property
}

//-----------------------------------------------------------------------------------------------
/**
 * Filter the managed properties by the given boolean property mask. It replaces
 * all time
 * series properties with filtered time series properties
 * @param filter :: A boolean time series to filter each property on
 */
void PropertyManager::filterByProperty(
    const Kernel::TimeSeriesProperty<bool> &filter) {
  for (auto &orderedProperty : m_orderedProperties) {
    Property *currentProp = orderedProperty;
    if (auto doubleSeries =
            dynamic_cast<TimeSeriesProperty<double> *>(currentProp)) {
      std::unique_ptr<Property> filtered =
          std::make_unique<FilteredTimeSeriesProperty<double>>(doubleSeries,
                                                               filter);
      orderedProperty = filtered.get();
      // Now replace in the map
      this->m_properties[createKey(currentProp->name())] = std::move(filtered);
    }
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
void PropertyManager::declareProperty(std::unique_ptr<Property> p,
                                      const std::string &doc) {
  p->setDocumentation(doc);

  const std::string key = createKey(p->name());
  auto existing = m_properties.find(key);
  if (existing == m_properties.end()) {
    m_orderedProperties.push_back(p.get());
    m_properties[key] = std::move(p);
  } else {
    // Don't error if this is actually the same property object!
    if (existing->second != p) {
      throw Exception::ExistsError("Property with given name already exists",
                                   key);
    }
  }
}

/** Add or replace a property in the list of managed properties
 *  @param p :: The property object to add (sinks the unique_ptr)
 *  @param doc :: A description of the property that may be displayed to users
 *  @throw std::invalid_argument  if the property declared has an empty name.
 */
void PropertyManager::declareOrReplaceProperty(std::unique_ptr<Property> p,
                                               const std::string &doc) {
  p->setDocumentation(doc);

  const std::string key = createKey(p->name());
  auto existing = m_properties.find(key);
  if (existing != std::end(m_properties)) {
    // replace it in the same position
    auto oldPropPtr = existing->second.get();
    auto ordereredPropPos =
        std::find(std::begin(m_orderedProperties),
                  std::end(m_orderedProperties), oldPropPtr);
    // if the property exists it should be guaranteed to be in the ordered list
    // by declareProperty
    assert(ordereredPropPos != std::end(m_orderedProperties));
    *ordereredPropPos = p.get();
  } else {
    m_orderedProperties.push_back(p.get());
  }
  m_properties[key] = std::move(p);
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
void PropertyManager::setProperties(
    const std::string &propertiesJson,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
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
void PropertyManager::setProperties(
    const std::string &propertiesJson, IPropertyManager *targetPropertyManager,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
  ::Json::Reader reader;
  ::Json::Value jsonValue;

  if (reader.parse(propertiesJson, jsonValue)) {
    setProperties(jsonValue, targetPropertyManager, ignoreProperties,
                  createMissing);
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
void PropertyManager::setProperties(
    const ::Json::Value &jsonValue,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
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
void PropertyManager::setProperties(
    const ::Json::Value &jsonValue, IPropertyManager *targetPropertyManager,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
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
    if ((propFilename == propName) ||
        (ignoreProperties.find(propName) != ignoreProperties.end())) {
      continue;
    }
    const ::Json::Value &propValue = jsonValue[propName];
    if (createMissing) {
      targetPropertyManager->declareOrReplaceProperty(
          decodeAsProperty(propName, propValue));
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
void PropertyManager::setPropertiesWithString(
    const std::string &propertiesString,
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
void PropertyManager::setPropertiesWithJSONString(
    const std::string &propertiesString,
    const std::unordered_set<std::string> &ignoreProperties) {
  ::Json::Reader reader;
  ::Json::Value propertyJson;

  if (reader.parse(propertiesString, propertyJson)) {
    setProperties(propertyJson, ignoreProperties);
  } else {
    throw std::invalid_argument(
        "Could not parse JSON string when trying to set a property from: " +
        propertiesString);
  }
}

/** Sets all the declared properties from a string.
  @param propertiesString :: A list of name = value pairs separated by a
    semicolon
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void PropertyManager::setPropertiesWithSimpleString(
    const std::string &propertiesString,
    const std::unordered_set<std::string> &ignoreProperties) {
  ::Json::Value propertyJson;
  // Split up comma-separated properties
  using tokenizer = Mantid::Kernel::StringTokenizer;

  boost::char_separator<char> sep(";");
  tokenizer propPairs(propertiesString, ";",
                      Mantid::Kernel::StringTokenizer::TOK_TRIM);
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
void PropertyManager::setPropertyValue(const std::string &name,
                                       const std::string &value) {
  auto *prop = getPointerToProperty(name);
  auto helpMsg = prop->setValue(value);
  afterPropertySet(name);
  if (!helpMsg.empty()) {
    helpMsg = "Invalid value for property " + prop->name() + " (" +
              prop->type() + ") from string \"" + value + "\": " + helpMsg;
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
void PropertyManager::setPropertyValueFromJson(const std::string &name,
                                               const Json::Value &value) {
  auto *prop = getPointerToProperty(name);
  auto helpMsg = prop->setValueFromJson(value);
  afterPropertySet(name);
  if (!helpMsg.empty()) {
    helpMsg = "Invalid value for property " + prop->name() + " (" +
              prop->type() + ") from Json \"" + value.toStyledString() +
              "\": " + helpMsg;
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
void PropertyManager::setPropertyOrdinal(const int &index,
                                         const std::string &value) {
  Property *p = getPointerToPropertyOrdinal(
      index); // throws runtime_error if property not in vector
  std::string errorMsg = p->setValue(value);
  this->afterPropertySet(p->name());
  if (!errorMsg.empty()) {
    errorMsg = "Invalid value for property " + p->name() + " (" + p->type() +
               ") \"" + value + "\" : " + errorMsg;
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
      g_log.error() << "Property \"" << property.first
                    << "\" is not set to a valid value: \"" << error << "\".\n";
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
size_t PropertyManager::propertyCount() const {
  return m_orderedProperties.size();
}

//-----------------------------------------------------------------------------------------------
/** Get the value of a property as a string
 *  @param name :: The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
std::string PropertyManager::getPropertyValue(const std::string &name) const {
  Property *p = getPointerToProperty(
      name); // throws NotFoundError if property not in vector
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
  ::Json::FastWriter writer;
  const string output = writer.write(asJson(withDefaultValues));

  return output;
}

//-----------------------------------------------------------------------------------------------
/** Return the property manager serialized as a json object.
 * Note that this method does not serialize WorkspacePropertys with workspaces
 * not
 * in the ADS.
 * @param withDefaultValues :: If true then the value of default parameters
 * will be included
 * @returns A serialized version of the manager
 */
::Json::Value PropertyManager::asJson(bool withDefaultValues) const {
  ::Json::Value jsonMap;
  const int count = static_cast<int>(propertyCount());
  for (int i = 0; i < count; ++i) {
    Property *p = getPointerToPropertyOrdinal(i);
    bool is_enabled = true;
    if (p->getSettings()) {
      is_enabled = p->getSettings()->isEnabled(this);
    }
    if (p->isValueSerializable() && (withDefaultValues || !p->isDefault()) &&
        is_enabled) {
      jsonMap[p->name()] = p->valueAsJson();
    }
  }

  return jsonMap;
}

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
Property *
PropertyManager::getPointerToPropertyOrNull(const std::string &name) const {
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
const std::vector<Property *> &PropertyManager::getProperties() const {
  return m_orderedProperties;
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
PropertyManager::TypedValue
PropertyManager::getProperty(const std::string &name) const {
  return TypedValue(*this, name);
}

//-----------------------------------------------------------------------------------------------
/** Removes the property from properties map.
 *  @param name ::  name of the property to be removed.
 *  @param delproperty :: if true, delete the named property
 */
void PropertyManager::removeProperty(const std::string &name,
                                     const bool delproperty) {
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
Json::Value encodeAsJson(const PropertyManager &propMgr) {
  return propMgr.asJson(true);
}

} // namespace Kernel
} // namespace Mantid
