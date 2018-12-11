// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValueJSONDecoder.h"
#include "MantidKernel/PropertyWithValue.h"
#include <json/json.h>
#include <unordered_map>

using Json::StreamWriterBuilder;
using Json::Value;
using Json::writeString;

namespace Mantid {
namespace Kernel {

namespace {
// Define a lookup mapping Json::ValueTypes to functions able to
// create a PropertyWithValue object from that type
using FromJsonFn = std::function<std::unique_ptr<Property>(
    const std::string &, const Json::Value &)>;
using FromJsonConverters = std::map<Json::ValueType, FromJsonFn>;

// A pointer to a member function for doing the Json::Value->C++ type conversion
template <typename T> using ValueAsTypeMemFn = T (Json::Value::*)() const;

// A generic function accepting a pointer to a Json::Value::as* function, e.g.
// asInt, plus the name and value of the property
template <typename ValueType>
std::unique_ptr<Property> fromJson(ValueAsTypeMemFn<ValueType> asFn,
                                   const std::string &name,
                                   const Json::Value &value) {
  return std::make_unique<PropertyWithValue<ValueType>>(name,
                                                        (value.*(asFn))());
}

// Returns (and creates on first call) the map of Json::ValueType to
// FromJsonFn for converting a JsonValue to a Property object
const FromJsonConverters &converters() {
  static FromJsonConverters converters;
  if (converters.empty()) {
    using namespace std::placeholders;
    // Build a map of Json types to fromJson functions of the appropriate type
    converters.insert(std::make_pair(
        Json::booleanValue,
        std::bind(fromJson<bool>, &Json::Value::asBool, _1, _2)));
    converters.insert(std::make_pair(
        Json::intValue, std::bind(fromJson<int>, &Json::Value::asInt, _1, _2)));
    converters.insert(std::make_pair(
        Json::realValue,
        std::bind(fromJson<double>, &Json::Value::asDouble, _1, _2)));
    converters.insert(std::make_pair(
        Json::stringValue,
        std::bind(fromJson<std::string>, &Json::Value::asString, _1, _2)));
  }
  return converters;
}

/**
 * @brief Create a PropertyWithValue object from the given Json::Value
 * @param name The name of the new property
 * @param value The value as Json::Value
 * @return A pointer to a new Property object
 * @throws std::invalid_argument if the type of the Json::Value is not known
 */
std::unique_ptr<Property> createProperty(const std::string &name,
                                         const Json::Value &value) {
  auto conversionFnIter = converters().find(value.type());
  if (conversionFnIter == converters().end()) {
    throw std::invalid_argument(
        "Cannot create property with name " + name +
        ". Unable to find converter for Json::ValueType to C++ type");
  }
  return conversionFnIter->second(name, value);
}

} // namespace

/**
 * @brief decode
 * @param value A value as a Json serialized quantity
 * @return
 */
std::unique_ptr<Property> decode(const Json::Value &value) {
  if (value.size() != 1) {
    StreamWriterBuilder wbuilder;
    throw std::invalid_argument(
        "Expected Json::Value with a single member. Found " +
        writeString(wbuilder, value));
  }
  Value::Members members(value.getMemberNames());
  assert(members.size() == 1);

  return createProperty(members[0], value[members.front()]);
}

} // namespace Kernel
} // namespace Mantid
