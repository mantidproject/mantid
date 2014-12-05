#ifndef MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <boost/assign.hpp>

#include "MantidSINQ/PoldiUtilities/PoldiAutoCorrelationCore.h"

#include "MantidSINQ/PoldiUtilities/PoldiAbstractDetector.h"
#include "MantidSINQ/PoldiUtilities/PoldiAbstractChopper.h"
#include "MantidSINQ/PoldiUtilities/PoldiDeadWireDecorator.h"
#include "MantidSINQ/PoldiUtilities/PoldiDGrid.h"

#include "MantidDataObjects/TableWorkspace.h"

#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidDataObjects/Workspace2D.h"

#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using ::testing::Return;

using namespace Mantid;
using namespace Mantid::Poldi;
using namespace Mantid::DataObjects;

class TestablePoldiAutoCorrelationCore : public PoldiAutoCorrelationCore
{
    friend class PoldiAutoCorrelationCoreTest;
    TestablePoldiAutoCorrelationCore(Mantid::Kernel::Logger &logger)
      : PoldiAutoCorrelationCore(logger)
    {
    }
};

class PoldiAutoCorrelationCoreTest : public CxxTest::TestSuite
{
private:
    TestablePoldiAutoCorrelationCore getCorrelationCoreWithInstrument()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        int deadWires [] = {0, 1, 2, 3, 4, 5, 394, 395, 396, 397, 398, 399 };
        boost::shared_ptr<PoldiDeadWireDecorator> deadWireDecorator(
                    new PoldiDeadWireDecorator(std::set<int>(deadWires, deadWires + 12), detector));

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setInstrument(deadWireDecorator, mockChopper);

        return autoCorrelationCore;
    }

public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiAutoCorrelationCoreTest *createSuite() { return new PoldiAutoCorrelationCoreTest(); }
    static void destroySuite( PoldiAutoCorrelationCoreTest *suite ) { delete suite; }

    PoldiAutoCorrelationCoreTest() : m_log("PoldiAutoCorrelationCoreTest")
    {
    }

    void testsetInstrument()
    {
        boost::shared_ptr<MockDetector> mockDetector(new MockDetector);
        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setInstrument(mockDetector, mockChopper);

        TS_ASSERT_EQUALS(autoCorrelationCore.m_chopper, mockChopper);
        TS_ASSERT_EQUALS(autoCorrelationCore.m_detector, mockDetector);
    }

    void testsetWavelengthRange()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setWavelengthRange(1.1, 5.0);

        TS_ASSERT_EQUALS(autoCorrelationCore.m_wavelengthRange.first, 1.1);
        TS_ASSERT_EQUALS(autoCorrelationCore.m_wavelengthRange.second, 5.0);
    }

    void testgetTOFsFor1Angstrom()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setInstrument(detector, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(1)
                .WillRepeatedly(Return(11800.0));

        std::vector<double> tofsForD1 = autoCorrelationCore.getTofsFor1Angstrom(detector->availableElements());

        TS_ASSERT_DELTA(tofsForD1[0], 4257.66624637, 1e-4);
        TS_ASSERT_DELTA(tofsForD1[399], 5538.73486007, 1e-4);
    }

    void testgetDistances()
    {
        boost::shared_ptr<PoldiAbstractDetector> detector(new ConfiguredHeliumDetector);

        boost::shared_ptr<MockChopper> mockChopper(new MockChopper);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setInstrument(detector, mockChopper);

        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(1)
                .WillRepeatedly(Return(11800.0));

        std::vector<double> distances = autoCorrelationCore.getDistances(detector->availableElements());

        TS_ASSERT_DELTA(distances[0] - 11800.0, 1859.41, 1e-2);
        TS_ASSERT_DELTA(distances[399]- 11800.0, 2167.13, 1e-2);
    }

    void testcalculateDWeights()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore = getCorrelationCoreWithInstrument();
        boost::shared_ptr<MockChopper> mockChopper = boost::dynamic_pointer_cast<MockChopper>(autoCorrelationCore.m_chopper);
        EXPECT_CALL(*mockChopper, distanceFromSample())
                .WillOnce(Return(11800.0));

        std::vector<double> tofsD1 = autoCorrelationCore.getTofsFor1Angstrom(autoCorrelationCore.m_detector->availableElements());

        TS_ASSERT_EQUALS(tofsD1.size(), 388);

        double deltaT = 3.0;
        double deltaD = 0.000606307;
        size_t nd = 5531;

        std::vector<double> weights = autoCorrelationCore.calculateDWeights(tofsD1, deltaT, deltaD, nd);

        TS_ASSERT_EQUALS(weights[0], weights[1]);
    }

    void testgetNormalizedTOFSum()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore = getCorrelationCoreWithInstrument();
        boost::shared_ptr<MockChopper> mockChopper = boost::dynamic_pointer_cast<MockChopper>(autoCorrelationCore.m_chopper);
        EXPECT_CALL(*mockChopper, distanceFromSample())
                .Times(2)
                .WillRepeatedly(Return(11800.0));

        std::vector<double> tofsD1 = autoCorrelationCore.getTofsFor1Angstrom(autoCorrelationCore.m_detector->availableElements());
        TS_ASSERT_EQUALS(tofsD1.size(), 388);

        PoldiDGrid dGrid(autoCorrelationCore.m_detector, autoCorrelationCore.m_chopper, 3.0, std::make_pair(1.1, 5.0));

        double deltaT = 3.0;
        double deltaD = dGrid.deltaD();
        size_t nd = 5531;

        std::vector<double> weights = autoCorrelationCore.calculateDWeights(tofsD1, deltaT, deltaD, nd);
        double sum = autoCorrelationCore.getNormalizedTOFSum(weights);

        TS_ASSERT_DELTA(1.0 / 5531.0, weights[0] / sum, 1e-15);

        TS_ASSERT_DELTA(sum, 2139673.0, 1e-1);
    }

    void testCleanIndex()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);

        TS_ASSERT_EQUALS(autoCorrelationCore.cleanIndex(-10, 500), 490);
        TS_ASSERT_EQUALS(autoCorrelationCore.cleanIndex(550, 500), 50);
        TS_ASSERT_EQUALS(autoCorrelationCore.cleanIndex(500, 500), 0);
    }

    void testgetElementFromIndex()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);

        int elements[] = {10, 20, 30, 40};
        autoCorrelationCore.m_detectorElements = std::vector<int>(elements, elements + 4);

        TS_ASSERT_EQUALS(autoCorrelationCore.getElementFromIndex(0), 10);
        TS_ASSERT_EQUALS(autoCorrelationCore.getElementFromIndex(3), 40);

        TS_ASSERT_THROWS(autoCorrelationCore.getElementFromIndex(10), std::range_error);
        TS_ASSERT_THROWS(autoCorrelationCore.getElementFromIndex(-10), std::range_error);
    }

    void testgetTofFromIndex()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);

        double elements[] = {345.0, 3123.2, 232.1, 65765.2};
        autoCorrelationCore.m_tofsFor1Angstrom = std::vector<double>(elements, elements + 4);

        TS_ASSERT_EQUALS(autoCorrelationCore.getTofFromIndex(0), 345.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getTofFromIndex(3), 65765.2);

        TS_ASSERT_THROWS(autoCorrelationCore.getTofFromIndex(10), std::range_error);
        TS_ASSERT_THROWS(autoCorrelationCore.getTofFromIndex(-10), std::range_error);
    }

    void testgetCounts()
    {
        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setCountData(testWorkspace);

        TS_ASSERT_EQUALS(autoCorrelationCore.m_countData->getNumberHistograms(), 2);

        TS_ASSERT_EQUALS(autoCorrelationCore.getCounts(0, 0), 0.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getCounts(0, 1), 0.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getCounts(1, 0), 1.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getCounts(1, 1), 1.0);
    }

    void testgetNormCounts()
    {
        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setNormCountData(testWorkspace);

        TS_ASSERT_EQUALS(autoCorrelationCore.getNormCounts(0, 0), 1.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getNormCounts(0, 1), 1.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getNormCounts(1, 0), 1.0);
        TS_ASSERT_EQUALS(autoCorrelationCore.getNormCounts(1, 1), 1.0);
    }

    void testgetSumOfCounts()
    {
        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);

        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);
        autoCorrelationCore.setCountData(testWorkspace);
        autoCorrelationCore.setNormCountData(testWorkspace);

        int elements[] = {0, 1};
        std::vector<int> elementVector(elements, elements + 2);
        TS_ASSERT_EQUALS(autoCorrelationCore.getSumOfCounts(2, elementVector), 2.0)
    }

    void testgetCMessAndCSigma()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore = getCorrelationCoreWithInstrument();
        boost::shared_ptr<MockChopper> mockChopper = boost::dynamic_pointer_cast<MockChopper>(autoCorrelationCore.m_chopper);

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        autoCorrelationCore.setCountData(testWorkspace);
        autoCorrelationCore.setNormCountData(testWorkspace);

        EXPECT_CALL(*mockChopper, zeroOffset())
                .WillRepeatedly(Return(0.0));

        autoCorrelationCore.m_deltaD = 0.01;
        autoCorrelationCore.m_deltaT = 3.0;
        autoCorrelationCore.m_timeBinCount = 2;

        double tofElements[] = {1.0, 2.0};
        autoCorrelationCore.m_tofsFor1Angstrom = std::vector<double>(tofElements, tofElements + 2);

        int elements[] = {0, 1};
        autoCorrelationCore.m_detectorElements = std::vector<int>(elements, elements + 2);

        TS_ASSERT_DELTA(autoCorrelationCore.getCMessAndCSigma(1.2, 0.0, 0).value(), 0.0, 1e-6);
        TS_ASSERT_DELTA(autoCorrelationCore.getCMessAndCSigma(1.2, 0.0, 0).error(), 0.00333333, 1e-6);
    }

    void testreduceChopperList()
    {
        TestablePoldiAutoCorrelationCore autoCorrelationCore(m_log);

        UncertainValue pair0(2.0, 1.0);
        UncertainValue pair1(3.0, 2.0);
        UncertainValue pair2(0.0, 2.0);

        std::vector<UncertainValue> goodList;
        goodList.push_back(pair0);
        goodList.push_back(pair1);

        TS_ASSERT_DELTA(autoCorrelationCore.reduceChopperSlitList(goodList, 1.0), 3.428571428571428, 1e-6);

        std::vector<UncertainValue> badList;
        badList.push_back(pair0);
        badList.push_back(pair1);
        badList.push_back(pair2);

        TS_ASSERT_EQUALS(autoCorrelationCore.reduceChopperSlitList(badList, 1.0), 0.0);
    }

    void testFinalizeCalculation()
    {
        TestablePoldiAutoCorrelationCore core(m_log);

        MantidVec dValues = boost::assign::list_of(0.5)(0.6)(0.7)(0.8).convert_to_container<MantidVec>();
        MantidVec intensities = boost::assign::list_of(1.0)(2.0)(3.0)(4.0).convert_to_container<MantidVec>();

        DataObjects::Workspace2D_sptr output = core.finalizeCalculation(intensities, dValues);

        TS_ASSERT_EQUALS(output->getNumberHistograms(), 1);

        const MantidVec &qValues = output->readX(0);
        // qvalues should be reversed.
        TS_ASSERT_EQUALS(qValues[0], Conversions::dToQ(dValues[3]));
        TS_ASSERT_EQUALS(qValues[1], Conversions::dToQ(dValues[2]));
        TS_ASSERT_EQUALS(qValues[2], Conversions::dToQ(dValues[1]));
        TS_ASSERT_EQUALS(qValues[3], Conversions::dToQ(dValues[0]));

        const MantidVec &outputIntensities = output->readY(0);
        // intensities are not reversed, they should be reversed already.
        TS_ASSERT_EQUALS(outputIntensities[0], intensities[0]);
        TS_ASSERT_EQUALS(outputIntensities[1], intensities[1]);
        TS_ASSERT_EQUALS(outputIntensities[2], intensities[2]);
        TS_ASSERT_EQUALS(outputIntensities[3], intensities[3]);
    }

private:
    Mantid::Kernel::Logger m_log;
};


#endif /* MANTID_SINQ_POLDIAUTOCORRELATIONCORETEST_H_ */
