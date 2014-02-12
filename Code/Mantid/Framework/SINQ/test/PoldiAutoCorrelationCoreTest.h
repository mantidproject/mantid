#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidSINQ/PoldiAutoCorrelationCore.h"

#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"

#include "MantidDataObjects/TableWorkspace.h"

#include "MantidSINQ/PoldiMockInstrumentHelpers.h"

using ::testing::Return;

using namespace Mantid;
using namespace Mantid::Poldi;

class TestablePoldiAutoCorrelationCore : public PoldiAutoCorrelationCore
{
    friend class PoldiAutoCorrelationCoreTest;
};

class PoldiAutoCorrelationCoreTest : public CxxTest::TestSuite
{
private:


public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiAutoCorrelationCoreTest *createSuite() { return new PoldiAutoCorrelationCoreTest(); }
    static void destroySuite( PoldiAutoCorrelationCoreTest *suite ) { delete suite; }

    void testgetDeltaD()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);

        EXPECT_CALL(*mockDetector, centralElement())
                .WillOnce(Return(199));
        EXPECT_CALL(*mockChopper, distanceFromSample())
                .WillOnce(Return(11800.0));
        EXPECT_CALL(*mockDetector, distanceFromSample(199))
                .WillOnce(Return(1996.017578125));
        EXPECT_CALL(*mockDetector, twoTheta(199))
                .WillOnce(Return(1.577357650));

        TS_ASSERT_DELTA(autoCorrelationCore.getDeltaD(3.0), 0.000606307, 1e-9);
    }

    void testgetDRangeAsDeltaMultiples()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);
        autoCorrelationCore.setWavelengthRange(1.1, 5.0);

        EXPECT_CALL(*mockDetector, qLimits(1.1, 5.0))
                .WillOnce(Return(std::make_pair(1.549564, 8.960878)));

        std::pair<int, int> drange = autoCorrelationCore.getDRangeAsDeltaMultiples(0.000606307);

        TS_ASSERT_EQUALS(drange.first, 1156);
        TS_ASSERT_EQUALS(drange.second, 6687);
    }

    void testgetDGrid()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);
        autoCorrelationCore.setWavelengthRange(1.1, 5.0);

        EXPECT_CALL(*mockDetector, centralElement())
                .WillOnce(Return(199));
        EXPECT_CALL(*mockChopper, distanceFromSample())
                .WillOnce(Return(11800.0));
        EXPECT_CALL(*mockDetector, distanceFromSample(199))
                .WillOnce(Return(1996.017578125));
        EXPECT_CALL(*mockDetector, twoTheta(199))
                .WillOnce(Return(1.577357650));

        EXPECT_CALL(*mockDetector, qLimits(1.1, 5.0))
                .WillOnce(Return(std::make_pair(1.549564, 8.960878)));

        std::vector<double> dgrid = autoCorrelationCore.getDGrid(3.0);

        TS_ASSERT_DELTA(dgrid[0], 0.700890601 + 0.000606307, 1e-7);
        TS_ASSERT_DELTA(dgrid[1] - dgrid[0], 0.000606307, 1e-9);
        TS_ASSERT_DELTA(dgrid.back(), 4.0543741859, 1e-6);

        TS_ASSERT_EQUALS(dgrid.size(), 5531);
    }

    void testConversions()
    {
        double distance = 11800.0 + 1996.017578125;
        double sinTheta = sin(1.577357650 / 2.0);
        double tof = 3.0;

        double d = PoldiAutoCorrelationCore::TOFtod(tof, distance, sinTheta);

        TS_ASSERT_DELTA(d, 0.000606307, 1e-9);
        TS_ASSERT_EQUALS(PoldiAutoCorrelationCore::dtoTOF(d, distance, sinTheta), tof);
    }
};


#endif /* MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_ */
