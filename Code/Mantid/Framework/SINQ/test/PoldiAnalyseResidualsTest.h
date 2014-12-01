#ifndef MANTID_SINQ_POLDIANALYSERESIDUALSTEST_H_
#define MANTID_SINQ_POLDIANALYSERESIDUALSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiAnalyseResiduals.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"

using namespace Mantid::Poldi;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

class PoldiAnalyseResidualsTest : public CxxTest::TestSuite
{
public:
    // This pair of boilerplate methods prevent the suite being created statically
    // This means the constructor isn't called when running other tests
    static PoldiAnalyseResidualsTest *createSuite() { return new PoldiAnalyseResidualsTest(); }
    static void destroySuite( PoldiAnalyseResidualsTest *suite ) { delete suite; }

    PoldiAnalyseResidualsTest()
    {
        FrameworkManager::Instance();
    }


    void test_Init()
    {
        PoldiAnalyseResiduals alg;
        TS_ASSERT_THROWS_NOTHING( alg.initialize() )
                TS_ASSERT( alg.isInitialized() )
    }

    void testSumCounts()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        TS_ASSERT_EQUALS(alg.sumCounts(testWorkspace, std::vector<int>(1, 1)), 2.0);
        TS_ASSERT_EQUALS(alg.sumCounts(testWorkspace, std::vector<int>(1, 0)), 0.0);

        TS_ASSERT_THROWS_ANYTHING(alg.sumCounts(testWorkspace, std::vector<int>(1, 3)));
    }

    void testNumberOfPoints()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        TS_ASSERT_EQUALS(alg.numberOfPoints(testWorkspace, std::vector<int>(1, 1)), 2);
        TS_ASSERT_EQUALS(alg.numberOfPoints(testWorkspace, std::vector<int>(1, 0)), 2);

        TS_ASSERT_THROWS_ANYTHING(alg.sumCounts(testWorkspace, std::vector<int>(1, 3)));
    }

    void testAddValue()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        TS_ASSERT_THROWS_NOTHING(alg.addValue(testWorkspace, 3.0, std::vector<int>(1, 1)));
        TS_ASSERT_THROWS_NOTHING(alg.addValue(testWorkspace, 3.0, std::vector<int>(1, 0)));

        TS_ASSERT_EQUALS(testWorkspace->readY(0)[0], 3.0);
        TS_ASSERT_EQUALS(testWorkspace->readY(0)[1], 3.0);
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[0], 4.0);
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[1], 4.0);

        TS_ASSERT_THROWS_ANYTHING(alg.addValue(testWorkspace, 3.0, std::vector<int>(1, 3)));
    }

    void testCalculateResidualWorkspace()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr measured = WorkspaceCreationHelper::Create2DWorkspace123(2, 2);
        Workspace2D_sptr calculated = WorkspaceCreationHelper::Create2DWorkspace154(2, 2);

        TS_ASSERT_THROWS_NOTHING(alg.calculateResidualWorkspace(measured, calculated));
        Workspace2D_sptr residuals = alg.calculateResidualWorkspace(measured, calculated);
        TS_ASSERT_EQUALS(residuals->readY(0)[0], -3.0);
        TS_ASSERT_EQUALS(residuals->readY(0)[1], -3.0);
        TS_ASSERT_EQUALS(residuals->readY(1)[0], -3.0);
        TS_ASSERT_EQUALS(residuals->readY(1)[1], -3.0);

        TS_ASSERT_THROWS_NOTHING(alg.calculateResidualWorkspace(calculated, measured));
        residuals = alg.calculateResidualWorkspace(calculated, measured);
        TS_ASSERT_EQUALS(residuals->readY(0)[0], 3.0);
        TS_ASSERT_EQUALS(residuals->readY(0)[1], 3.0);
        TS_ASSERT_EQUALS(residuals->readY(1)[0], 3.0);
        TS_ASSERT_EQUALS(residuals->readY(1)[1], 3.0);
    }

    void testNormalizeResiduals()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspace123(2, 2);
        TS_ASSERT_THROWS_NOTHING(alg.normalizeResiduals(testWorkspace, std::vector<int>(1, 1)));

        // nothing happens here
        TS_ASSERT_EQUALS(testWorkspace->readY(0)[0], 2.0);
        TS_ASSERT_EQUALS(testWorkspace->readY(0)[1], 2.0);

        // but here, because 1 is a valid workspace index
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[0], 0.0);
        TS_ASSERT_EQUALS(testWorkspace->readY(1)[1], 0.0);
    }

    void testRelativeCountChange()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr testWorkspace = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        TS_ASSERT_EQUALS(alg.relativeCountChange(testWorkspace, 10.0), 0.0);

        alg.addValue(testWorkspace, 10.0, std::vector<int>(1, 0));
        // (sum of dataY(0)) / 40 = 20 / 40 = 0.5 = 50%
        TS_ASSERT_EQUALS(alg.relativeCountChange(testWorkspace, 40.0), 50.0);
    }

    void testAddWorkspaces()
    {
        TestablePoldiAnalyseResiduals alg;

        Workspace2D_sptr lhs = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);
        Workspace2D_sptr rhs = WorkspaceCreationHelper::Create2DWorkspaceWhereYIsWorkspaceIndex(2, 2);

        Workspace2D_sptr sum = alg.addWorkspaces(lhs, rhs);

        TS_ASSERT_EQUALS(sum->readY(0)[0], 0.0);
        TS_ASSERT_EQUALS(sum->readY(0)[1], 0.0);
        TS_ASSERT_EQUALS(sum->readY(1)[0], 2.0);
        TS_ASSERT_EQUALS(sum->readY(1)[1], 2.0);
    }

    void testRelativeChangeIsLargerThanLimit()
    {
        TestablePoldiAnalyseResiduals alg;
        alg.initialize();
        alg.setProperty("MaxRelativeChange", 1.0);

        TS_ASSERT(alg.relativeChangeIsLargerThanLimit(20.0));
        TS_ASSERT(alg.relativeChangeIsLargerThanLimit(1.1));
        TS_ASSERT(alg.relativeChangeIsLargerThanLimit(2.0));

        TS_ASSERT(!alg.relativeChangeIsLargerThanLimit(0.5));
        TS_ASSERT(!alg.relativeChangeIsLargerThanLimit(-0.5));
    }

    void testIterationLimitReached()
    {
        TestablePoldiAnalyseResiduals alg;
        alg.initialize();
        alg.setProperty("MaxIterations", 10);

        TS_ASSERT(!alg.iterationLimitReached(1));
        TS_ASSERT(!alg.iterationLimitReached(9));
        TS_ASSERT(alg.iterationLimitReached(10));
        TS_ASSERT(alg.iterationLimitReached(11));

        alg.setProperty("MaxIterations", 0);

        TS_ASSERT(!alg.iterationLimitReached(1));
        TS_ASSERT(!alg.iterationLimitReached(9));
        TS_ASSERT(!alg.iterationLimitReached(10));
        TS_ASSERT(!alg.iterationLimitReached(11));
        TS_ASSERT(!alg.iterationLimitReached(1100));
    }

    void testNextIterationAllowed()
    {
        TestablePoldiAnalyseResiduals alg;
        alg.initialize();
        alg.setProperty("MaxRelativeChange", 1.0);
        alg.setProperty("MaxIterations", 10);

        TS_ASSERT(alg.nextIterationAllowed(1, 23.0));
        TS_ASSERT(alg.nextIterationAllowed(9, 1.1));
        TS_ASSERT(!alg.nextIterationAllowed(9, 0.5));
        TS_ASSERT(!alg.nextIterationAllowed(10, 23.0));
        TS_ASSERT(!alg.nextIterationAllowed(10, 0.5));
    }

private:
    class TestablePoldiAnalyseResiduals : public PoldiAnalyseResiduals
    {
        friend class PoldiAnalyseResidualsTest;

    public:
        TestablePoldiAnalyseResiduals() :
            PoldiAnalyseResiduals()
        { }
        ~TestablePoldiAnalyseResiduals() { }
    };


};


#endif /* MANTID_SINQ_POLDIANALYSERESIDUALSTEST_H_ */
