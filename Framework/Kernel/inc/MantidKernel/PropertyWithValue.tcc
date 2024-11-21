// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValue.h"

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

namespace Mantid {
namespace Kernel {

//------------------------------------------------------------------------------------------------
// Now the PropertyWithValue class itself
//------------------------------------------------------------------------------------------------

/** Constructor
 *  @param name :: The name to assign to the property
 *  @param defaultValue :: Is stored initial default value of the property
 *  @param validator :: The validator to use for this property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) property
 */
template <typename TYPE>
PropertyWithValue<TYPE>::PropertyWithValue(std::string name, TYPE defaultValue, IValidator_sptr validator,
                                           unsigned int direction)
    : Property(std::move(name), typeid(TYPE), direction), m_value(defaultValue),
      m_initialValue(std::move(defaultValue)), m_validator(std::move(validator)) {}

/** Constructor
 *  @param name :: The name to assign to the property
 *  @param defaultValue :: Is stored initial default value of the property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) property
 */
template <typename TYPE>
PropertyWithValue<TYPE>::PropertyWithValue(std::string name, TYPE defaultValue, unsigned int direction)
    : Property(std::move(name), typeid(TYPE), direction), m_value(defaultValue),
      m_initialValue(std::move(defaultValue)), m_validator(std::make_shared<NullValidator>()) {}

/*
  Constructor to handle vector value assignments to m_initialValue
  so they can be remembered when the algorithm dialog is reloaded.
*/
/**Constructor
 *  @param name :: The name to assign to the property.
 *  @param defaultValue :: A vector of numerical type, empty to comply with
 * other definitions.
 *  @param defaultValueStr :: The numerical values you wish to assign to the
 * property
 *  @param validator :: The validator to use for this property
 *  @param direction :: Whether this is a Direction::Input, Direction::Output
 * or Direction::InOut (Input & Output) property
 */
template <typename TYPE>
PropertyWithValue<TYPE>::PropertyWithValue(const std::string &name, TYPE defaultValue,
                                           const std::string &defaultValueStr, IValidator_sptr validator,
                                           unsigned int direction)
    : Property(name, typeid(TYPE), direction), m_value(extractToValueVector<TYPE>(defaultValueStr)),
      m_initialValue(m_value), m_validator(std::move(validator)) {
  UNUSED_ARG(defaultValue);
}

/**Copy constructor
 *  Note the default value of the copied object is the initial value of
 * original
 */
template <typename TYPE>
PropertyWithValue<TYPE>::PropertyWithValue(const PropertyWithValue<TYPE> &right)
    : Property(right), m_value(right.m_value), m_initialValue(right.m_initialValue), // the default is the initial
                                                                                     // value of the original object
      m_validator(right.m_validator->clone()) {}

/// 'Virtual copy constructor'
template <typename TYPE> PropertyWithValue<TYPE> *PropertyWithValue<TYPE>::clone() const {
  return new PropertyWithValue<TYPE>(*this);
}

template <typename TYPE> void PropertyWithValue<TYPE>::saveProperty(::NeXus::File * /*file*/) {
  // AppleClang 7.3 and later gives a -Winfinite-recursion warning if I call the
  // base class method. The function is small enough that reimplementing it
  // isn't a big deal.
  throw std::invalid_argument("PropertyWithValue::saveProperty - Cannot save '" + this->name() + "', property type " +
                              typeid(TYPE).name() + " not implemented.");
}

/** Get the value of the property as a string
 *  @return The property's value
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::value() const { return toString(m_value); }

/** Get the value of the property as a string
 *  @return The property's value
 */
template <typename TYPE>
std::string PropertyWithValue<TYPE>::valueAsPrettyStr(const size_t maxLength, const bool collapseLists) const {
  std::string retVal;
  try {
    retVal = toPrettyString(m_value, maxLength, collapseLists);
  } catch (boost::bad_lexical_cast &) {
    // toPrettyStringFailed, default to using toString instead
    retVal = Strings::shorten(value(), maxLength);
  }
  return retVal;
}

/**
 * Attempt to construct a Json::Value object from the plain value
 * @return A new Json::Value object
 */
template <typename TYPE> Json::Value PropertyWithValue<TYPE>::valueAsJson() const { return encodeAsJson((*this)()); }

/**
 * Deep comparison.
 * @param rhs The other property to compare to.
 * @return true if the are equal.
 */
template <typename TYPE> bool PropertyWithValue<TYPE>::operator==(const PropertyWithValue<TYPE> &rhs) const {
  if (this->name() != rhs.name())
    return false;
  return (m_value == rhs.m_value);
}

/**
 * Deep comparison (not equal).
 * @param rhs The other property to compare to.
 * @return true if the are not equal.
 */
template <typename TYPE> bool PropertyWithValue<TYPE>::operator!=(const PropertyWithValue<TYPE> &rhs) const {
  return !(*this == rhs);
}

/** Get the size of the property.
 */
template <typename TYPE> int PropertyWithValue<TYPE>::size() const { return findSize(m_value); }

/** Get the value the property was initialised with -its default value
 *  @return The default value
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::getDefault() const { return toString(m_initialValue); }

/** Set the value of the property from a string representation.
 *  Note that "1" & "0" must be used for bool properties rather than
 * true/false.
 *  @param value :: The value to assign to the property
 *  @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::setValue(const std::string &value) {
  try {
    TYPE result = m_value;
    std::string valueCopy = value;
    if (autoTrim()) {
      boost::trim(valueCopy);
    }
    toValue(valueCopy, result);
    // Uses the assignment operator defined below which runs isValid() and
    // throws based on the result
    *this = result;
    return "";
  } catch (boost::bad_lexical_cast &) {
    std::string error = "Could not set property " + name() + ". Can not convert \"" + value + "\" to " + type();
    g_logger.debug() << error;
    return error;
  } catch (std::invalid_argument &except) {
    g_logger.debug() << "Could not set property " << name() << ": " << except.what();
    return except.what();
  }
}

/**
 * Set the value of the property from a Json representation.
 * @param value :: The value to assign to the property
 * @return Returns "" if the assignment was successful or a user level
 * description of the problem
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::setValueFromJson(const Json::Value &value) {
  if (value.type() != Json::stringValue) {
    try {
      *this = decode<TYPE>(value);
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
template <typename TYPE> std::string PropertyWithValue<TYPE>::setDataItem(const std::shared_ptr<DataItem> &data) {
  // Pass of the helper function that is able to distinguish whether
  // the TYPE of the PropertyWithValue can be converted to a
  // shared_ptr<DataItem>
  return setTypedValue(data, std::is_convertible<TYPE, std::shared_ptr<DataItem>>());
}

/// Copy assignment operator assigns only the value and the validator not the
/// name, default (initial) value, etc.
template <typename TYPE> PropertyWithValue<TYPE> &PropertyWithValue<TYPE>::operator=(const PropertyWithValue &right) {
  if (&right == this)
    return *this;
  m_value = right.m_value;
  m_validator = right.m_validator->clone();
  return *this;
}

//--------------------------------------------------------------------------------------
/** Add the value of another property
 * @param right the property to add
 * @return the sum
 */
template <typename TYPE> PropertyWithValue<TYPE> &PropertyWithValue<TYPE>::operator+=(Property const *right) {
  PropertyWithValue const *rhs = dynamic_cast<PropertyWithValue const *>(right);

  if (rhs) {
    // This function basically does:
    //  m_value += rhs->m_value; for values
    //  or concatenates vectors for vectors
    addingOperator(m_value, rhs->m_value);
  } else
    g_logger.warning() << "PropertyWithValue " << this->name()
                       << " could not be added to another property of the "
                          "same name but incompatible type.\n";

  return *this;
}

//--------------------------------------------------------------------------------------
/** Assignment operator.
 *  Allows assignment of a new value to the property by writing,
 *  e.g., myProperty = 3;
 *  @param value :: The new value to assign to the property
 *  @return the reference to itself
 */
template <typename TYPE> PropertyWithValue<TYPE> &PropertyWithValue<TYPE>::operator=(const TYPE &value) {
  TYPE oldValue = m_value;
  if (std::is_same<TYPE, std::string>::value) {
    std::string valueCopy = toString(value);
    if (autoTrim()) {
      boost::trim(valueCopy);
    }
    toValue(valueCopy, m_value);
  } else {
    m_value = value;
  }
  std::string problem = this->isValid();
  if (problem.empty()) {
    return *this;
  } else if (problem == "_alias") {
    m_value = getValueForAlias(value);
    return *this;
  } else {
    m_value = oldValue;
    throw std::invalid_argument("When setting value of property \"" + this->name() + "\": " + problem);
  }
}

/** Allows you to get the value of the property via an expression like
 * myProperty()
 *  @returns the value of the property
 */
template <typename TYPE> const TYPE &PropertyWithValue<TYPE>::operator()() const { return m_value; }

/** Allows you to get the value of the property simply by typing its name.
 *  Means you can use an expression like: int i = myProperty;
 * @return the value
 */
template <typename TYPE> PropertyWithValue<TYPE>::operator const TYPE &() const { return m_value; }

/** Check the value chosen for the property is OK, unless overidden it just
 * calls the validator's isValid()
 *  N.B. Problems found in validator are written to the log
 *  if you this function to do checking outside a validator may want
 * to do more logging
 *  @returns "" if the value is valid or a discription of the problem
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::isValid() const { return m_validator->isValid(m_value); }

/** Indicates if the property's value is the same as it was when it was set
 *  N.B. Uses an unsafe comparison in the case of doubles, consider overriding
 * if the value is a pointer or floating point type
 *  @return true if the value is the same as the initial value or false
 * otherwise
 */
template <typename TYPE> bool PropertyWithValue<TYPE>::isDefault() const { return m_initialValue == m_value; }

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty vector.
 *  @return Returns the set of valid values for this property, or it returns
 * an empty vector.
 */
template <typename TYPE> std::vector<std::string> PropertyWithValue<TYPE>::allowedValues() const {
  return determineAllowedValues(m_value, *m_validator);
}

/** Returns the set of valid values for this property, if such a set exists.
 *  If not, it returns an empty vector.
 *  @return Returns the set of valid values for this property, or it returns
 * an empty vector.
 */
template <typename TYPE> bool PropertyWithValue<TYPE>::isMultipleSelectionAllowed() {
  return m_validator->isMultipleSelectionAllowed();
}

/**
 * Replace the current validator with the given one
 * @param newValidator :: A replacement validator
 */
template <typename TYPE> void PropertyWithValue<TYPE>::replaceValidator(IValidator_sptr newValidator) {
  m_validator = newValidator;
}

/**
 * Set the value of the property via a reference to another property.
 * If the value is unacceptable the value is not changed but a string is
 * returned.
 * The value is only accepted if the other property has the same type as this
 * @param right :: A reference to a property.
 */
template <typename TYPE> std::string PropertyWithValue<TYPE>::setValueFromProperty(const Property &right) {

  if (auto prop = dynamic_cast<const PropertyWithValue<TYPE> *>(&right)) {
    m_value = prop->m_value;
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
template <typename TYPE>
template <typename U>
std::string PropertyWithValue<TYPE>::setTypedValue(const U &value, const std::true_type &) {
  TYPE data = std::dynamic_pointer_cast<typename TYPE::element_type>(value);
  std::string msg;
  if (data) {
    try {
      (*this) = data;
    } catch (std::invalid_argument &exc) {
      msg = exc.what();
    }
  } else {
    msg = "Invalid DataItem. The object type (" + std::string(typeid(value).name()) +
          ") does not match the declared type of the property (" + std::string(this->type()) + ").";
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
template <typename TYPE>
template <typename U>
std::string PropertyWithValue<TYPE>::setTypedValue(const U &value, const std::false_type &) {
  UNUSED_ARG(value);
  return "Attempt to assign object of type DataItem to property (" + name() + ") of incorrect type";
}

/** Return value for a given alias.
 * @param alias :: An alias for a value. If a value cannot be found throw an
 * invalid_argument exception.
 * @return :: A value.
 */
template <typename TYPE> const TYPE PropertyWithValue<TYPE>::getValueForAlias(const TYPE &alias) const {
  std::string strAlias = toString(alias);
  std::string strValue = m_validator->getValueForAlias(strAlias);
  TYPE value;
  toValue(strValue, value);
  return value;
}

/**Returns the validator as a constant variable so it cannot be changed
 * @tparam TYPE :: The type of the property value
 * @return IValidator_sptr :: the validator
 */
template <typename TYPE> IValidator_sptr PropertyWithValue<TYPE>::getValidator() const { return m_validator; }

} // namespace Kernel
} // namespace Mantid
