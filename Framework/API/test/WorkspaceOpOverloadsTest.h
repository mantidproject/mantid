// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidFrameworkTestHelpers/FakeObjects.h"

using namespace Mantid::API;

class WorkspaceOpOverloadsTest : public CxxTest::TestSuite {
public:
  //-------------------------------------------------------------------------------------------
  // WorkspaceHelpers tests (N.B. Operator overload tests are in the algorithms
  // that they call)
  //-------------------------------------------------------------------------------------------

  void test_matchingBins() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    TSM_ASSERT("Passing it the same workspace twice had better work!", WorkspaceHelpers::matchingBins(ws, ws));

    // Different size workspaces fail of course
    auto ws2 = std::make_shared<WorkspaceTester>();
    ws2->initialize(3, 2, 1);
    auto ws3 = std::make_shared<WorkspaceTester>();
    ws3->initialize(2, 3, 2);
    TSM_ASSERT("Different size workspaces should always fail", !WorkspaceHelpers::matchingBins(ws, ws2));
    TSM_ASSERT("Different size workspaces should always fail", !WorkspaceHelpers::matchingBins(ws, ws3));

    ws2->dataX(1)[0] = 99.0;
    TSM_ASSERT("First-spectrum-only check should pass even when things differ "
               "in later spectra",
               WorkspaceHelpers::matchingBins(ws, ws2, true));

    // Check it fails if the sum is zero but the boundaries differ, both for 1st
    // & later spectra.
    auto ws4 = std::make_shared<WorkspaceTester>();
    ws4->initialize(2, 3, 2);
    ws4->dataX(0)[0] = -1;
    ws4->dataX(0)[1] = 0;
    auto ws5 = std::make_shared<WorkspaceTester>();
    ws5->initialize(2, 3, 2);
    ws5->dataX(0)[0] = -1;
    ws5->dataX(0)[2] = 0;
    TS_ASSERT(!WorkspaceHelpers::matchingBins(ws4, ws5, true))
    auto ws6 = std::make_shared<WorkspaceTester>();
    ws6->initialize(2, 3, 2);
    ws6->dataX(1)[0] = -1;
    ws6->dataX(1)[1] = 0;
    auto ws7 = std::make_shared<WorkspaceTester>();
    ws7->initialize(2, 3, 2);
    ws7->dataX(1)[0] = -1;
    ws7->dataX(1)[2] = 0;
    TS_ASSERT(!WorkspaceHelpers::matchingBins(ws6, ws7))

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
    auto ws1 = std::make_shared<WorkspaceTester>();
    ws1->initialize(2, 2, 1);
    ws1->getSpectrum(1).dataX()[0] = -2.5;
    ws1->getSpectrum(1).dataX()[1] = -1.5;

    auto ws2 = std::make_shared<WorkspaceTester>();
    ws2->initialize(2, 2, 1);
    ws2->getSpectrum(1).dataX()[0] = -2.7;
    ws2->getSpectrum(1).dataX()[1] = -1.7;

    TS_ASSERT(WorkspaceHelpers::matchingBins(ws1, ws2, true));
    TS_ASSERT(!WorkspaceHelpers::matchingBins(ws1, ws2));

    ws1->getSpectrum(0).dataX()[0] = -2.0;
    ws1->getSpectrum(0).dataX()[1] = -1.0;
    ws2->getSpectrum(0).dataX()[0] = -3.0;
    ws2->getSpectrum(0).dataX()[1] = -4.0;

    TS_ASSERT(!WorkspaceHelpers::matchingBins(ws1, ws2, true));
  }

  void test_sharedXData() {
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 1);
    // By default the X vectors are different ones
    TS_ASSERT(!WorkspaceHelpers::sharedXData(ws));
    // Force both X spectra to point to the same underlying vector
    ws->getSpectrum(1).setX(ws->getSpectrum(0).ptrX());
    TS_ASSERT(WorkspaceHelpers::sharedXData(ws));
  }

  void test_makeDistribution() {
    // N.B. This is also tested in the tests for the
    // Convert[To/From]Distribution algorithms.
    // Test only on tiny data here.
    auto ws = std::make_shared<WorkspaceTester>();
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
    auto ws = std::make_shared<WorkspaceTester>();
    ws->initialize(2, 2, 2);
    TS_ASSERT(!ws->isDistribution());

    TS_ASSERT_THROWS(WorkspaceHelpers::makeDistribution(ws), const std::runtime_error &);
  }
};
