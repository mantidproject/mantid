// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CATALOG_ONCATENTITYTEST_H_
#define MANTID_CATALOG_ONCATENTITYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCatalog/Exception.h"
#include "MantidCatalog/ONCatEntity.h"

using Mantid::Catalog::Exception::MalformedRepresentationError;
using Mantid::Catalog::ONCat::ONCatEntity;

class ONCatEntityTest : public CxxTest::TestSuite {
public:
  // CxxTest boilerplate.
  static ONCatEntityTest *createSuite() { return new ONCatEntityTest(); }
  static void destroySuite(ONCatEntityTest *suite) { delete suite; }

  void test_basic_attributes() {
    std::string dummyRepresentation =
        "{"
        "  \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\","
        "  \"type\": \"dummy\""
        "}";

    std::stringstream ss;
    ss << dummyRepresentation;

    auto dummy = ONCatEntity::fromJSONStream(ss);

    TS_ASSERT_EQUALS(dummy.id(),
                     std::string("3fa1d522-f1b8-4134-a56b-b61f24d20510"));
    TS_ASSERT_EQUALS(dummy.type(), std::string("dummy"));
  }

  void test_basic_attributes_of_entity_vector() {
    std::string dummiesRepresentation =
        "["
        "  {"
        "    \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\","
        "    \"type\": \"dummy\""
        "  },"
        "  {"
        "    \"id\": \"4b1dec2a-0f15-416d-8d23-e08901ac4634\","
        "    \"type\": \"dummy\""
        "  }"
        "]";

    std::stringstream ss;
    ss << dummiesRepresentation;

    auto dummies = ONCatEntity::vectorFromJSONStream(ss);

    TS_ASSERT_EQUALS(dummies.size(), 2);

    TS_ASSERT_EQUALS(dummies[0].id(),
                     std::string("3fa1d522-f1b8-4134-a56b-b61f24d20510"));
    TS_ASSERT_EQUALS(dummies[0].type(), std::string("dummy"));

    TS_ASSERT_EQUALS(dummies[1].id(),
                     std::string("4b1dec2a-0f15-416d-8d23-e08901ac4634"));
    TS_ASSERT_EQUALS(dummies[1].type(), std::string("dummy"));
  }

  void test_throws_on_malformed_json() {
    std::string malformedRepresentation =
        "{"
        "  \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\","
        "  \"type\": \"dummy";

    std::stringstream ss;
    ss << malformedRepresentation;

    TS_ASSERT_THROWS(ONCatEntity::fromJSONStream(ss),
                     const MalformedRepresentationError &);
  }

  void test_throws_on_malformed_representation() {
    std::string missingTypeRepresentation =
        "{"
        "  \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\""
        "}";

    std::stringstream ss;
    ss << missingTypeRepresentation;

    TS_ASSERT_THROWS(ONCatEntity::fromJSONStream(ss),
                     const MalformedRepresentationError &);
  }

  void test_nested_values_with_various_types() {
    std::string dummyRepresentation =
        "{"
        "  \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\","
        "  \"type\": \"dummy\","
        "  \"val\": {"
        "    \"a\": {"
        "      \"string\": \"value\","
        "      \"int\": 1234,"
        "      \"float\": 1234.5,"
        "      \"double\": 1234.5,"
        "      \"bool\": true"
        "    }"
        "  }"
        "}";

    std::stringstream ss;
    ss << dummyRepresentation;

    auto dummy = ONCatEntity::fromJSONStream(ss);

    TS_ASSERT_EQUALS(dummy.get<std::string>("val.a.string"),
                     std::string("value"));
    TS_ASSERT_EQUALS(dummy.get<int>("val.a.int"), 1234);
    TS_ASSERT_EQUALS(dummy.get<float>("val.a.float"), 1234.5f);
    TS_ASSERT_EQUALS(dummy.get<double>("val.a.double"), 1234.5);
    TS_ASSERT_EQUALS(dummy.get<bool>("val.a.bool"), true);

    TS_ASSERT(!dummy.get<std::string>("a.string"));
    TS_ASSERT(!dummy.get<int>("a.int"));
    TS_ASSERT(!dummy.get<float>("a.float"));
    TS_ASSERT(!dummy.get<double>("a.double"));
    TS_ASSERT(!dummy.get<bool>("a.bool"));
  }

  void test_default_values_with_various_types() {
    std::string dummyRepresentation =
        "{"
        "  \"id\": \"3fa1d522-f1b8-4134-a56b-b61f24d20510\","
        "  \"type\": \"dummy\""
        "  }"
        "}";

    std::stringstream ss;
    ss << dummyRepresentation;

    auto dummy = ONCatEntity::fromJSONStream(ss);

    TS_ASSERT_EQUALS(dummy.get<std::string>("a.string", "val"),
                     std::string("val"));
    TS_ASSERT_EQUALS(dummy.get<int>("a.int", 1234), 1234);
    TS_ASSERT_EQUALS(dummy.get<float>("a.float", 1234.5f), 1234.5f);
    TS_ASSERT_EQUALS(dummy.get<double>("a.double", 1234.5), 1234.5);
    TS_ASSERT_EQUALS(dummy.get<bool>("a.bool", true), true);
  }
};

#endif /* MANTID_CATALOG_ONCATENTITYTEST_H_ */
