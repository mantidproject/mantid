// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyWithValue.h"
#include <string>
#include <vector>

namespace Mantid {
namespace Kernel {

// Define the EnumStringProperty with simpler syntax
template <typename T, const std::vector<std::string> *Names>
using SimpleEnumStringPropertyBase = PropertyWithValue<EnumeratedString<T, Names>>;

template <typename T, const std::vector<std::string> *Names>
class DLLExport EnumStringProperty : public SimpleEnumStringPropertyBase<T, Names> {

public:
  // constructors
  EnumStringProperty();
  std::string value();
  std::string setValue(const std::string &value);
};

// Example implementation of the constructor (usually in a source file)
template <typename T, const std::vector<std::string> *Names> EnumStringProperty<T, Names>::EnumStringProperty() {
  // Constructor implementation...
}

template <typename T, const std::vector<std::string> *Names> std::string EnumStringProperty<T, Names>::value() {
  // Return value implementation...
  return {};
}

template <typename T, const std::vector<std::string> *Names>
std::string EnumStringProperty<T, Names>::setValue(const std::string &value) {
  // Set value implementation...
  return {};
}
/** EnumStringProperty : TODO: DESCRIPTION
 */
// template <template <typename, typename> class E, typename T, const std::vector<std::string> *>
// class DLLExport EnumStringProperty : public PropertyWithValue<E<T, const std::vector<std::string> *>> {
//
// public:
//  // constructors
//  EnumStringProperty();
//  std::string value();
//  std::string setValue(const std::string &value);
//};
//
// template <typename T, const std::vector<std::string> *Names>
// using EnumeratedStringProperty = EnumStringProperty<EnumeratedString, T, Names>;

// template <template <typename, typename> class E, typename T, const std::vector<std::string> *Names>
// using EnumeratedStringProperty = EnumStringProperty<E, T, Names>;

// template < class E, typename T, const std::vector<std::string> *>
//  using EnumeratedStringProperty = EnumStringProperty<EnumeratedString, T, const std::vector<std::string> *names>;
//
// template <typename T>
// class DLLExport EnumStringProperty : public PropertyWithValue<EnumeratedString<T>> {
// public:
//  //constructors
//  EnumStringProperty();
//
//  std::string value() const; // Make this method const
//  std::string setValue(const std::string &value); // Ensure the return type fits your design
//};
// template <class E, const std::vector<std::string>& names>
// class DLLExport EnumStringProperty : public PropertyWithValue<EnumeratedString<E, names>> {
// public:
//  // Constructor definitions
//  EnumStringProperty() : PropertyWithValue<EnumeratedString<E, names>>() {}
//
//  // Member function definitions
//  std::string value();
//  std::string setValue(const std::string &value);
//};
//
//// Definitions of the member functions
// template <class E, const std::vector<std::string>& names>
// std::string EnumStringProperty<E, names>::value() {
//  // Define this method
//}
//
// template <class E, const std::vector<std::string>& names>
// std::string EnumStringProperty<E, names>::setValue(const std::string &value) {
//  // Define this method
//}
} // namespace Kernel
} // namespace Mantid
