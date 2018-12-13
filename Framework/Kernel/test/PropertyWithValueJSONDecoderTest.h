// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROPERTYWITHVALUEJSONDECODERTEST_H
#define PROPERTYWITHVALUEJSONDECODERTEST_H

#include <cxxtest/TestSuite.h>
#include <jsoncpp/json/value.h>

#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValueJSONDecoder.h"

class PropertyWithValueJSONDecoderTest : public CxxTest::TestSuite {
public:
  static PropertyWithValueJSONDecoderTest *createSuite() {
    return new PropertyWithValueJSONDecoderTest;
  }

  static void destroySuite(PropertyWithValueJSONDecoderTest *suite) {
    return delete suite;
  }

  void testDecodeSingleJSONIntAsProperty() {
    doSingleValueObjectDecodeTest("IntProperty", 10);
  }

  void testDecodeSingleJSONDoubleAsProperty() {
    doSingleValueObjectDecodeTest("DoubleProperty", 10.5);
  }

  void testDecodeSingleJSONStringAsProperty() {
    doSingleValueObjectDecodeTest("StringProperty", std::string("My value"));
  }

  void testDecodeSingleJSONBoolAsProperty() {
    doSingleValueObjectDecodeTest("BoolProperty", false);
  }

  void testDecodeArrayValueAsArrayProperty() {
    const auto propName{"ArrayProperty"};
    const std::vector<double> propValue{1.0, 2.0, 3.0};
    Json::Value arrayItem(Json::arrayValue);
    for (const auto &elem : propValue)
      arrayItem.append(elem);

    using Mantid::Kernel::ArrayProperty;
    auto typedProperty =
        doBasicDecodeTest<ArrayProperty<double>>(propName, arrayItem);
  }

  void testDecodeSingleObjectValuePropertyManagerProperty() {
    const auto propName{"SinglePropertyManager"}, intKey("k1"), realKey("k2");
    const int intValue(1);
    const double realValue(5.3);
    Json::Value dict(Json::objectValue);
    dict[intKey] = intValue;
    dict[realKey] = realValue;

    using Mantid::Kernel::PropertyManagerProperty;
    auto typedProperty =
        doBasicDecodeTest<PropertyManagerProperty>(propName, dict);

    using Mantid::Kernel::PropertyManager_sptr;
    PropertyManager_sptr propMgr{(*typedProperty)()};
    TS_ASSERT_EQUALS(intValue, static_cast<int>(propMgr->getProperty(intKey)));
    TS_ASSERT_EQUALS(realValue,
                     static_cast<double>(propMgr->getProperty(realKey)));
  }

  void testDecodeNestedObjectValuesAsNestedPropertyManagerProperty() {
    const auto propName{"NestedPropertyManager"}, outerIntKey("k1"),
        innerIntKey("ik1"), outerRealKey("k2"), innerRealKey("ik2"),
        outerDictKey("ik3");
    const int outerIntValue(1), innerIntValue(10);
    const double outerRealValue(5.3), innerRealValue(15.3);

    Json::Value innerDict(Json::objectValue);
    innerDict[innerIntKey] = innerIntValue;
    innerDict[innerRealKey] = innerRealValue;
    Json::Value outerDict(Json::objectValue);
    outerDict[outerIntKey] = outerIntValue;
    outerDict[outerRealKey] = outerRealValue;
    outerDict[outerDictKey] = innerDict;

    using Mantid::Kernel::PropertyManagerProperty;
    auto typedProperty =
        doBasicDecodeTest<PropertyManagerProperty>(propName, outerDict);

    using Mantid::Kernel::PropertyManager_sptr;
    PropertyManager_sptr propMgr{(*typedProperty)()};
    TS_ASSERT_EQUALS(outerIntValue,
                     static_cast<int>(propMgr->getProperty(outerIntKey)));
    TS_ASSERT_EQUALS(outerRealValue,
                     static_cast<double>(propMgr->getProperty(outerRealKey)));
  }

  // ----------------------- Failure tests -----------------------

  void testDecodeThrowsWithEmptyValue() {
    using Mantid::Kernel::decodeAsProperty;
    Json::Value root;
    TSM_ASSERT_THROWS("Expected decode to throw for empty value",
                      decodeAsProperty("NullValue", root),
                      std::invalid_argument);
  }

  void testDecodeEmptyArrayValueThrows() {
    Json::Value root;
    root["EmptyArray"] = Json::Value(Json::arrayValue);

    using Mantid::Kernel::decodeAsProperty;
    TSM_ASSERT_THROWS("Expected an empty json array to throw",
                      decodeAsProperty("EmptyArray", root),
                      std::invalid_argument);
  }

  void testDecodeHeterogenousArrayValueThrows() {
    Json::Value mixedArray(Json::arrayValue);
    mixedArray.append(1);
    mixedArray.append(true);
    mixedArray.append("hello");

    using Mantid::Kernel::decodeAsProperty;
    TSM_ASSERT_THROWS("Expected an empty json array to throw",
                      decodeAsProperty("Mixed", mixedArray),
                      std::invalid_argument);
  }

private:
  template <typename ValueType>
  void doSingleValueObjectDecodeTest(const std::string &propName,
                                     const ValueType &propValue) {
    Json::Value root(propValue);

    using Mantid::Kernel::PropertyWithValue;
    auto typedProperty =
        doBasicDecodeTest<PropertyWithValue<ValueType>>(propName, root);
    TS_ASSERT_EQUALS(propValue, (*typedProperty)());
  }

  template <typename PropertyType>
  std::unique_ptr<PropertyType>
  doBasicDecodeTest(const std::string &propName, const Json::Value &jsonValue) {
    using Mantid::Kernel::decodeAsProperty;
    auto property = decodeAsProperty(propName, jsonValue);
    TSM_ASSERT("Decode failed to create a Property. ", property);
    auto typedProperty = std::unique_ptr<PropertyType>{
        dynamic_cast<PropertyType *>(property.release())};
    TSM_ASSERT("Property has unexpected type ", typedProperty);
    TS_ASSERT_EQUALS(propName, typedProperty->name());

    return typedProperty;
  }
};

#endif // PROPERTYWITHVALUEJSONDECODERTEST_H
