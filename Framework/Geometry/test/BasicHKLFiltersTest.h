// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidGeometry/Crystal/BasicHKLFilters.h"
#include "MantidGeometry/Crystal/SpaceGroupFactory.h"
#include "MantidGeometry/Crystal/StructureFactorCalculator.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;

using ::testing::_;
using ::testing::Mock;
using ::testing::Return;

class BasicHKLFiltersTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BasicHKLFiltersTest *createSuite() { return new BasicHKLFiltersTest(); }
  static void destroySuite(BasicHKLFiltersTest *suite) { delete suite; }

  void testHKLFilterNone() {
    HKLFilterNone filter;

    TS_ASSERT(filter.isAllowed(V3D(1, 2, 3)))
    TS_ASSERT(filter.isAllowed(V3D(-1, -2, 3)))
    TS_ASSERT(filter.isAllowed(V3D(-1, -2, -3)))
    TS_ASSERT(filter.isAllowed(V3D(120380123, 4012983, -131233)))
  }

  void testHKLFilterDRangeConstructors() {
    UnitCell cell(10., 10., 10.);

    TS_ASSERT_THROWS_NOTHING(HKLFilterDRange dFilter(cell, 1.0));
    TS_ASSERT_THROWS(HKLFilterDRange dFilter(cell, -1.0), const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterDRange dFilter(cell, 0.0), const std::range_error &);

    TS_ASSERT_THROWS_NOTHING(HKLFilterDRange dFilter(cell, 1.0, 2.0));
    TS_ASSERT_THROWS(HKLFilterDRange dFilter(cell, 1.0, 0.5), const std::range_error &);
    TS_ASSERT_THROWS(HKLFilterDRange dFilter(cell, 1.0, -0.5), const std::range_error &);
  }

  void testHKLFilterDRangeDescription() {
    UnitCell cell(10., 10., 10.);

    std::ostringstream reference;
    reference << "(" << 1.0 << " <= d <= " << 10.0 << ")";

    HKLFilterDRange dFilter(cell, 1.0);
    TS_ASSERT_EQUALS(dFilter.getDescription(), reference.str());
  }

  void testHKLFilterDRangeIsAllowed() {
    UnitCell cell(10., 10., 10.);
    HKLFilterDRange dFilter(cell, 1.0, 9.0);

    TS_ASSERT(dFilter.isAllowed(V3D(1, 2, 3)));

    TS_ASSERT(dFilter.isAllowed(V3D(2, 0, 0)));
    TS_ASSERT(!dFilter.isAllowed(V3D(1, 0, 0)));

    TS_ASSERT(dFilter.isAllowed(V3D(10, 0, 0)));
    TS_ASSERT(!dFilter.isAllowed(V3D(11, 0, 0)));
  }

  void testHKLFilterSpaceGroupConstructor() {
    SpaceGroup_const_sptr invalid;
    TS_ASSERT_THROWS(HKLFilterSpaceGroup sgFilter(invalid), const std::runtime_error &);

    SpaceGroup_const_sptr sg = SpaceGroupFactory::Instance().createSpaceGroup("F d -3 m");
    TS_ASSERT_THROWS_NOTHING(HKLFilterSpaceGroup sgFilter(sg));
  }

  void testHKLFilterSpaceGroupDescription() {
    SpaceGroup_const_sptr sg = SpaceGroupFactory::Instance().createSpaceGroup("F d -3 m");

    HKLFilterSpaceGroup sgFilter(sg);

    TS_ASSERT_EQUALS(sgFilter.getDescription(), "(Space group: " + sg->hmSymbol() + ")");
  }

  void testHKLFilterSpaceGroupIsAllowed() {
    SpaceGroup_const_sptr sg = SpaceGroupFactory::Instance().createSpaceGroup("F d -3 m");

    HKLFilterSpaceGroup sgFilter(sg);

    TS_ASSERT(!sgFilter.isAllowed(V3D(1, 0, 0)));
    TS_ASSERT(!sgFilter.isAllowed(V3D(1, 1, 0)));
    TS_ASSERT(sgFilter.isAllowed(V3D(1, 1, 1)));

    TS_ASSERT(!sgFilter.isAllowed(V3D(2, 0, 0)));
    TS_ASSERT(!sgFilter.isAllowed(V3D(3, 0, 0)));
    TS_ASSERT(sgFilter.isAllowed(V3D(4, 0, 0)));
  }

  void testHKLFilterStructureFactorConstructor() {
    StructureFactorCalculator_sptr invalid;
    TS_ASSERT_THROWS(HKLFilterStructureFactor sfFilter(invalid), const std::runtime_error &);

    StructureFactorCalculator_sptr mock = std::make_shared<MockStructureFactorCalculator>();
    TS_ASSERT_THROWS_NOTHING(HKLFilterStructureFactor sfFilter(mock));
    TS_ASSERT_THROWS_NOTHING(HKLFilterStructureFactor sfFilter(mock, 12.0));
  }

  void testHKLFilterStructureFactorDescription() {
    std::ostringstream reference;
    reference << "(F^2 > " << 1.0 << ")";

    StructureFactorCalculator_sptr mock = std::make_shared<MockStructureFactorCalculator>();
    HKLFilterStructureFactor sfFilter(mock, 1.0);
    TS_ASSERT_EQUALS(sfFilter.getDescription(), reference.str());
  }

  void testHKLFilterStructureFactorIsAllowed() {
    std::shared_ptr<MockStructureFactorCalculator> mock = std::make_shared<MockStructureFactorCalculator>();

    EXPECT_CALL(*mock, getFSquared(_)).WillOnce(Return(2.0)).WillOnce(Return(0.5)).WillOnce(Return(1.0));

    HKLFilterStructureFactor sfFilter(mock, 1.0);
    TS_ASSERT(sfFilter.isAllowed(V3D(1, 1, 1)));
    TS_ASSERT(!sfFilter.isAllowed(V3D(1, 1, 1)));
    TS_ASSERT(!sfFilter.isAllowed(V3D(1, 1, 1)));

    TS_ASSERT(Mock::VerifyAndClearExpectations(mock.get()))
  }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
private:
  class MockStructureFactorCalculator : public StructureFactorCalculator {
  public:
    MOCK_CONST_METHOD1(getF, StructureFactor(const V3D &));
    MOCK_CONST_METHOD1(getFSquared, double(const V3D &));
  };
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
