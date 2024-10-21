// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"
#include "PropertyManager_fwd.h"

#include <json/value.h>
#include <memory>

#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
template <typename T> class Matrix;
class Property;

namespace pwvjdetail {

/// General type to convert a Json::Value to a set C++ type
template <typename T> struct ToCpp {
  T operator()(const Json::Value &) {
    throw Exception::NotImplementedError("Unknown conversion from Json to C++ type");
  }
};

/// Specialization of ToCpp for int
template <> struct ToCpp<int> {
  int operator()(const Json::Value &value) { return value.asInt(); }
};
/// Specialization of ToCpp for long long
template <> struct ToCpp<long long> {
  long long operator()(const Json::Value &value) { return value.asInt64(); }
};
/// Specialization of ToCpp for long
template <> struct ToCpp<long> {
  long operator()(const Json::Value &value) { return value.asInt(); }
};
/// Specialization of ToCpp for unsigned int
template <> struct ToCpp<unsigned int> {
  unsigned int operator()(const Json::Value &value) { return value.asUInt(); }
};
/// Specialization of ToCpp for unsigned long long int
template <> struct ToCpp<unsigned long long int> {
  unsigned long long int operator()(const Json::Value &value) { return value.asUInt64(); }
};
/// Specialization of ToCpp for bool
template <> struct ToCpp<bool> {
  bool operator()(const Json::Value &value) { return value.asBool(); }
};
/// Specialization of ToCpp for float
template <> struct ToCpp<float> {
  float operator()(const Json::Value &value) { return value.asFloat(); }
};
/// Specialization of ToCpp for double
template <> struct ToCpp<double> {
  double operator()(const Json::Value &value) { return value.asDouble(); }
};
/// Specialization of ToCpp for std::string
template <> struct ToCpp<std::string> {
  std::string operator()(const Json::Value &value) { return value.asString(); }
};

/// Specialization of ToCpp for std::vector
template <typename T> struct ToCpp<std::vector<T>> {
  std::vector<T> operator()(const Json::Value &value) {
    std::vector<T> arrayValues;
    arrayValues.reserve(value.size());
    auto toCpp = ToCpp<T>();
    for (const auto &elem : value) {
      try {
        arrayValues.emplace_back(toCpp(elem));
      } catch (Json::Exception &exc) {
        throw std::invalid_argument("Mixed-type JSON array values not supported:" + std::string(exc.what()));
      }
    }
    return arrayValues;
  }
};

} // namespace pwvjdetail

/// Attempt to decode the given Json::Value as the given Type
template <typename Type> Type decode(const Json::Value &value) { return pwvjdetail::ToCpp<Type>()(value); }

/// Attempt to create a PropertyManager from the Json::Value
MANTID_KERNEL_DLL
PropertyManager_sptr createPropertyManager(const Json::Value &keyValues);

/// Attempt to create a Property of the most appropriate type
/// from a string name and Json value object
MANTID_KERNEL_DLL std::unique_ptr<Property> decodeAsProperty(const std::string &name, const Json::Value &value);

namespace pwvjdetail {
// There is a known isssue with jsoncpp and ambiguous overloads when called
// with long/unsigned long. We disambigate this by defining a generic
// template that passes through its type and provide specializations
// for the integer types to map these to appropriate Json integer types
template <typename ValueType> struct JsonType {
  using Type = ValueType;
};
template <> struct JsonType<int> {
  using Type = Json::Int;
};
template <> struct JsonType<long> {
  using Type = Json::Int64;
};
template <> struct JsonType<long long> {
  using Type = Json::Int64;
};
template <> struct JsonType<unsigned int> {
  using Type = Json::UInt;
};
template <> struct JsonType<unsigned long> {
  using Type = Json::UInt64;
};
template <> struct JsonType<unsigned long long> {
  using Type = Json::UInt64;
};
} // namespace pwvjdetail

/**
 * Encode a single value as a Json::Value
 * @param value The C++ value to encode
 * @return A new Json::Value
 */
template <typename ValueType> Json::Value encodeAsJson(const ValueType &value) {
  using JsonValueType = typename pwvjdetail::JsonType<ValueType>::Type;
  return Json::Value(static_cast<JsonValueType>(value));
}

/**
 * Encode a std::vector value as a Json::Value arrayValue type
 * @param vectorValue The C++ value to encode
 * @return A new Json::Value
 */
template <typename ValueType> Json::Value encodeAsJson(const std::vector<ValueType> &vectorValue) {
  Json::Value jsonArray(Json::arrayValue);
  for (const auto &element : vectorValue) {
    jsonArray.append(encodeAsJson(element));
  }
  return jsonArray;
}

/**
 * Specialization to encode a std::vector<bool> value as a Json::Value arrayValue type. Needs to
 * deal with the fact that the return value from an iterator is a temporary object
 * @param vectorValue The C++ value to encode
 * @return A new Json::Value
 */
template <> inline Json::Value encodeAsJson(const std::vector<bool> &vectorValue) {
  Json::Value jsonArray(Json::arrayValue);
  for (const auto element : vectorValue) {
    jsonArray.append(encodeAsJson(element));
  }
  return jsonArray;
}

/**
 * Encode a shared_ptr by dereferencing a it and encoding its value
 * @return A new Json::Value
 * @throws std::runtime_error for all inputs
 */
template <typename ValueType> Json::Value encodeAsJson(const std::shared_ptr<ValueType> &) {
  throw std::runtime_error("Unable to encode shared_ptr<T> as Json::Value.");
}

/**
 * Encode a Matrix as a Json::Value. Currently throws as it is not required
 * @return A new Json::Value
 * @throws Exception::NotImplementedError
 */
template <typename ValueType> Json::Value encodeAsJson(const Kernel::Matrix<ValueType> &) {
  throw Exception::NotImplementedError("encodeAsJson not implemented for matrix-value type");
}

} // namespace Kernel
} // namespace Mantid
