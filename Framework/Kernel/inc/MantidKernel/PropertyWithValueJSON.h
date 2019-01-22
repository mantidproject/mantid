// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROPERTYWITHVALUEJSON_H
#define PROPERTYWITHVALUEJSON_H

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Matrix.h"

#include <boost/shared_ptr.hpp>
#include <json/value.h>

#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {
template <typename T> class Matrix;
class Property;

/// Attempt to create a Property from a string name and Json value object
MANTID_KERNEL_DLL std::unique_ptr<Property>
decodeAsProperty(const std::string &name, const Json::Value &value);

namespace pwvj_detail {
// There is a known isssue with jsoncpp and ambiguous overloads when called
// with long/unsigned long. We disambigate this by defining a generic
// template that passes through its type and provide specializations
// for the integer types to map these to appropriate Json integer types
template <typename ValueType> struct JsonType { using Type = ValueType; };
template <> struct JsonType<int> { using Type = Json::Int; };
template <> struct JsonType<long> { using Type = Json::Int64; };
template <> struct JsonType<unsigned int> { using Type = Json::UInt; };
template <> struct JsonType<unsigned long> { using Type = Json::UInt64; };

} // namespace pwvj_detail

/**
 * Encode a single value as a Json::Value
 * @param value The C++ value to encode
 * @return A new Json::Value
 */
template <typename ValueType> Json::Value encodeAsJson(const ValueType &value) {
  using JsonValueType = typename pwvj_detail::JsonType<ValueType>::Type;
  return Json::Value(static_cast<JsonValueType>(value));
}

/**
 * Encode a std::vector value as a Json::Value arrayValue type
 * @param vectorValue The C++ value to encode
 * @return A new Json::Value
 */
template <typename ValueType>
Json::Value encodeAsJson(const std::vector<ValueType> &vectorValue) {
  Json::Value jsonArray(Json::arrayValue);
  for (const auto &element : vectorValue) {
    jsonArray.append(encodeAsJson(element));
  }
  return jsonArray;
}

/**
 * Encode a shared_ptr by dereferencing a it and encoding its value
 * @return A new Json::Value
 * @throws std::runtime_error for all inputs
 */
template <typename ValueType>
Json::Value encodeAsJson(const boost::shared_ptr<ValueType> &) {
  throw std::runtime_error("Unable to encode shared_ptr<T> as Json::Value.");
}

/**
 * Encode a Matrix as a Json::Value. Currently throws as it is not required
 * @return A new Json::Value
 * @throws Exception::NotImplementedError
 */
template <typename ValueType>
Json::Value encodeAsJson(const Kernel::Matrix<ValueType> &) {
  throw Exception::NotImplementedError(
      "encodeAsJson not implemented for matrix-value type");
}

} // namespace Kernel
} // namespace Mantid

#endif // PROPERTYWITHVALUEJSONDECODER_H
