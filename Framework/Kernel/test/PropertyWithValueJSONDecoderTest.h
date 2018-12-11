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

#include "MantidKernel/PropertyWithValue.h"
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
    doSimpleObjectDecodeTest("IntProperty", 10);
  }

  void testDecodeSingleJSONDoubleAsProperty() {
    doSimpleObjectDecodeTest("DoubleProperty", 10.5);
  }

  void testDecodeSingleJSONStringAsProperty() {
    doSimpleObjectDecodeTest("StringProperty", std::string("My value"));
  }

  void testDecodeSingleJSONBoolAsProperty() {
    doSimpleObjectDecodeTest("BoolProperty", false);
  }

  // ----------------------- Failure tests -----------------------

  void testDecodeThrowsWithEmptyValue() {
    using Mantid::Kernel::decode;
    Json::Value root;
    TSM_ASSERT_THROWS("Expected decode to throw for empty value", decode(root),
                      std::invalid_argument);
  }

  void testDecodeThrowsWithGreaterThanOneMember() {
    using Mantid::Kernel::decode;
    Json::Value root;
    root["one"] = 1;
    root["two"] = 2;

    TSM_ASSERT_THROWS("Expected decode to throw with more than 1 member",
                      decode(root), std::invalid_argument);
  }

  void testDecodeThrowsWithNonObjectValue() {
    using Mantid::Kernel::decode;
    TSM_ASSERT_THROWS("Expected decode to throw with non-object type",
                      decode(Json::Value(10)), std::invalid_argument);
  }

private:
  template <typename ValueType>
  void doSimpleObjectDecodeTest(const std::string &propName,
                                const ValueType &propValue) {
    Json::Value root;
    root[propName] = propValue;

    using Mantid::Kernel::decode;
    auto property = decode(root);
    TSM_ASSERT("Decode failed to create a Property. ", property);
    using Mantid::Kernel::PropertyWithValue;
    auto typedProperty =
        dynamic_cast<PropertyWithValue<ValueType> *>(property.get());
    TSM_ASSERT("Property has unexpected type ", typedProperty);
    TS_ASSERT_EQUALS(propName, typedProperty->name());
    TS_ASSERT_EQUALS(propValue, (*typedProperty)());
  }
};

#endif // PROPERTYWITHVALUEJSONDECODERTEST_H
