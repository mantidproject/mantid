#ifndef MANTID_SINQ_POLDIANALYSERESIDUALSTEST_H_
#define MANTID_SINQ_POLDIANALYSERESIDUALSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidSINQ/PoldiAnalyseResiduals.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
