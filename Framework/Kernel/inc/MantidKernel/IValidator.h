// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DataItem.h"
#include "MantidKernel/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/any.hpp>
#include <memory>
#endif
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace Kernel {
// Forward declaration so that the typedef std::shared_ptr<Validator>
// understand it
class IValidator;

/// A shared_ptr to an IValidator
using IValidator_sptr = std::shared_ptr<IValidator>;

namespace {
/// Helper object to determine if a type is either a pointer/shared_ptr
/// Generic implementation says it is not
template <class T> struct IsPtrType : public std::is_pointer<T> {};
/// Helper object to determine if a type is either a pointer/shared_ptr
/// Specialized implementation for shared_ptr
template <class T> struct IsPtrType<std::shared_ptr<T>> : public std::true_type {};
template <> struct IsPtrType<decltype(nullptr)> : public std::true_type {};
} // namespace

/** IValidator is the basic interface for all validators for properties

    @author Nick Draper, Tessella Support Services plc
    @date 28/11/2007
*/
class MANTID_KERNEL_DLL IValidator {
public:
  /// Constructor
  IValidator() = default;

  /// virtual Destructor
  virtual ~IValidator() = default;

  //------------------------------------------------------------------------------------------------------------
  /** Calls the validator
   *
   *  @param value :: The value to be checked
   *  @returns An error message to display to users or an empty string on no
   *error
   */
  template <typename TYPE> std::string isValid(const TYPE &value) const { return runCheck(value, IsPtrType<TYPE>()); }

  /**
   * Deal with a C-style string by first converting it to a std::string
   * so that boost::any can deal with it, calls isValid(std::string)
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no
   * error
   */
  std::string isValid(const char *value) const { return isValid(std::string(value)); }

  //------------------------------------------------------------------------------------------------------------
  /** The set of allowed values that this validator may have, if a discrete set
   * exists.
   *  Overridden in applicable concrete validators; the base class just returns
   * an empty set.
   *  @return The set of allowed values that this validator may have or an empty
   * set
   */
  virtual std::vector<std::string> allowedValues() const { return std::vector<std::string>(); }

  /** Is Multiple Selection Allowed
   *  @return true if multiple selection is allowed
   */
  virtual bool isMultipleSelectionAllowed() { return false; };

  /**
   * Implement this method for validators which wish to support aliasing for
   * alloeed values.
   * @param alias :: A string representation of an alias.
   * @return :: A string representation of an aliased value. Should throw
   * std::invalid_argument
   *    is the given alias is invalid.
   */
  virtual std::string getValueForAlias(const std::string &alias) const {
    UNUSED_ARG(alias);
    throw std::invalid_argument("Validator does'n support value aliasing.");
  }

  /// Make a copy of the present type of validator
  virtual IValidator_sptr clone() const = 0;

  /** Checks the value based on the validator's rules
   *
   *  @returns An error message to display to users or an empty string on no
   *error
   */
  virtual std::string check(const boost::any &) const = 0;

private:
  /** Calls the validator for a type that is not a pointer type
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no
   * error
   */
  template <typename T> std::string runCheck(const T &value, const std::false_type &) const {
    const T *valuePtr = &value; // Avoid a copy by storing the pointer in the any holder
    return check(boost::any(valuePtr));
  }

  /** Calls the validator for a type that is either a raw pointer or a
   * std::shared pointer
   * @returns An error message to display to users or an empty string on no
   * error
   */
  template <typename T> std::string runCheck(const T &value, const std::true_type &) const {
    return runCheckWithDataItemPtr(value,
                                   std::integral_constant<bool, std::is_convertible<T, DataItem_sptr>::value &&
                                                                    !std::is_same<T, decltype(nullptr)>::value>());
  }
  /** Calls the validator for a pointer type that is NOT convertible to
   * DataItem_sptr
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no
   * error
   */
  template <typename T> std::string runCheckWithDataItemPtr(const T &value, const std::false_type &) const {
    return check(boost::any(value));
  }
  /** Calls the validator for a pointer type that IS convertible to
   * DataItem_sptr
   * @param value :: The value to be checked
   * @returns An error message to display to users or an empty string on no
   * error
   */
  template <typename T> std::string runCheckWithDataItemPtr(const T &value, const std::true_type &) const {
    return check(boost::any(std::static_pointer_cast<DataItem>(value)));
  }
};

} // namespace Kernel
} // namespace Mantid
