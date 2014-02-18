#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidSINQ/PoldiAutoCorrelationCore.h"

#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiDeadWireDecorator.h"

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

    void testgetTOFsForD1()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(detector, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(1)
                .WillRepeatedly(Return(11800.0));

        std::vector<double> tofsForD1 = autoCorrelationCore.getTofsForD1(detector->availableElements());

        TS_ASSERT_DELTA(tofsForD1[0], 4257.66624637, 1e-4);
        TS_ASSERT_DELTA(tofsForD1[399], 5538.73486007, 1e-4);
    }

    void testgetDistances()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(detector, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(1)
                .WillRepeatedly(Return(11800.0));

        std::vector<double> distances = autoCorrelationCore.getDistances(detector->availableElements());

        TS_ASSERT_DELTA(distances[0] - 11800.0, 1859.41, 1e-2);
        TS_ASSERT_DELTA(distances[399]- 11800.0, 2167.13, 1e-2);
    }

    void testgetNormalizedTOFSum()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        int deadWires [] = {1, 2, 3, 4, 5, 6, 395, 396, 397, 398, 399, 400 };
        boost::shared_ptr<PoldiDeadWireDecorator> deadWireDecorator(
                    new PoldiDeadWireDecorator(std::set<int>(deadWires, deadWires + 12), detector));

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(deadWireDecorator, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(2)
                .WillRepeatedly(Return(11800.0));
        std::vector<double> tofsD1 = autoCorrelationCore.getTofsForD1(deadWireDecorator->availableElements());

        TS_ASSERT_EQUALS(tofsD1.size(), 388);

        double sum = autoCorrelationCore.getNormalizedTOFSum(tofsD1, 3.0, 5531);

        TS_ASSERT_DELTA(1.0 / 5531.0, autoCorrelationCore.m_weightsForD[0] / sum, 1e-15);

        TS_ASSERT_DELTA(sum, 2139673.0, 1e-1);
    }

    void testgetNormalizedTOFSumAlternative()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        int deadWires [] = {1, 2, 3, 4, 5, 6, 395, 396, 397, 398, 399, 400 };
        boost::shared_ptr<PoldiDeadWireDecorator> deadWireDecorator(
                    new PoldiDeadWireDecorator(std::set<int>(deadWires, deadWires + 12), detector));

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(deadWireDecorator, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(2)
                .WillRepeatedly(Return(11800.0));
        std::vector<double> tofsD1 = autoCorrelationCore.getTofsForD1(deadWireDecorator->availableElements());

        TS_ASSERT_EQUALS(tofsD1.size(), 388);

        double sum = autoCorrelationCore.getNormalizedTOFSumAlternative(tofsD1, 3.0, 5531);

        TS_ASSERT_DELTA(sum, 2139673., 1e2);

    }
};


#endif /* MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_ */
