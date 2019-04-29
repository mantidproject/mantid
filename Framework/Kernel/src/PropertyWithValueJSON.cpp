// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/PropertyWithValueJSON.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include <json/value.h>
#include <map>

using Json::Value;

namespace Mantid {
namespace Kernel {

namespace {

// A pointer to a member function for doing the Json::Value->C++ type conversion
// Used only for type deduction in the FromJson constructor
template <typename T> using ValueAsTypeMemFn = T (Json::Value::*)() const;

// A non-templated outer struct that can be stored in a container without
// requiring a pointer. The implementation follows the concept-model idiom
// of storing the templated type using type erasure. Creating an object
// of this type uses the type passed to the constructor to infer the
// template parameter type. This template type is then used to call
// the appropriate ToCpp conversion function defined in the header when
// createProperty is called.
struct FromJson {
  template <typename T>
  explicit FromJson(ValueAsTypeMemFn<T> /*unused*/)
      : m_self{std::make_unique<ModelT<T>>()} {}

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
    std::unique_ptr<Property>
    singleValueProperty(const std::string &name,
                        const Json::Value &value) const override final {
      using ToCppT = pwvjdetail::ToCpp<T>;
      return std::make_unique<PropertyWithValue<T>>(name, ToCppT()(value));
    }

    std::unique_ptr<Property>
    arrayValueProperty(const std::string &name,
                       const Json::Value &value) const override final {
      using ToCppVectorT = pwvjdetail::ToCpp<std::vector<T>>;
      return std::make_unique<ArrayProperty<T>>(name, ToCppVectorT()(value));
    }
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
    // Build a map of Json types to FromJson converters of the appropriate type
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
 * @param value The value as Json::Value. For an array is guaranteed to have
 at
 * least 1 element
 * @param createArray If true creates an ArrayProperty
 * @return A pointer to a new Property object
 * @throws std::invalid_argument if the type of the Json::Value is not known
 */
std::unique_ptr<Property> createSingleTypeProperty(const std::string &name,
                                                   const Json::Value &value) {
  const auto isArray = value.isArray();
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
  return std::make_unique<PropertyManagerProperty>(
      name, createPropertyManager(keyValues));
}

} // namespace

/**
 * @brief Create a PropertyManager from a Json Object.
 * @param keyValues A Json objectValue. This is not checked.
 * @return A new PropertyManager
 * @throws std::invalid_argument if the Json::Value can't be interpreted
 */
PropertyManager_sptr createPropertyManager(const Json::Value &keyValues) {
  auto propMgr = boost::make_shared<PropertyManager>();
  auto members = keyValues.getMemberNames();
  for (const auto &key : members) {
    const auto &value = keyValues[key];
    if (value.isObject())
      propMgr->declareProperty(createKeyValueProperty(key, value));
    else
      propMgr->declareProperty(createSingleTypeProperty(key, value));
  }
  return propMgr;
}

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
