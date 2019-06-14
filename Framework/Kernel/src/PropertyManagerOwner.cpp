// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/PropertyManagerOwner.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyManager.h"
#include <algorithm>
#include <json/json.h>

namespace Mantid {
namespace Kernel {
namespace {
// Get a reference to the logger
Logger g_log("PropertyManagerOwner");
} // namespace

/// Default constructor
PropertyManagerOwner::PropertyManagerOwner()
    : m_properties(new PropertyManager) {}

/// Copy constructor
PropertyManagerOwner::PropertyManagerOwner(const PropertyManagerOwner &po) {
  m_properties = po.m_properties;
}

/// Assignment operator
PropertyManagerOwner &PropertyManagerOwner::
operator=(const PropertyManagerOwner &po) {
  m_properties = po.m_properties;
  return *this;
}

/** Add a property to the list of managed properties
 *  @param p :: The property object to add
 *  @param doc :: A description of the property that may be displayed to users
 *  @throw Exception::ExistsError if a property with the given name already
 * exists
 */
void PropertyManagerOwner::declareProperty(std::unique_ptr<Property> p,
                                           const std::string &doc) {
  m_properties->declareProperty(std::move(p), doc);
}

/** Add or replace property in the list of managed properties
 *  @param p :: The property object to add
 *  @param doc :: A description of the property that may be displayed to users
 */
void PropertyManagerOwner::declareOrReplaceProperty(std::unique_ptr<Property> p,
                                                    const std::string &doc) {
  m_properties->declareOrReplaceProperty(std::move(p), doc);
}

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
void PropertyManagerOwner::setProperties(
    const std::string &propertiesJson,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
  m_properties->setProperties(propertiesJson, this, ignoreProperties,
                              createMissing);
}

/** Sets all the declared properties from a json object
  @param jsonValue :: A json name value pair collection
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
  @param createMissing :: If the property does not exist then create it
*/
void PropertyManagerOwner::setProperties(
    const ::Json::Value &jsonValue,
    const std::unordered_set<std::string> &ignoreProperties,
    bool createMissing) {
  m_properties->setProperties(jsonValue, this, ignoreProperties, createMissing);
}

/** Sets all the declared properties from a string.
  @param propertiesString :: A list of name = value pairs separated by a
    semicolon
  @param ignoreProperties :: A set of names of any properties NOT to set
  from the propertiesArray
*/
void PropertyManagerOwner::setPropertiesWithString(
    const std::string &propertiesString,
    const std::unordered_set<std::string> &ignoreProperties) {
  m_properties->setPropertiesWithString(propertiesString, ignoreProperties);
}

/** Set the value of a property by string
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void PropertyManagerOwner::setPropertyValue(const std::string &name,
                                            const std::string &value) {
  m_properties->setPropertyValue(name, value);
  this->afterPropertySet(name);
}

/** Set the value of a property by Json::Value object
 *  @param name :: The name of the property (case insensitive)
 *  @param value :: The value to assign to the property
 *  @throw Exception::NotFoundError if the named property is unknown
 *  @throw std::invalid_argument If the value is not valid for the property
 * given
 */
void PropertyManagerOwner::setPropertyValueFromJson(const std::string &name,
                                                    const Json::Value &value) {
  m_properties->setPropertyValueFromJson(name, value);
  this->afterPropertySet(name);
}

/** Set the value of a property by an index
 *  N.B. bool properties must be set using 1/0 rather than true/false
 *  @param index :: The index of the property to assign
 *  @param value :: The value to assign to the property
 *  @throw std::runtime_error if the property index is too high
 */
void PropertyManagerOwner::setPropertyOrdinal(const int &index,
                                              const std::string &value) {
  m_properties->setPropertyOrdinal(index, value);
  this->afterPropertySet(
      m_properties->getPointerToPropertyOrdinal(index)->name());
}

/** Checks whether the named property is already in the list of managed
 * property.
 *  @param name :: The name of the property (case insensitive)
 *  @return True if the property is already stored
 */
bool PropertyManagerOwner::existsProperty(const std::string &name) const {
  return m_properties->existsProperty(name);
}

/** Validates all the properties in the collection
 *  @return True if all properties have a valid value
 */
bool PropertyManagerOwner::validateProperties() const {
  return m_properties->validateProperties();
}

/**
 * Count the number of properties under management
 * @returns The number of properties being managed
 */
size_t PropertyManagerOwner::propertyCount() const {
  return m_properties->propertyCount();
}

/** Get the value of a property as a string
 *  @param name :: The name of the property (case insensitive)
 *  @return The value of the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
std::string
PropertyManagerOwner::getPropertyValue(const std::string &name) const {
  return m_properties->getPropertyValue(name);
}

/** Get a property by name
 *  @param name :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw Exception::NotFoundError if the named property is unknown
 */
Property *
PropertyManagerOwner::getPointerToProperty(const std::string &name) const {
  return m_properties->getPointerToProperty(name);
}

/** Get a property by an index
 *  @param index :: The name of the property (case insensitive)
 *  @return A pointer to the named property
 *  @throw std::runtime_error if the property index is too high
 */
Property *
PropertyManagerOwner::getPointerToPropertyOrdinal(const int &index) const {
  return m_properties->getPointerToPropertyOrdinal(index);
}

/** Get the list of managed properties.
 *  The properties will be stored in the order that they were declared.
 *  @return A vector holding pointers to the list of properties
 */
const std::vector<Property *> &PropertyManagerOwner::getProperties() const {
  return m_properties->getProperties();
}

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
IPropertyManager::TypedValue
PropertyManagerOwner::getProperty(const std::string &name) const {
  return m_properties->getProperty(name);
}

/**
 * @param name
 * @return True if the property is its default value.
 */
bool PropertyManagerOwner::isDefault(const std::string &name) const {
  return m_properties->getPointerToProperty(name)->isDefault();
}

/**
 * Return the property manager serialized as a string.
 * The format is propName=value,propName=value,propName=value
 * @param withDefaultValues :: If true then the value of default parameters will
 * be included
 * @returns A stringized version of the manager
 */
std::string PropertyManagerOwner::asString(bool withDefaultValues) const {
  return m_properties->asString(withDefaultValues);
}
/**
 * Return the property manager serialized as a json object.
 * @param withDefaultValues :: If true then the value of default parameters will
 * be included
 * @returns A jsonValue of the manager
 */
::Json::Value PropertyManagerOwner::asJson(bool withDefaultValues) const {
  return m_properties->asJson(withDefaultValues);
}

/**
 * Removes the property from properties map.
 * @param name :: Name of the property to be removed.
 *  @param delproperty :: if true, delete the named property
 */
void PropertyManagerOwner::removeProperty(const std::string &name,
                                          const bool delproperty) {
  m_properties->removeProperty(name, delproperty);
}

/**
 * Clears all properties under management
 */
void PropertyManagerOwner::clear() { m_properties->clear(); }

/**
 * Override this method to perform a custom action right after a property was
 * set.
 * The argument is the property name. Default - do nothing.
 * @param name :: A property name.
 */
void PropertyManagerOwner::afterPropertySet(const std::string &name) {
  m_properties->afterPropertySet(name);
}

} // namespace Kernel
} // namespace Mantid
