// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once

#include "../../../ISISReflectometry/Reduction/LookupRowFinder.h"
#include "TestHelpers/ModelCreationHelper.h"
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class LookupRowFinderTest : public CxxTest::TestSuite {
public:
  void test_searching_by_theta_found() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAnglesAndWildcard();

    LookupRowFinder findLookupRow(table);
    for (const auto angle : {0.5, 2.3}) {
      const auto lookupRow = findLookupRow(angle, m_exactMatchTolerance);
      TS_ASSERT(lookupRow)
      if (lookupRow) {
        const auto foundAngle = lookupRow->thetaOrWildcard();
        TS_ASSERT(foundAngle)
        TS_ASSERT_DELTA(*foundAngle, angle, m_exactMatchTolerance)
      }
    }
  }

  void test_searching_by_theta_tolerance_found() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAnglesAndWildcard();

    LookupRowFinder findLookupRow(table);
    const double matchTolerance = 0.01;
    for (const auto angle : {(0.5 - matchTolerance), (2.3 + matchTolerance)}) {
      const auto lookupRow = findLookupRow(angle, matchTolerance);
      TS_ASSERT(lookupRow)
      if (lookupRow) {
        const auto foundAngle = lookupRow->thetaOrWildcard();
        TS_ASSERT(foundAngle)
        if (foundAngle)
          TS_ASSERT_DELTA(*foundAngle, angle, matchTolerance)
      }
    }
  }

  void test_searching_by_theta_not_found_returns_wildcard() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAnglesAndWildcard();

    LookupRowFinder findLookupRow(table);
    for (const auto angle : {1.2, 3.4}) {
      const auto lookupRow = findLookupRow(angle, m_exactMatchTolerance);
      TS_ASSERT(lookupRow)
      if (lookupRow) {
        const auto foundAngle = lookupRow->thetaOrWildcard();
        TS_ASSERT(!foundAngle)
        TS_ASSERT(lookupRow->isWildcard())
      }
    }
  }

  void test_searching_by_theta_not_found_returns_nullptr() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAngles();

    LookupRowFinder findLookupRow(table);
    constexpr double notThere = 999;
    const auto lookupRow = findLookupRow(notThere, m_exactMatchTolerance);
    TS_ASSERT(!lookupRow)
  }

  void test_searching_empty_table_returns_nullptr() {
    LookupTable table = ModelCreationHelper::makeEmptyLookupTable();

    LookupRowFinder findLookupRow(table);
    constexpr double notThere = 0.5;
    const auto lookupRow = findLookupRow(notThere, m_exactMatchTolerance);
    TS_ASSERT(!lookupRow)
  }

  void test_searching_by_theta_and_title_found() {
    auto constexpr angle = 2.3;
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El"))};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(angle, m_exactMatchTolerance, "El Em En Oh");
    TS_ASSERT(foundLookupRow)
    if (foundLookupRow)
      TS_ASSERT_EQUALS(*foundLookupRow, table[1])
  }

  void test_searching_by_theta_and_title_found_with_wildcard_present() {
    auto constexpr angle = 2.3;
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El")),
                             ModelCreationHelper::makeWildcardLookupRow()};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(angle, m_exactMatchTolerance, "El Em En Oh");
    TS_ASSERT(foundLookupRow)
    if (foundLookupRow)
      TS_ASSERT_EQUALS(*foundLookupRow, table[1])
  }

  void test_searching_by_theta_found_but_title_not_found_returns_none() {
    auto constexpr angle = 2.3;
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El"))};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(angle, m_exactMatchTolerance, "En Oh");
    TS_ASSERT(!foundLookupRow.has_value())
  }

  void test_searching_by_title_found_but_theta_not_found_returns_none() {
    auto constexpr angle = 2.3;
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El"))};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(0.5, m_exactMatchTolerance, "En Oh");
    TS_ASSERT(!foundLookupRow.has_value())
  }

  void test_searching_by_theta_found_but_title_not_found_returns_wildcard() {
    auto constexpr angle = 2.3;
    auto wildcardRow = ModelCreationHelper::makeWildcardLookupRow();
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El")), wildcardRow};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(angle, m_exactMatchTolerance, "En Oh");
    TS_ASSERT(foundLookupRow.has_value())
    TS_ASSERT_EQUALS(wildcardRow, foundLookupRow)
  }

  void test_searching_by_title_found_but_theta_not_found_returns_wildcard() {
    auto constexpr angle = 2.3;
    auto wildcardRow = ModelCreationHelper::makeWildcardLookupRow();
    auto table = LookupTable{ModelCreationHelper::makeLookupRow(angle, boost::regex("Ay")),
                             ModelCreationHelper::makeLookupRow(angle, boost::regex("El")), wildcardRow};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(0.5, m_exactMatchTolerance, "En Oh");
    TS_ASSERT(foundLookupRow.has_value())
    TS_ASSERT_EQUALS(wildcardRow, foundLookupRow)
  }

  void test_searching_by_title_matches_regex_over_wildcard() {
    auto constexpr angle = 2.3;
    auto wildcardRow = ModelCreationHelper::makeWildcardLookupRow();
    auto regexRow = ModelCreationHelper::makeLookupRow(angle, boost::regex(".*"));
    auto table = LookupTable{wildcardRow, regexRow};

    LookupRowFinder findLookupRow(table);
    const auto foundLookupRow = findLookupRow(angle, m_exactMatchTolerance, "En Oh");
    TS_ASSERT(foundLookupRow.has_value())
    TS_ASSERT_EQUALS(regexRow, foundLookupRow)
  }

  // void test_searching_by_title_when_no_regex

  // error cases:
  // lookup title specified but theta is not
  // multiple wildcard rows
  // duplicate criteria
  // matches multiple non-empty titles with same theta
  // whitespace
private:
  const double m_exactMatchTolerance = 1e-6;
};
