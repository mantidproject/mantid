// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Exception.h"
#include "MantidKernel/TypedValidator.h"
#ifndef Q_MOC_RUN
#include <boost/lexical_cast.hpp>
#include <memory>
#endif
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

namespace Mantid {
namespace Kernel {
/** ListValidator is a validator that requires the value of a property to be one
   of a defined list
    of possibilities. The default type is std::string

    @author Russell Taylor, Tessella Support Services plc
    @date 18/06/2008
*/
template <typename TYPE> class ListValidator : public TypedValidator<TYPE> {
public:
  /// Default constructor. Sets up an empty list of valid values.
  ListValidator() : TypedValidator<TYPE>(), m_allowMultiSelection(false) {}

  /** Constructor
   *  @param values :: An iterable type of the valid values
   *  @param aliases :: Optional aliases for the valid values.
   *  @param allowMultiSelection :: True if the list allows multi selection
   */
  template <typename T>
  explicit ListValidator(const T &values,
                         const std::map<std::string, std::string> &aliases = std::map<std::string, std::string>(),
                         const bool allowMultiSelection = false)
      : TypedValidator<TYPE>(), m_allowedValues(values.begin(), values.end()),
        m_aliases(aliases.begin(), aliases.end()), m_allowMultiSelection(allowMultiSelection) {
    if (m_allowMultiSelection) {
      throw Kernel::Exception::NotImplementedError("The List Validator does not support multi selection yet");
    }
    for (auto aliasIt = m_aliases.begin(); aliasIt != m_aliases.end(); ++aliasIt) {
      if (values.end() == std::find(values.begin(), values.end(), boost::lexical_cast<TYPE>(aliasIt->second))) {
        throw std::invalid_argument("Alias " + aliasIt->first + " refers to invalid value " + aliasIt->second);
      }
    }
  }

  /// Clone the validator
  IValidator_sptr clone() const override { return std::make_shared<ListValidator<TYPE>>(*this); }
  /**
   * Returns the set of allowed values currently defined
   * @returns A set of allowed values that this validator will currently allow
   */
  std::vector<std::string> allowedValues() const override {
    /// The interface requires strings
    std::vector<std::string> allowedStrings;
    allowedStrings.reserve(m_allowedValues.size());
    auto cend = m_allowedValues.end();
    for (auto cit = m_allowedValues.begin(); cit != cend; ++cit) {
      allowedStrings.emplace_back(boost::lexical_cast<std::string>(*cit));
    }
    return allowedStrings;
  }

  /**
   * Add value to the list of allowable values if it's not already there
   * @param value :: A value of the templated type
   */
  void addAllowedValue(const TYPE &value) {
    // add only new values
    if (std::find(m_allowedValues.begin(), m_allowedValues.end(), value) == m_allowedValues.end()) {
      m_allowedValues.emplace_back(value);
    }
  }

  /**
   * Return an allowed value (as a string) given an alias.
   * @param alias :: An alias string.
   * @return :: Allowed value or throw if alias is unknown.
   */
  std::string getValueForAlias(const std::string &alias) const override {
    auto aliasIt = m_aliases.find(alias);
    if (aliasIt == m_aliases.end()) {
      throw std::invalid_argument("Unknown alias found " + alias);
    }
    return aliasIt->second;
  }

  bool isMultipleSelectionAllowed() override { return m_allowMultiSelection; }

  void setMultipleSelectionAllowed(const bool isMultiSelectionAllowed) {
    if (isMultiSelectionAllowed) {
      throw Kernel::Exception::NotImplementedError("The List Validator does not support Multi selection yet");
    }
    m_allowMultiSelection = isMultiSelectionAllowed;
  }

protected:
  /** Checks if the string passed is in the list
   *  @param value :: The value to test
   *  @return "" if the value is on the list, or "The value is not in the list
   * of allowed values"
   */
  std::string checkValidity(const TYPE &value) const override {
    if (m_allowedValues.end() != std::find(m_allowedValues.begin(), m_allowedValues.end(), value)) {
      return "";
    } else {
      // For a generic type, isEmpty always returns false, but if TYPE is std::string,
      // then the string version of isEmpty will be used, which could be true or false.
      // cppcheck-suppress knownConditionTrueFalse
      if (isEmpty(value))
        return "Select a value";
      if (isAlias(value))
        return "_alias";
      std::ostringstream os;
      os << "The value \"" << value << "\" is not in the list of allowed values";
      return os.str();
    }
  }

  /**
   * Is the value considered empty.
   * @param value :: The value to check
   * @return True if it is considered empty
   */
  template <typename T> bool isEmpty(const T &value) const { UNUSED_ARG(value) return false; }
  /**
   * Is the value considered empty. Specialized string version to use empty
   * @param value :: The value to check
   * @return True if it is considered empty
   */
  bool isEmpty(const std::string &value) const { return value.empty(); }

  /**
   * Test if a value is an alias of an alowed value.
   * @param value :: Value to test.
   * @return :: True if it's an alias.
   */
  template <typename T> bool isAlias(const T &value) const {
    std::string strValue = boost::lexical_cast<std::string>(value);
    return m_aliases.find(strValue) != m_aliases.end();
  }

  /**
   * Test if a value is an alias of an alowed value.
   * @param value :: Value to test.
   * @return :: True if it's an alias.
   */
  bool isAlias(const std::string &value) const { return m_aliases.find(value) != m_aliases.end(); }

  /// The set of valid values
  std::vector<TYPE> m_allowedValues;
  /// The optional aliases for the allowed values.
  std::map<std::string, std::string> m_aliases;

  /// if the validator should allow multiple selection
  bool m_allowMultiSelection;
};

/// ListValidator<std::string> is used heavily
using StringListValidator = ListValidator<std::string>;

} // namespace Kernel
} // namespace Mantid
