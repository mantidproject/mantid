// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValueJSONDecoder.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include <json/json.h>
#include <map>

#include <iostream>

using Json::StreamWriterBuilder;
using Json::Value;
using Json::writeString;

namespace Mantid {
namespace Kernel {

namespace {

// A pointer to a member function for doing the Json::Value->C++ type conversion
template <typename T> using ValueAsTypeMemFn = T (Json::Value::*)() const;

// A struct mapping a Json::Value to a property. Follows the
//  Runtime concept idiom to store a templated type internally
struct FromJson {
  template <typename T>
  FromJson(ValueAsTypeMemFn<T> asFn) noexcept
      : m_self{std::make_unique<ModelT<T>>(std::move(asFn))} {}

  std::unique_ptr<Property> createProperty(const std::string &name,
                                           const Json::Value &value,
                                           bool createArray) const {
    if (createArray)
      return m_self->arrayValueProperty(name, value);
    else
      return m_self->singleValueProperty(name, value);
  }

private:
  struct ConceptT {
    virtual ~ConceptT() = default;
    virtual std::unique_ptr<Property>
    singleValueProperty(const std::string &name,
                        const Json::Value &value) const = 0;
    virtual std::unique_ptr<Property>
    arrayValueProperty(const std::string &name,
                       const Json::Value &value) const = 0;
  };

  template <typename T> struct ModelT : ConceptT {
    ModelT(ValueAsTypeMemFn<T> asFn) noexcept : m_asFn{std::move(asFn)} {}

    std::unique_ptr<Property>
    singleValueProperty(const std::string &name,
                        const Json::Value &value) const override {
      return std::make_unique<PropertyWithValue<T>>(name, (value.*m_asFn)());
    }

    std::unique_ptr<Property>
    arrayValueProperty(const std::string &name,
                       const Json::Value &value) const override {
      std::vector<T> arrayValue;
      arrayValue.reserve(value.size());
      for (const auto &elem : value) {
        try {
          arrayValue.emplace_back((elem.*m_asFn)());
        } catch (Json::Exception &exc) {
          throw std::invalid_argument(
              "Mixed-type JSON array values not supported:" +
              std::string(exc.what()));
        }
      }
      return std::make_unique<ArrayProperty<T>>(name, std::move(arrayValue));
    }

  private:
    ValueAsTypeMemFn<T> m_asFn;
  };
  std::unique_ptr<ConceptT> m_self;
};

// Define a lookup mapping Json::ValueTypes to a FromJson able to
// create a PropertyWithValue object from that type
using FromJsonConverters = std::map<Json::ValueType, FromJson>;

// Returns (and creates on first call) the map of Json::ValueType to
// FromJson for converting a JsonValue to a Property object
const FromJsonConverters &converters() {
  static FromJsonConverters converters;
  if (converters.empty()) {
    using namespace std::placeholders;
    // Build a map of Json types to fromJson functions of the appropriate type
    converters.insert(
        std::make_pair(Json::booleanValue, FromJson(&Json::Value::asBool)));
    converters.insert(
        std::make_pair(Json::intValue, FromJson(&Json::Value::asInt)));
    converters.insert(
        std::make_pair(Json::realValue, FromJson(&Json::Value::asDouble)));
    converters.insert(
        std::make_pair(Json::stringValue, FromJson(&Json::Value::asString)));
  }
  return converters;
}

/**
 * @brief Create a PropertyWithValue object from the given Json::Value
 * @param name The name of the new property
 * @param value The value as Json::Value. For an array is guaranteed to have at
 * least 1 element
 * @param createArray If true creates an ArrayProperty
 * @return A pointer to a new Property object
 * @throws std::invalid_argument if the type of the Json::Value is not known
 */
std::unique_ptr<Property> createSingleTypeProperty(const std::string &name,
                                                   const Json::Value &value) {
  const auto isArray{value.isArray()};
  FromJsonConverters::const_iterator conversionFnIter;
  // For an array use the first element as the type checker and the rest must
  // be convertible
  if (isArray)
    conversionFnIter = converters().find(value[0].type());
  else
    conversionFnIter = converters().find(value.type());

  if (conversionFnIter == converters().end()) {
    throw std::invalid_argument("Cannot create property with name " + name +
                                ". Unable to find converter "
                                "for Json::ValueType to C++ "
                                "type");
  }
  return conversionFnIter->second.createProperty(name, value, value.isArray());
}

/**
 * @brief Create a PropertyManagerProperty from the given Json:objectValue
 * @param name The name of the object
 * @param keyValues A Json::objectValue containing key-value pairs
 * @return A new Property object
 */
std::unique_ptr<Property> createKeyValueProperty(const std::string &name,
                                                 const Json::Value &keyValues) {
  auto propMgr{boost::make_shared<PropertyManager>()};
  auto propMgrProp{std::make_unique<PropertyManagerProperty>(name, propMgr)};
  auto members{keyValues.getMemberNames()};
  for (const auto &key : members) {
    const auto &value = keyValues[key];
    if (value.isObject())
      propMgr->declareProperty(createKeyValueProperty(key, value));
    else
      propMgr->declareProperty(createSingleTypeProperty(key, value));
  }
  return propMgrProp;
}

} // namespace

/**
 * @param name The name of the new property
 * @param value A value as a Json serialized quantity
 * @return A pointer to a new Property if the underlying value can
 * be converted to a known C++ type
 * @throws std::invalid_argument If the value cannot be transformed to
 * a Property object
 */
std::unique_ptr<Property> decodeAsProperty(const std::string &name,
                                           const Json::Value &value) {
  if (value.isNull()) {
    throw std::invalid_argument("decodeAsProperty(): Found null Json value.");
  }

  if (!value.isObject()) {
    return createSingleTypeProperty(name, value);
  } else {
    return createKeyValueProperty(name, value);
  }
}

} // namespace Kernel
} // namespace Mantid
