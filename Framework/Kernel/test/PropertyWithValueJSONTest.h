// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValue.hxx"
#include "MantidKernel/PropertyWithValueJSON.h"
#include <cxxtest/TestSuite.h>
#include <json/value.h>

template <typename T> using AsTypeMemFn = T (Json::Value::*)() const;

class PropertyWithValueJSONTest : public CxxTest::TestSuite {
public:
  static PropertyWithValueJSONTest *createSuite() { return new PropertyWithValueJSONTest; }

  static void destroySuite(PropertyWithValueJSONTest *suite) { return delete suite; }

  void testDecodeAsPropertySingleJSONIntAsProperty() { doSingleValueObjectDecodeTest("IntProperty", 10); }

  void testDecodeAsPropertySingleJSONDoubleAsProperty() { doSingleValueObjectDecodeTest("DoubleProperty", 10.5); }

  void testDecodeAsPropertySingleJSONStringAsProperty() {
    doSingleValueObjectDecodeTest("StringProperty", std::string("My value"));
  }

  void testDecodeAsPropertySingleJSONBoolAsProperty() { doSingleValueObjectDecodeTest("BoolProperty", false); }

  void testDecodeAsPropertyArrayValueAsArrayProperty() {
    const std::string propName{"ArrayProperty"};
    const std::vector<double> propValue{1.0, 2.0, 3.0};
    Json::Value arrayItem(Json::arrayValue);
    for (const auto &elem : propValue)
      arrayItem.append(elem);

    using Mantid::Kernel::ArrayProperty;
    auto typedProperty = doBasicDecodeTest<ArrayProperty<double>>(propName, arrayItem);
  }

  void testCreatePropertyManagerFromSingleJsonObject() {
    const std::string intKey{"k1"}, realKey{"k2"};
    const int intValue(1);
    const double realValue(5.3);
    Json::Value dict(Json::objectValue);
    dict[intKey] = intValue;
    dict[realKey] = realValue;

    using Mantid::Kernel::createPropertyManager;
    auto propMgr = createPropertyManager(dict);

    TS_ASSERT_EQUALS(intValue, static_cast<int>(propMgr->getProperty(intKey)));
    TS_ASSERT_EQUALS(realValue, static_cast<double>(propMgr->getProperty(realKey)));
  }

  void testCreatePropertyManagerFromNestedJsonObject() {
    const std::string outerIntKey{"k1"}, innerIntKey{"ik1"}, outerRealKey{"k2"}, innerRealKey{"ik2"},
        outerDictKey{"ik3"};
    const int outerIntValue(1), innerIntValue(10);
    const double outerRealValue(5.3), innerRealValue(15.3);

    Json::Value innerDict(Json::objectValue);
    innerDict[innerIntKey] = innerIntValue;
    innerDict[innerRealKey] = innerRealValue;
    Json::Value outerDict(Json::objectValue);
    outerDict[outerIntKey] = outerIntValue;
    outerDict[outerRealKey] = outerRealValue;
    outerDict[outerDictKey] = innerDict;

    using Mantid::Kernel::createPropertyManager;
    auto outerPropMgr = createPropertyManager(outerDict);

    TS_ASSERT_EQUALS(outerIntValue, static_cast<int>(outerPropMgr->getProperty(outerIntKey)));
    TS_ASSERT_EQUALS(outerRealValue, static_cast<double>(outerPropMgr->getProperty(outerRealKey)));
    using Mantid::Kernel::PropertyManager_sptr;
    PropertyManager_sptr innerPropMgr = outerPropMgr->getProperty(outerDictKey);
    TS_ASSERT_EQUALS(innerIntValue, static_cast<int>(innerPropMgr->getProperty(innerIntKey)));
    TS_ASSERT_EQUALS(innerRealValue, static_cast<double>(innerPropMgr->getProperty(innerRealKey)));
  }

  void testEncodeIntPropertyAsJsonInt() { doSingleValueEncodeTest<int>(10, Json::intValue, &Json::Value::asInt); }

  void testEncodeDoublePropertyAsJsonReal() {
    doSingleValueEncodeTest<double>(10, Json::realValue, &Json::Value::asDouble);
  }

  void testEncodeBoolPropertyAsJsonBool() {
    doSingleValueEncodeTest<bool>(false, Json::booleanValue, &Json::Value::asBool);
  }

  void testEncodeStringPropertyAsJsonString() {
    doSingleValueEncodeTest<std::string>("test string", Json::stringValue, &Json::Value::asString);
  }

  void testEncodeArrayPropertyAsJsonArray() {
    std::vector<double> values{1, 2, 3};

    auto jsonVal = doBasicEncodeTest<decltype(values)>(values, Json::arrayValue);

    TS_ASSERT_EQUALS(values.size(), jsonVal.size());
    for (auto i = 0u; i < values.size(); ++i) {
      TS_ASSERT_EQUALS(jsonVal[i], values[i]);
    }
  }

  // ----------------------- Failure tests -----------------------

  void testDecodeAsPropertyThrowsWithEmptyValue() {
    using Mantid::Kernel::decodeAsProperty;
    Json::Value root;
    TSM_ASSERT_THROWS("Expected decode to throw for empty value", decodeAsProperty("NullValue", root),
                      const std::invalid_argument &);
  }

  void testDecodeAsPropertyEmptyArrayValueThrows() {
    Json::Value root;
    root["EmptyArray"] = Json::Value(Json::arrayValue);

    using Mantid::Kernel::decodeAsProperty;
    TSM_ASSERT_THROWS("Expected an empty json array to throw", decodeAsProperty("EmptyArray", root),
                      const std::invalid_argument &);
  }

  void testDecodeAsPropertyHeterogenousArrayValueThrows() {
    Json::Value mixedArray(Json::arrayValue);
    mixedArray.append(1);
    mixedArray.append(true);
    mixedArray.append("hello");

    using Mantid::Kernel::decodeAsProperty;
    TSM_ASSERT_THROWS("Expected an empty json array to throw", decodeAsProperty("Mixed", mixedArray),
                      const std::invalid_argument &);
  }

private:
  template <typename ValueType>
  void doSingleValueObjectDecodeTest(const std::string &propName, const ValueType &propValue) {
    Json::Value root(propValue);

    using Mantid::Kernel::PropertyWithValue;
    auto typedProperty = doBasicDecodeTest<PropertyWithValue<ValueType>>(propName, root);
    TS_ASSERT_EQUALS(propValue, (*typedProperty)());
  }

  template <typename PropertyType>
  std::unique_ptr<PropertyType> doBasicDecodeTest(const std::string &propName, const Json::Value &jsonValue) {
    using Mantid::Kernel::decodeAsProperty;
    auto property = decodeAsProperty(propName, jsonValue);
    TSM_ASSERT("Decode failed to create a Property. ", property);
    auto typedProperty = std::unique_ptr<PropertyType>{dynamic_cast<PropertyType *>(property.release())};
    TSM_ASSERT("Property has unexpected type ", typedProperty);
    TS_ASSERT_EQUALS(propName, typedProperty->name());

    return typedProperty;
  }

  template <typename ValueType>
  void doSingleValueEncodeTest(const ValueType &propValue, const Json::ValueType &expectedType,
                               const AsTypeMemFn<ValueType> &asFn) {
    auto jsonVal = doBasicEncodeTest<ValueType>(propValue, expectedType);
    TS_ASSERT_EQUALS(propValue, (jsonVal.*asFn)());
  }

  template <typename ValueType>
  Json::Value doBasicEncodeTest(const ValueType &propValue, const Json::ValueType &expectedType) {
    using Mantid::Kernel::encodeAsJson;
    auto jsonVal = encodeAsJson(propValue);

    TS_ASSERT_EQUALS(expectedType, jsonVal.type());
    return jsonVal;
  }
};
