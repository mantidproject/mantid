#ifndef MANTID_API_INDEXTYPEPROPERTYTEST_H_
#define MANTID_API_INDEXTYPEPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/IndexTypeProperty.h"

using namespace Mantid::API;
using Mantid::API::IndexTypeProperty;

class IndexTypePropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IndexTypePropertyTest *createSuite() {
    return new IndexTypePropertyTest();
  }
  static void destroySuite(IndexTypePropertyTest *suite) { delete suite; }

  void testConstruct() {
    TS_ASSERT_THROWS_NOTHING((IndexTypeProperty(IndexType::SpectrumNumber)));
  }

  void testContructorFailsWithInvalidIndexType() {
    TS_ASSERT_THROWS(IndexTypeProperty(0), std::invalid_argument);
  }

  void testSingleIndexTypeAutomaticallySet() {
    IndexTypeProperty prop1(IndexType::SpectrumNumber);
    IndexTypeProperty prop2(IndexType::WorkspaceIndex);

    TS_ASSERT_EQUALS(prop1.value(), "SpectrumNumber");
    TS_ASSERT_EQUALS(prop2.value(), "WorkspaceIndex");
  }

  void testAllowedValuesCorrectlySet() {
    IndexTypeProperty prop(IndexType::SpectrumNumber |
                           IndexType::WorkspaceIndex);
    auto allowed = prop.allowedValues();

    TS_ASSERT_EQUALS(allowed.size(), 2);
    TS_ASSERT(std::find(allowed.cbegin(), allowed.cend(), "SpectrumNumber") !=
              allowed.cend());
    TS_ASSERT(std::find(allowed.cbegin(), allowed.cend(), "WorkspaceIndex") !=
              allowed.cend());
  }

  void testAllowedTypesCorrectlySet() {
    IndexTypeProperty prop(IndexType::SpectrumNumber |
                           IndexType::WorkspaceIndex);
    auto allowed = prop.allowedTypes();

    TS_ASSERT(allowed & IndexType::SpectrumNumber);
    TS_ASSERT(allowed & IndexType::WorkspaceIndex);
  }

  void testCorrectTypeReturnedWhenSetWithString() {
    IndexTypeProperty prop(IndexType::SpectrumNumber |
                           IndexType::WorkspaceIndex);

    prop = "SpectrumNumber";

    TS_ASSERT(prop.selectedType() & IndexType::SpectrumNumber);

    prop = "WorkspaceIndex";

    TS_ASSERT(prop.selectedType() & IndexType::WorkspaceIndex);
  }

  void testCorrectTypeReturnedWhenSetWithIndexType() {
    IndexTypeProperty prop(IndexType::SpectrumNumber);

    prop = IndexType::SpectrumNumber;

    TS_ASSERT(prop.selectedType() & IndexType::SpectrumNumber);

    prop = IndexType::WorkspaceIndex;

    TS_ASSERT(prop.selectedType() & IndexType::WorkspaceIndex);
  }
};

#endif /* MANTID_API_INDEXTYPEPROPERTYTEST_H_ */