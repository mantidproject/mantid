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
      const auto *lookupRow = findLookupRow(angle, m_exactMatchTolerance);
      TS_ASSERT(lookupRow)
      const auto foundAngle = lookupRow->thetaOrWildcard();
      TS_ASSERT(foundAngle)
      TS_ASSERT_DELTA(*foundAngle, angle, m_exactMatchTolerance)
    }
  }

  void test_searching_by_theta_tolerance_found() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAnglesAndWildcard();

    LookupRowFinder findLookupRow(table);
    const double matchTolerance = 0.01;
    for (const auto angle : {(0.5 - matchTolerance), (2.3 + matchTolerance)}) {
      const auto *lookupRow = findLookupRow(angle, matchTolerance);
      TS_ASSERT(lookupRow)
      const auto foundAngle = lookupRow->thetaOrWildcard();
      TS_ASSERT(foundAngle)
      if (foundAngle)
        TS_ASSERT_DELTA(*foundAngle, angle, matchTolerance)
    }
  }

  void test_searching_by_theta_not_found_returns_wildcard() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAnglesAndWildcard();

    LookupRowFinder findLookupRow(table);
    for (const auto angle : {1.2, 3.4}) {
      const auto *lookupRow = findLookupRow(angle, m_exactMatchTolerance);
      TS_ASSERT(lookupRow)
      const auto foundAngle = lookupRow->thetaOrWildcard();
      TS_ASSERT(!foundAngle)
      TS_ASSERT(lookupRow->isWildcard())
    }
  }

  void test_searching_by_theta_not_found_returns_nullptr() {
    LookupTable table = ModelCreationHelper::makeLookupTableWithTwoAngles();

    LookupRowFinder findLookupRow(table);
    constexpr double notThere = 999;
    const auto *lookupRow = findLookupRow(notThere, m_exactMatchTolerance);
    TS_ASSERT(!lookupRow)
  }

  void test_searching_empty_table_returns_nullptr() {
    LookupTable table = ModelCreationHelper::makeEmptyLookupTable();

    LookupRowFinder findLookupRow(table);
    constexpr double notThere = 0.5;
    const auto *lookupRow = findLookupRow(notThere, m_exactMatchTolerance);
    TS_ASSERT(!lookupRow)
  }

private:
  const double m_exactMatchTolerance = 1e-6;
};
