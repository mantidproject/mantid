// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/IValidator.h"
#include <typeinfo>

namespace Mantid {
namespace Kernel {
/**
TypedValidator provides an layer on top of IValidator to ensure
that the template TYPE is extract from the boost::any instance
and passed down to the concrete validator instance. Most validators
will probably want to inherit from this rather than IValidator
directly.

A specialised type exists for std::shared_ptr types
 */
template <typename HeldType> class DLLExport TypedValidator : public IValidator {
protected:
  /// Override this function to check the validity of the type
  virtual std::string checkValidity(const HeldType &) const = 0;

private:
  /**
   * Attempts to extract the TYPE from the boost any object and calls
   * the typed checkValidity member
   *  @returns An error message to display to users or an empty string on no
   * error
   */
  std::string check(const boost::any &value) const override {
    try {
      const HeldType *dataPtr = boost::any_cast<const HeldType *>(value);
      return checkValidity(*dataPtr);
    } catch (boost::bad_any_cast &) {
      return "Value was not of expected type.";
    }
  }
};

/**
 * Specialization for std::shared_ptr<T> types.
 * boost::any_cast cannot convert between types, the type extracted must match
 * the
 * the stored type. In our case IValidator ensures that all items that inherit
 * from
 * DataItem are stored as a DataItem_sptr within the boost::any instance. Once
 * extracted
 * the DataItem can then be downcast to the Validator::TYPE.
 * The advantage of this is that Validator types then don't have to match
 * their types exactly.
 */
template <typename ElementType> class DLLExport TypedValidator<std::shared_ptr<ElementType>> : public IValidator {
  /// Shared ptr type
  using ElementType_sptr = std::shared_ptr<ElementType>;

protected:
  /// Override this function to check the validity of the type
  virtual std::string checkValidity(const ElementType_sptr &) const = 0;

private:
  /**
   * Attempts to extract the TYPE from the boost any object and if
   * the type is a DataItem_sptr then it attempts to downcast the value to
   * the concrete type sepcified by the validator
   * @param value :: The values to verify
   * @returns An error message to display to users or an empty string on no
   * error
   */
  std::string check(const boost::any &value) const override {
    try {
      const ElementType_sptr typedValue = extractValue(value);
      return checkValidity(typedValue);
    } catch (std::invalid_argument &exc) {
      return exc.what();
    }
  }
  /**
   * Extract the value type as the concrete type
   * @param value :: The value
   * @returns The concrete type
   */
  ElementType_sptr extractValue(const boost::any &value) const {
// Despite the name and hash code being identical, operator== is returning false
// in Release mode
// with clang and libc++. The simplest workaround is to compare hash codes.
#ifdef __clang__
    if (value.type().hash_code() == m_dataitemTypeID.hash_code())
#else
    if (value.type() == m_dataitemTypeID)
#endif
    {
      return extractFromDataItem(value);
    } else {
      return extractFromSharedPtr(value);
    }
  }
  /**
   * Extract the DataItem value type by trying to downcast to the concrete type
   * @param value :: The value
   * @returns The concrete type
   */
  ElementType_sptr extractFromDataItem(const boost::any &value) const {
    const DataItem_sptr data = boost::any_cast<DataItem_sptr>(value);
    // First try and push it up to the type of the validator
    ElementType_sptr typedValue = std::dynamic_pointer_cast<ElementType>(data);
    if (!typedValue) {
      throw std::invalid_argument("DataItem \"" + data->getName() + "\" is not of the expected type.");
    }
    return typedValue;
  }
  /**
   * Extract the shared_ptr value type
   * @param value :: The value
   * @returns The concrete type
   */
  ElementType_sptr extractFromSharedPtr(const boost::any &value) const {
    try {
      return boost::any_cast<ElementType_sptr>(value);
    } catch (boost::bad_any_cast &) {
      throw std::invalid_argument("Value was not a shared_ptr type");
    }
  }
  /// Typeid of DataItem_sptr
  static const std::type_info &m_dataitemTypeID;
};

/// @cond
// This switch off an warning because oxygen could not figureout that this is a
// specialized type of the general one.
/** Intialize the DataItem_sptr typeinfo
 */
template <typename T>
const std::type_info &TypedValidator<std::shared_ptr<T>>::m_dataitemTypeID = typeid(std::shared_ptr<DataItem>);
} // namespace Kernel
/// @endcond
} // namespace Mantid
