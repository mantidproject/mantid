// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Json.h"

#include <cxxtest/TestSuite.h>

class JsonTest : public CxxTest::TestSuite {
public:
  void testJsonToString() {
    std::stringstream initialString("{\"bar\":2,\"baz\":3.1400000000000001,\"foo\":1,\"hello world\":\"HelloWorld\"}");
    Json::Value json;
    initialString >> json;
    TS_ASSERT_EQUALS(initialString.str(), Mantid::Kernel::JsonHelpers::jsonToString(json))
  }

  void testStringToJson() {
    const std::string initialString = "{\"bar\":2,\"baz\":3.1400000000000001,\"foo\":1,\"hello world\":\"HelloWorld\"}";
    const Json::Value json = Mantid::Kernel::JsonHelpers::stringToJson(initialString);
    TS_ASSERT_EQUALS(1, json["foo"].asInt())
    TS_ASSERT_EQUALS(2, json["bar"].asInt())
    TS_ASSERT_DELTA(3.14, json["baz"].asFloat(), 1e-5)
    TS_ASSERT_EQUALS("HelloWorld", json["hello world"].asString())
  }

  void testJsonToStringToJsonToString() {
    std::stringstream initialString("{\"bar\":2,\"baz\":3,\"foo\":1,\"hello world\":\"HelloWorld\"}");
    Json::Value json;
    initialString >> json;
    const std::string endString = Mantid::Kernel::JsonHelpers::jsonToString(json);
    TS_ASSERT_EQUALS(initialString.str(), endString)
    TS_ASSERT_EQUALS(Mantid::Kernel::JsonHelpers::jsonToString(json),
                     Mantid::Kernel::JsonHelpers::jsonToString(Mantid::Kernel::JsonHelpers::stringToJson(endString)))
  }

  void testStringToJsonToString() {
    const std::string initialString =
        "{\"bar\":2,\"baz\":3,\"foo\":1,\"hello world\":\"HelloWorld\",\"string_number\":\"0\"}";
    const Json::Value json = Mantid::Kernel::JsonHelpers::stringToJson(initialString);
    TS_ASSERT_EQUALS(initialString, Mantid::Kernel::JsonHelpers::jsonToString(json))
  }

  void testParse() {
    const std::string initialString =
        "{\"bar\":2,\"baz\":3,\"foo\":1,\"hello world\":\"HelloWorld\",\"string_number\":\"0\"}";
    Json::Value json;
    bool result = Mantid::Kernel::JsonHelpers::parse(initialString, &json, NULL);
    TS_ASSERT(result)
    TS_ASSERT_EQUALS(initialString, Mantid::Kernel::JsonHelpers::jsonToString(json))
  }
};