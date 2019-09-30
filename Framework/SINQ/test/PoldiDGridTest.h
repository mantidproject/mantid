// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef POLDIDGRIDTEST_H
#define POLDIDGRIDTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidSINQ/PoldiUtilities/PoldiDGrid.h"
#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"

#include <stdexcept>

using ::testing::Return;
using namespace Mantid::Poldi;

class TestablePoldiDGrid : public PoldiDGrid {
  friend class PoldiDGridTest;

  TestablePoldiDGrid(
      boost::shared_ptr<PoldiAbstractDetector> detector =
          boost::shared_ptr<PoldiAbstractDetector>(),
      boost::shared_ptr<PoldiAbstractChopper> chopper =
          boost::shared_ptr<PoldiAbstractChopper>(),
      double deltaT = 0.0,
      std::pair<double, double> wavelengthRange = std::pair<double, double>())
      : PoldiDGrid(detector, chopper, deltaT, wavelengthRange) {}
};

class PoldiDGridTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PoldiDGridTest *createSuite() { return new PoldiDGridTest(); }
  static void destroySuite(PoldiDGridTest *suite) { delete suite; }

  void testDefaultConstructor() {
    TestablePoldiDGrid grid;

    TS_ASSERT_EQUALS(grid.m_detector,
                     boost::shared_ptr<PoldiAbstractDetector>());
    TS_ASSERT_EQUALS(grid.m_chopper, boost::shared_ptr<PoldiAbstractChopper>());
    TS_ASSERT_EQUALS(grid.m_deltaT, 0.0);
    TS_ASSERT_EQUALS(grid.m_wavelengthRange, std::make_pair(0.0, 0.0));

    TS_ASSERT_EQUALS(grid.m_hasCachedCalculation, false);
    TS_ASSERT_EQUALS(grid.m_deltaD, 0.0);

    TS_ASSERT_THROWS(grid.createGrid(), const std::runtime_error &);
  }

  void testdeltaD() {
    boost::shared_ptr<MockDetector> mockDetector =
        boost::make_shared<MockDetector>();
    boost::shared_ptr<MockChopper> mockChopper =
        boost::make_shared<MockChopper>();

    TestablePoldiDGrid grid(mockDetector, mockChopper, 3.0,
                            std::make_pair(1.1, 5.0));

    EXPECT_CALL(*mockDetector, centralElement()).WillOnce(Return(199));
    EXPECT_CALL(*mockChopper, distanceFromSample()).WillOnce(Return(11800.0));
    EXPECT_CALL(*mockDetector, distanceFromSample(199))
        .WillOnce(Return(1996.017578125));
    EXPECT_CALL(*mockDetector, twoTheta(199)).WillOnce(Return(1.577357650));

    EXPECT_CALL(*mockDetector, qLimits(1.1, 5.0))
        .WillOnce(Return(std::make_pair(1.549564, 8.960878)));

    TS_ASSERT_DELTA(grid.deltaD(), 0.000606307, 1e-9);
    TS_ASSERT_EQUALS(grid.m_hasCachedCalculation, true);
  }

  void testdRange() {
    boost::shared_ptr<MockDetector> mockDetector =
        boost::make_shared<MockDetector>();
    boost::shared_ptr<MockChopper> mockChopper =
        boost::make_shared<MockChopper>();

    TestablePoldiDGrid grid(mockDetector, mockChopper, 3.0,
                            std::make_pair(1.1, 5.0));

    EXPECT_CALL(*mockDetector, centralElement()).WillOnce(Return(199));
    EXPECT_CALL(*mockChopper, distanceFromSample()).WillOnce(Return(11800.0));
    EXPECT_CALL(*mockDetector, distanceFromSample(199))
        .WillOnce(Return(1996.017578125));
    EXPECT_CALL(*mockDetector, twoTheta(199)).WillOnce(Return(1.577357650));

    EXPECT_CALL(*mockDetector, qLimits(1.1, 5.0))
        .WillOnce(Return(std::make_pair(1.549564, 8.960878)));

    grid.createGrid();

    TS_ASSERT_EQUALS(grid.m_dRangeAsMultiples.first, 1156);
    TS_ASSERT_EQUALS(grid.m_dRangeAsMultiples.second, 6687);
    TS_ASSERT_EQUALS(grid.m_hasCachedCalculation, true);
  }

  void testgrid() {
    boost::shared_ptr<MockDetector> mockDetector =
        boost::make_shared<MockDetector>();
    boost::shared_ptr<MockChopper> mockChopper =
        boost::make_shared<MockChopper>();

    TestablePoldiDGrid grid(mockDetector, mockChopper, 3.0,
                            std::make_pair(1.1, 5.0));

    EXPECT_CALL(*mockDetector, centralElement()).WillOnce(Return(199));
    EXPECT_CALL(*mockChopper, distanceFromSample()).WillOnce(Return(11800.0));
    EXPECT_CALL(*mockDetector, distanceFromSample(199))
        .WillOnce(Return(1996.017578125));
    EXPECT_CALL(*mockDetector, twoTheta(199)).WillOnce(Return(1.577357650));

    EXPECT_CALL(*mockDetector, qLimits(1.1, 5.0))
        .WillOnce(Return(std::make_pair(1.549564, 8.960878)));

    std::vector<double> dgrid = grid.grid();

    TS_ASSERT_DELTA(dgrid[0], 0.700890601 + 0.000606307, 1e-7);
    TS_ASSERT_DELTA(dgrid[1] - dgrid[0], 0.000606307, 1e-9);
    TS_ASSERT_DELTA(dgrid.back(), 4.0543741859, 1e-6);

    TS_ASSERT_EQUALS(dgrid.size(), 5531);

    TS_ASSERT_EQUALS(grid.m_hasCachedCalculation, true);
  }
};

#endif // POLDIDGRIDTEST_H
