#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidSINQ/PoldiAutoCorrelationCore.h"

#include "MantidSINQ/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiAbstractChopper.h"

#include "MantidDataObjects/TableWorkspace.h"

typedef std::pair<double, double> DoublePair;

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
    class MockDetector : public PoldiAbstractDetector
    {
    public:
        ~MockDetector() { }

        void loadConfiguration(DataObjects::TableWorkspace_sptr detectorConfigurationWorkspace)
        {
            UNUSED_ARG(detectorConfigurationWorkspace);
        }

        MOCK_METHOD1(twoTheta, double(int elementIndex));
        MOCK_METHOD1(distanceFromSample, double(int elementIndex));
        MOCK_METHOD0(elementCount, size_t());
        MOCK_METHOD0(centralElement, size_t());
        MOCK_METHOD2(qLimits, DoublePair(double lambdaMin, double lambdaMax));
    };

    class MockChopper : public PoldiAbstractChopper
    {
    public:
        ~MockChopper() { }

        void loadConfiguration(DataObjects::TableWorkspace_sptr chopperConfigurationWorkspace, DataObjects::TableWorkspace_sptr chopperSlitWorkspace, DataObjects::TableWorkspace_sptr chopperSpeedWorkspace)
        {
            UNUSED_ARG(chopperConfigurationWorkspace);
            UNUSED_ARG(chopperSlitWorkspace);
            UNUSED_ARG(chopperSpeedWorkspace);
        }

        MOCK_METHOD0(rotationSpeed, double());
        MOCK_METHOD0(cycleTime, double());
        MOCK_METHOD0(zeroOffset, double());
        MOCK_METHOD0(distanceFromSample, double());

        MOCK_METHOD1(setRotationSpeed, void(double rotationSpeed));

        std::vector<double> slitPositions() {
            double slits [] = {0.000000, 0.162156};

            return std::vector<double>(slits, slits + sizeof(slits) / sizeof(slits[0]));
        }
        std::vector<double> slitTimes() {
            double slits [] = {0.000000, 243.234};

            return std::vector<double>(slits, slits + sizeof(slits) / sizeof(slits[0]));
        }
    };

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

    void testgetRawElements()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);

        EXPECT_CALL(*mockDetector, elementCount())
                .WillOnce(Return(400));

        std::vector<int> rawElements = autoCorrelationCore.getRawElements();

        TS_ASSERT_EQUALS(rawElements.size(), 400);
        TS_ASSERT_EQUALS(rawElements.front(), 0);
        TS_ASSERT_EQUALS(rawElements.back(), 399);
    }

    void testgetGoodElements()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore;
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);

        EXPECT_CALL(*mockDetector, elementCount())
                .WillOnce(Return(400));

        std::vector<int> rawElements = autoCorrelationCore.getRawElements();

        std::vector<int> goodElementsNoOp = autoCorrelationCore.getGoodElements(rawElements);

        TS_ASSERT_EQUALS(rawElements.size(), goodElementsNoOp.size());

        int rawDeadWires[] = {1, 2, 3, 6, 100, 300, 400};
        std::set<int> deadWires(rawDeadWires, rawDeadWires + 7);
        autoCorrelationCore.setDeadWires(deadWires);
        std::vector<int> goodElements = autoCorrelationCore.getGoodElements(rawElements);

        TS_ASSERT_EQUALS(goodElements.size(), 393);
        TS_ASSERT_EQUALS(goodElements.front(), 3);
        TS_ASSERT_EQUALS(goodElements.back(), 398);

        int rawDeadWiresOutOfRange[] = {1, 2, 401};
        std::set<int> deadWiresOutOfRange(rawDeadWiresOutOfRange, rawDeadWiresOutOfRange + 3);
        autoCorrelationCore.setDeadWires(deadWiresOutOfRange);

        TS_ASSERT_THROWS(autoCorrelationCore.getGoodElements(rawElements), std::runtime_error);
    }
};


#endif /* MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_ */
