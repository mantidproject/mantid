#ifndef WORKSPACEOPOVERLOADSTEST_H_
#define WORKSPACEOPOVERLOADSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidTestHelpers/FakeObjects.h"

using namespace Mantid::API;

class WorkspaceOpOverloadsTest : public CxxTest::TestSuite {
public:
  //-------------------------------------------------------------------------------------------
  // WorkspaceHelpers tests (N.B. Operator overload tests are in the algorithms
  // that they call)
  //-------------------------------------------------------------------------------------------

  void test_commonBoundaries() {
    auto singlespectrum = boost::make_shared<WorkspaceTester>();
    singlespectrum->initialize(1, 6, 5);
    TSM_ASSERT("A single-spectrum workspace should always pass",
               WorkspaceHelpers::commonBoundaries(*singlespectrum));

    // A simple success case
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(5, 6, 5);
    TS_ASSERT(WorkspaceHelpers::commonBoundaries(*ws));
    // Changing Y & E values makes no differenc
    ws->dataY(3).assign(5, -5.0);
    ws->dataE(1).assign(5, 1.0);
    TS_ASSERT(WorkspaceHelpers::commonBoundaries(*ws));
    // Simple failure case - change a single X value slightly
    ws->dataX(2)[4] = 1.01;
    TS_ASSERT(!WorkspaceHelpers::commonBoundaries(*ws));

    // Check it fails if there are nan's or infinities, even if they match up
    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws2->initialize(2, 6, 5);
    ws2->dataX(0)[2] = std::numeric_limits<double>::infinity();
    ws2->dataX(1)[2] = std::numeric_limits<double>::infinity();
    TS_ASSERT(!WorkspaceHelpers::commonBoundaries(*ws2));
    ws2->dataX(0)[2] = std::numeric_limits<double>::quiet_NaN();
    ws2->dataX(1)[2] = std::numeric_limits<double>::quiet_NaN();
    TS_ASSERT(!WorkspaceHelpers::commonBoundaries(*ws2));

    // Check it fails if the sum is the same, but the boundaries are different
    auto ws3 = boost::make_shared<WorkspaceTester>();
    ws3->initialize(2, 3, 2);
    ws3->dataX(0)[0] = -1;
    ws3->dataX(0)[2] = 0;
    TS_ASSERT(!WorkspaceHelpers::commonBoundaries(*ws3));
  }

  void test_commmonBoundaries_negative_sum() // Added in response to bug #7391
  {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    ws->getSpectrum(0).dataX()[0] = -2.0;
    ws->getSpectrum(0).dataX()[1] = -1.0;
    ws->getSpectrum(1).dataX()[0] = -2.5;
    ws->getSpectrum(1).dataX()[1] = -1.5;

    TS_ASSERT(!WorkspaceHelpers::commonBoundaries(*ws));
  }

  void test_matchingBins() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    TSM_ASSERT("Passing it the same workspace twice had better work!",
               WorkspaceHelpers::matchingBins(*ws, *ws));

    // Different size workspaces fail of course
    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws2->initialize(3, 2, 1);
    auto ws3 = boost::make_shared<WorkspaceTester>();
    ws3->initialize(2, 3, 2);
    TSM_ASSERT("Different size workspaces should always fail",
               !WorkspaceHelpers::matchingBins(*ws, *ws2));
    TSM_ASSERT("Different size workspaces should always fail",
               !WorkspaceHelpers::matchingBins(*ws, *ws3));

    ws2->dataX(1)[0] = 99.0;
    TSM_ASSERT("First-spectrum-only check should pass even when things differ "
               "in later spectra",
               WorkspaceHelpers::matchingBins(*ws, *ws2, true));

    // Check it fails if the sum is zero but the boundaries differ, both for 1st
    // & later spectra.
    auto ws4 = boost::make_shared<WorkspaceTester>();
    ws4->initialize(2, 3, 2);
    ws4->dataX(0)[0] = -1;
    ws4->dataX(0)[1] = 0;
    auto ws5 = boost::make_shared<WorkspaceTester>();
    ws5->initialize(2, 3, 2);
    ws5->dataX(0)[0] = -1;
    ws5->dataX(0)[2] = 0;
    TS_ASSERT(!WorkspaceHelpers::matchingBins(*ws4, *ws5, true))
    auto ws6 = boost::make_shared<WorkspaceTester>();
    ws6->initialize(2, 3, 2);
    ws6->dataX(1)[0] = -1;
    ws6->dataX(1)[1] = 0;
    auto ws7 = boost::make_shared<WorkspaceTester>();
    ws7->initialize(2, 3, 2);
    ws7->dataX(1)[0] = -1;
    ws7->dataX(1)[2] = 0;
    TS_ASSERT(!WorkspaceHelpers::matchingBins(*ws6, *ws7))

    // N.B. There are known ways to fool this method, but they are considered
    // acceptable because
    // we're making a trade-off between absolute accuracy and speed.
    //  - it is possible for bins boundaries to sum to the same and be
    //  different, but this is considered
    //    unlikely and boundaries are only checked individually if the sum is
    //    zero.
    //  - for large workspaces, only 1 in 10 of the spectra are checked.
  }

  void test_matchingBins_negative_sum() // Added in response to bug #7391
  {
    auto ws1 = boost::make_shared<WorkspaceTester>();
    ws1->initialize(2, 2, 1);
    ws1->getSpectrum(1).dataX()[0] = -2.5;
    ws1->getSpectrum(1).dataX()[1] = -1.5;

    auto ws2 = boost::make_shared<WorkspaceTester>();
    ws2->initialize(2, 2, 1);
    ws2->getSpectrum(1).dataX()[0] = -2.7;
    ws2->getSpectrum(1).dataX()[1] = -1.7;

    TS_ASSERT(WorkspaceHelpers::matchingBins(*ws1, *ws2, true));
    TS_ASSERT(!WorkspaceHelpers::matchingBins(*ws1, *ws2));

    ws1->getSpectrum(0).dataX()[0] = -2.0;
    ws1->getSpectrum(0).dataX()[1] = -1.0;
    ws2->getSpectrum(0).dataX()[0] = -3.0;
    ws2->getSpectrum(0).dataX()[1] = -4.0;

    TS_ASSERT(!WorkspaceHelpers::matchingBins(*ws1, *ws2, true));
  }

  void test_sharedXData() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    // By default the X vectors are different ones
    TS_ASSERT(!WorkspaceHelpers::sharedXData(*ws));
    // Force both X spectra to point to the same underlying vector
    ws->getSpectrum(1).setX(ws->getSpectrum(0).ptrX());
    TS_ASSERT(WorkspaceHelpers::sharedXData(*ws));
  }

  void test_makeDistribution() {
    // N.B. This is also tested in the tests for the
    // Convert[To/From]Distribution algorithms.
    // Test only on tiny data here.
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    ws->dataX(0)[1] = 3.0;
    ws->dataX(1)[1] = 1.5;
    TS_ASSERT(!ws->isDistribution());

    TS_ASSERT_THROWS_NOTHING(WorkspaceHelpers::makeDistribution(ws));
    TS_ASSERT(ws->isDistribution());
    TS_ASSERT_EQUALS(ws->readX(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(0)[1], 3.0)
    TS_ASSERT_EQUALS(ws->readX(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(1)[1], 1.5)
    TS_ASSERT_EQUALS(ws->readY(0)[0], 0.5)
    TS_ASSERT_EQUALS(ws->readY(1)[0], 2.0)
    TS_ASSERT_EQUALS(ws->readE(0)[0], 0.5)
    TS_ASSERT_EQUALS(ws->readE(1)[0], 2.0)

    // Try and do it again - will do nothing
    TS_ASSERT_THROWS_NOTHING(WorkspaceHelpers::makeDistribution(ws));
    TS_ASSERT(ws->isDistribution());
    TS_ASSERT_EQUALS(ws->readX(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(0)[1], 3.0)
    TS_ASSERT_EQUALS(ws->readX(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(1)[1], 1.5)
    TS_ASSERT_EQUALS(ws->readY(0)[0], 0.5)
    TS_ASSERT_EQUALS(ws->readY(1)[0], 2.0)
    TS_ASSERT_EQUALS(ws->readE(0)[0], 0.5)
    TS_ASSERT_EQUALS(ws->readE(1)[0], 2.0)

    // Now reverse the operation
    TS_ASSERT_THROWS_NOTHING(WorkspaceHelpers::makeDistribution(ws, false));
    TS_ASSERT(!ws->isDistribution());
    TS_ASSERT_EQUALS(ws->readX(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(0)[1], 3.0)
    TS_ASSERT_EQUALS(ws->readX(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(1)[1], 1.5)
    TS_ASSERT_EQUALS(ws->readY(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readY(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readE(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readE(1)[0], 1.0)

    // Try and do it again - will do nothing
    TS_ASSERT_THROWS_NOTHING(WorkspaceHelpers::makeDistribution(ws, false));
    TS_ASSERT(!ws->isDistribution());
    TS_ASSERT_EQUALS(ws->readX(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(0)[1], 3.0)
    TS_ASSERT_EQUALS(ws->readX(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readX(1)[1], 1.5)
    TS_ASSERT_EQUALS(ws->readY(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readY(1)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readE(0)[0], 1.0)
    TS_ASSERT_EQUALS(ws->readE(1)[0], 1.0)
  }

  void test_makeDistribution_fails_for_point_data() {
    auto ws = boost::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 2);
    TS_ASSERT(!ws->isDistribution());

    TS_ASSERT_THROWS(WorkspaceHelpers::makeDistribution(ws),
                     std::runtime_error);
  }
};

#endif
