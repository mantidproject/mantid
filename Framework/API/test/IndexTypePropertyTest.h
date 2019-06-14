// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
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
    TS_ASSERT_THROWS_NOTHING(
        (IndexTypeProperty("IndexType", IndexType::SpectrumNum)));
  }

  void testContructorFailsWithInvalidIndexType() {
    TS_ASSERT_THROWS(IndexTypeProperty("IndexType", 0),
                     const std::invalid_argument &);
  }

  void testSingleIndexTypeAutomaticallySet() {
    IndexTypeProperty prop1("IndexType", IndexType::SpectrumNum);
    IndexTypeProperty prop2("IndexType", IndexType::WorkspaceIndex);

    TS_ASSERT_EQUALS(prop1.value(), "SpectrumNumber");
    TS_ASSERT_EQUALS(prop2.value(), "WorkspaceIndex");
  }

  void testAllowedValuesCorrectlySet() {
    IndexTypeProperty prop("IndexType",
                           IndexType::SpectrumNum | IndexType::WorkspaceIndex);
    auto allowed = prop.allowedValues();

    TS_ASSERT_EQUALS(allowed.size(), 2);
    TS_ASSERT(std::find(allowed.cbegin(), allowed.cend(), "SpectrumNumber") !=
              allowed.cend());
    TS_ASSERT(std::find(allowed.cbegin(), allowed.cend(), "WorkspaceIndex") !=
              allowed.cend());
  }

  void testAllowedTypesCorrectlySet() {
    IndexTypeProperty prop("IndexType",
                           IndexType::SpectrumNum | IndexType::WorkspaceIndex);
    auto allowed = prop.allowedTypes();

    TS_ASSERT(allowed & IndexType::SpectrumNum);
    TS_ASSERT(allowed & IndexType::WorkspaceIndex);
  }

  void testCorrectTypeReturnedWhenSetWithString() {
    IndexTypeProperty prop("IndexType",
                           IndexType::SpectrumNum | IndexType::WorkspaceIndex);

    prop = "SpectrumNumber";

    TS_ASSERT_EQUALS(prop.selectedType(), IndexType::SpectrumNum);

    prop = "WorkspaceIndex";

    TS_ASSERT_EQUALS(prop.selectedType(), IndexType::WorkspaceIndex);
  }

  void testCorrectTypeReturnedWhenSetWithIndexType() {
    IndexTypeProperty prop("IndexType", IndexType::SpectrumNum);

    prop = IndexType::SpectrumNum;

    TS_ASSERT_EQUALS(prop.selectedType(), IndexType::SpectrumNum);

    prop = IndexType::WorkspaceIndex;

    TS_ASSERT_EQUALS(prop.selectedType(), IndexType::WorkspaceIndex);
  }

  void testGeneratePropertyName() {
    std::string propName = "InputWorkspace";
    TS_ASSERT_EQUALS(propName + "IndexType",
                     IndexTypeProperty::generatePropertyName(propName));
  }
};

#endif /* MANTID_API_INDEXTYPEPROPERTYTEST_H_ */