// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/EnumeratedStringProperty.h"

#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/NullValidator.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyHelper.h"
#include "MantidKernel/PropertyWithValueJSON.h"
#include "MantidKernel/Strings.h"

#ifndef Q_MOC_RUN
#include <boost/algorithm/string/trim.hpp>
#include <memory>
#endif

#include <json/value.h>
#include <nexus/NeXusFile.hpp>

#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/StringTokenizer.h"
#include <type_traits>
#include <vector>

namespace Mantid::Kernel {

//------------------------------------------------------------------------------------------------
// Now the PropertyWithValue class itself
//------------------------------------------------------------------------------------------------

//######################################################//
//    CONSTRUCTORS
//######################################################//

/** Constructor
 *  @param name :: The name to assign to the property
 *  @param defaultValue :: Is stored initial default value of the property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) property
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names>::EnumeratedStringProperty(std::string const &name, ENUMSTRING const &defaultValue,
                                                             Direction::Type const direction)
    : Property(std::move(name), typeid(ENUMSTRING), direction), m_value(defaultValue),
      m_initialValue(std::move(defaultValue)) {}

/**Copy constructor
 *  Note the default value of the copied object is the initial value of original
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names>::EnumeratedStringProperty(EnumeratedStringProperty const &right)
    : Property(right), m_value(right.m_value), m_initialValue(right.m_initialValue) {
} // the default is the initial value of the original object

/// 'Virtual copy constructor'
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> *EnumeratedStringProperty<E, names>::clone() const {
  return new EnumeratedStringProperty<E, names>(*this);
}

/// Copy assignment operator assigns only the value and the validator not the
/// name, default (initial) value, etc.
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> &
EnumeratedStringProperty<E, names>::operator=(EnumeratedStringProperty const &right) {
  if (&right == this)
    return *this;
  this->m_value = right.m_value;
  return *this;
}

//######################################################//
//    GETTERS
//######################################################//

/** Get the value of the property as a string
 *  @return The property's value as a string
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::value() const {
  return static_cast<std::string>(m_value);
}

/** Get the value of the property as a more prettier string
 *  @return The property's value as a more prettier string
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::valueAsPrettyStr(std::size_t const maxLength,
                                                                 bool const collapseLists) const {
  return toPrettyString(static_cast<std::string>(m_value), maxLength, collapseLists);
}

/**
 * Attempt to construct a Json::Value object from the plain value
 * @return A new Json::Value object
 */
template <class E, std::vector<std::string> const *const names>
Json::Value EnumeratedStringProperty<E, names>::valueAsJson() const {
  return encodeAsJson((*this)());
}

/**
 * Deep comparison.
 * @param rhs The other property to compare to.
 * @return true if the are equal.
 */
template <class E, std::vector<std::string> const *const names>
bool EnumeratedStringProperty<E, names>::operator==(EnumeratedStringProperty const &rhs) const {
  if (this->name() != rhs.name())
    return false;
  return (static_cast<E>(this->m_value) == static_cast<E>(rhs.m_value));
}

/**
 * Deep comparison (not equal).
 * @param rhs The other property to compare to.
 * @return true if they are not equal.
 */
template <class E, std::vector<std::string> const *const names>
bool EnumeratedStringProperty<E, names>::operator!=(EnumeratedStringProperty const &rhs) const {
  return !(*this == rhs);
}

/** Get the size of the property.
 */
template <class E, std::vector<std::string> const *const names> int EnumeratedStringProperty<E, names>::size() const {
  return 1;
}

/** Get the value the property was initialised with -its default value
 *  @return The default value
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::getDefault() const {
  return static_cast<std::string>(m_initialValue);
}

/** Allows you to get the value of the property simply by typing its name.
 *  Means you can use an expression like: int i = myProperty();
 * @return the value
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedString<E, names> EnumeratedStringProperty<E, names>::operator()() const {
  return m_value;
}

/** If the value has been set, then it is valid.
 *  @returns "" if the value is valid or a discription of the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::isValid() const {
  if (m_value.size() != 0)
    return "";
  else
    return "EnumeratedStringProperty was not set with valid EnumeratedString.\n";
}

/** Indicates if the property's value is the same as it was when it was set
 *  N.B. Uses an unsafe comparison in the case of doubles, consider overriding
 * if the value is a pointer or floating point type
 *  @return true if the value is the same as the initial value or false
 * otherwise
 */
template <class E, std::vector<std::string> const *const names>
bool EnumeratedStringProperty<E, names>::isDefault() const {
  return m_initialValue == m_value;
}

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty vector.
 *  @return Returns the set of valid values for this property, or it returns
 * an empty vector.
 */
template <class E, std::vector<std::string> const *const names>
std::vector<std::string> EnumeratedStringProperty<E, names>::allowedValues() const {
  return *names;
}

/** Returns true, as multiple selection is allowed.
 *  @return true
 */
template <class E, std::vector<std::string> const *const names>
bool EnumeratedStringProperty<E, names>::isMultipleSelectionAllowed() {
  return true;
}

//######################################################//
//    SETTERS
//######################################################//

/** Set the value of the property from a string representation.
 *  @param value :: The value to assign to the property
 *  @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setValue(E const value) {
  this->m_value = static_cast<EnumeratedString<E, names>>(value);
  return "";
}

/** Set the value of the property from a string representation.
 *  @param value :: The value to assign to the property
 *  @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setValue(std::string const &value) {
  this->m_value = static_cast<EnumeratedString<E, names>>(value);
  return "";
}

/** Set the value of the property from a string representation.
 *  @param value :: The value to assign to the property
 *  @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setValue(EnumeratedString<E, names> const &value) {
  this->m_value = value;
  return "";
}

/**
 * Set the value of the property from a Json representation.
 * @param value :: The value to assign to the property
 * @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setValueFromJson(Json::Value const &value) {
  if (value.type() != Json::stringValue) {
    try {
      *this = decode<E>(value);
    } catch (std::invalid_argument &exc) {
      return exc.what();
    }
    return "";
  } else {
    return setValue(value.asString());
  }
}

/**
 * Set a property value via a DataItem
 * @param data :: A shared pointer to a data item
 * @return "" if the assignment was successful or a user level description of
 * the problem
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setDataItem(std::shared_ptr<DataItem> const &data) {
  // Pass of the helper function that is able to distinguish whether
  // the TYPE of the PropertyWithValue can be converted to a
  // shared_ptr<DataItem>
  return setTypedValue(data, std::is_convertible<EnumeratedString<E, names>, std::shared_ptr<DataItem>>());
}

//--------------------------------------------------------------------------------------
/** Assignment operator.
 *  Allows assignment of a new value to the property by writing,
 *  e.g., myProperty = 3;
 *  @param value :: The new value to assign to the property
 *  @return the reference to itself
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> const &EnumeratedStringProperty<E, names>::operator=(E const value) {
  this->m_value = static_cast<EnumeratedString<E, names>>(value);
  return *this;
}

//--------------------------------------------------------------------------------------
/** Assignment operator.
 *  Allows assignment of a new value to the property by writing,
 *  e.g., myProperty = 3;
 *  @param value :: The new value to assign to the property
 *  @return the reference to itself
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> const &EnumeratedStringProperty<E, names>::operator=(std::string const &value) {
  this->m_value = static_cast<EnumeratedString<E, names>>(value);
  return *this;
}

//--------------------------------------------------------------------------------------
/** Assignment operator.
 *  Allows assignment of a new value to the property by writing,
 *  e.g., myProperty = 3;
 *  @param value :: The new value to assign to the property
 *  @return the reference to itself
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> const &
EnumeratedStringProperty<E, names>::operator=(EnumeratedString<E, names> const &value) {
  this->m_value = value;
  return *this;
}

//######################################################//
//    MUTATORS AND SUNDRY
//######################################################//

//--------------------------------------------------------------------------------------
/** Add the value of another property
 * @param right the property to add
 * @return the sum
 */
template <class E, std::vector<std::string> const *const names>
EnumeratedStringProperty<E, names> &EnumeratedStringProperty<E, names>::operator+=(Property const *right) {
  throw std::invalid_argument("Cannot add EnumeratedStringProperty, addition not implemented.\n");
}

template <class E, std::vector<std::string> const *const names>
void EnumeratedStringProperty<E, names>::saveProperty(::NeXus::File * /*file*/) {
  // AppleClang 7.3 and later gives a -Winfinite-recursion warning if I call the
  // base class method. The function is small enough that reimplementing it
  // isn't a big deal.
  throw std::invalid_argument("PropertyWithValue::saveProperty - Cannot save '" + this->name() + "', property type " +
                              typeid(ENUMSTRING).name() + " not implemented.");
}

//######################################################//
//    PRIVATE METHODS
//######################################################//

/**
 * Set the value of the property via a reference to another property.
 * If the value is unacceptable the value is not changed but a string is
 * returned.
 * The value is only accepted if the other property has the same type as this
 * @param right :: A reference to a property.
 */
template <class E, std::vector<std::string> const *const names>
std::string EnumeratedStringProperty<E, names>::setValueFromProperty(Property const &right) {

  if (auto prop = dynamic_cast<EnumeratedStringProperty<E, names> const *>(&right)) {
    this->m_value = prop->m_value;
    return "";
  } else {
    return setValue(right.value());
  }
}

/**
 * Helper function for setValue(DataItem_sptr). Uses boost type traits to
 * ensure
 * it is only used if U is a type that is convertible to
 * std::shared_ptr<DataItem>
 * @param value :: A object of type convertible to std::shared_ptr<DataItem>
 */
template <class E, std::vector<std::string> const *const names>
template <typename U>
std::string EnumeratedStringProperty<E, names>::setTypedValue(U const &value, std::true_type const &) {
  std::string msg;
  try {
    m_value = EnumeratedString<E, names>(value);
  } catch (std::runtime_error &exc) {
    msg = exc.what();
  }
  return msg;
}

/**
 * Helper function for setValue(DataItem_sptr). Uses boost type traits to
 * ensure
 * it is only used if U is NOT a type that is convertible to
 * std::shared_ptr<DataItem>
 * @param value :: A object of type convertible to std::shared_ptr<DataItem>
 */
template <class E, std::vector<std::string> const *const names>
template <typename U>
std::string EnumeratedStringProperty<E, names>::setTypedValue(U const &, std::false_type const &) {
  return "Attempt to assign object of type DataItem to property (" + name() + ") of incorrect type";
}

} // namespace Mantid::Kernel
