#ifndef MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_
#define MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_

#include <cxxtest/TestSuite.h>
#include <boost/assign.hpp>

#include "MantidSINQ/PoldiUtilities/PoldiResidualCorrelationCore.h"

#include "MantidSINQ/PoldiUtilities/PoldiMockInstrumentHelpers.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;

class PoldiResidualCorrelationCoreTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiResidualCorrelationCoreTest *createSuite() { return new PoldiResidualCorrelationCoreTest(); }
    static void destroySuite( PoldiResidualCorrelationCoreTest *suite ) { delete suite; }

    PoldiResidualCorrelationCoreTest() : m_log("PoldiResidualCorrelationCoreTest")
    {

    }

    void testGetSetWeight()
    {
        PoldiResidualCorrelationCore core(m_log);

        TS_ASSERT_EQUALS(core.getWeight(), 0.0);
        TS_ASSERT_THROWS_NOTHING(core.setWeight(323.0));
        TS_ASSERT_EQUALS(core.getWeight(), 323.0);
    }

    void testGetNormCounts()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        // test data with all 0s except (0, 0) - it's -1.0
        Mantid::DataObjects::Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        testWorkspace->dataY(0)[0] = -1.0;

        core.setNormCountData(testWorkspace);

        // The methods returns the absolute, so all returned values should be positive (or 0).
        TS_ASSERT_EQUALS(core.getNormCounts(0, 0), 1.0);
        TS_ASSERT_EQUALS(core.getNormCounts(0, 1), 0.0);
        TS_ASSERT_EQUALS(core.getNormCounts(1, 0), 1.0);
        TS_ASSERT_EQUALS(core.getNormCounts(1, 1), 1.0);

        // If a weight != 0 is set, the values change (+weight)
        core.setWeight(23.0);
        TS_ASSERT_EQUALS(core.getNormCounts(0, 0), 24.0);
        TS_ASSERT_EQUALS(core.getNormCounts(0, 1), 23.0);
        TS_ASSERT_EQUALS(core.getNormCounts(1, 0), 24.0);
        TS_ASSERT_EQUALS(core.getNormCounts(1, 1), 24.0);
    }

    void testAddToCountData()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        Mantid::DataObjects::Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        core.setCountData(testWorkspace);

        TS_ASSERT_EQUALS(testWorkspace->dataY(0)[0], 0.0);
        core.addToCountData(0, 0, 23.0);
        TS_ASSERT_EQUALS(testWorkspace->dataY(0)[0], 23.0);
        core.addToCountData(0, 0, 23.0);
        TS_ASSERT_EQUALS(testWorkspace->dataY(0)[0], 46.0);
    }

    void testCalculateCorrelationBackground()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        TS_ASSERT_EQUALS(core.calculateCorrelationBackground(20.0, 1.0), 20.0);
        TS_ASSERT_EQUALS(core.calculateCorrelationBackground(20.0, 3.0), 20.0);
        TS_ASSERT_EQUALS(core.calculateCorrelationBackground(20.0, -2.0), 20.0);
    }

    void testReduceChopperList()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        UncertainValue pair0(2.0, 1.0);
        UncertainValue pair1(3.0, 2.0);

        std::vector<UncertainValue> goodList;
        goodList.push_back(pair0);
        goodList.push_back(pair1);

        TS_ASSERT_EQUALS(core.reduceChopperSlitList(goodList, 1.0), 3.0625);
    }

    void testCorrectCountData()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        Mantid::DataObjects::Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        core.setCountData(testWorkspace);
        core.m_timeBinCount = 2;
        core.m_detectorElements = boost::assign::list_of(0)(1).convert_to_container<std::vector<int> >();
        core.m_indices = boost::assign::list_of(0)(1).convert_to_container<std::vector<int> >();

        // sum of counts = 2, number of cells = 4, that means ratio = 0.5, which is subtracted from all counts.
        core.correctCountData();

        TS_ASSERT_EQUALS(testWorkspace->readY(0)[0], -0.5);
        TS_ASSERT_EQUALS(testWorkspace->readY(0)[1], -0.5);
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[0], 0.5);
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[1], 0.5);
    }

    void testCalculateAverage()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        std::vector<double> numbers = boost::assign::list_of(1.0)(2.0)(3.0)(4.0)(5.0)(6.0).convert_to_container<std::vector<double> >();
        TS_ASSERT_EQUALS(core.calculateAverage(numbers), 3.5);

        std::vector<double> empty;
        TS_ASSERT_THROWS(core.calculateAverage(empty), std::runtime_error);
    }

    void testCalculateAverageDeviationFromValue()
    {
        TestablePoldiResidualCorrelationCore core(m_log);

        std::vector<double> numbers = boost::assign::list_of(1.0)(2.0)(3.0)(4.0)(5.0)(6.0).convert_to_container<std::vector<double> >();
        TS_ASSERT_EQUALS(core.calculateAverageDeviationFromValue(numbers, 3.5), 1.5);

        std::vector<double> empty;
        TS_ASSERT_THROWS(core.calculateAverageDeviationFromValue(empty, 3.5), std::runtime_error);
    }

private:
    Mantid::Kernel::Logger m_log;

    class TestablePoldiResidualCorrelationCore : public PoldiResidualCorrelationCore
    {
        friend class PoldiResidualCorrelationCoreTest;
        TestablePoldiResidualCorrelationCore(Mantid::Kernel::Logger &logger)
            : PoldiResidualCorrelationCore(logger)
        {
        }
    };
};


#endif /* MANTID_SINQ_POLDIRESIDUALCORRELATIONCORETEST_H_ */
