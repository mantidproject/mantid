// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidFrameworkTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidMDAlgorithms/DivideMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

/** Note: More detailed tests for the underlying
 * operations are in BinaryOperationMDTest and
 * MDHistoWorkspaceTest.
 *
 */
class DivideMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    DivideMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_histo_histo() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("DivideMD", "histo_A", "histo_B", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 2. / 3., 1e-5);
  }

  void test_histo_scalar() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("DivideMD", "histo_A", "scalar", "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 2. / 3., 1e-5);
    BinaryOperationMDTestHelper::doTest("DivideMD", "scalar", "histo_A", "out", false /*fails*/);
  }

  void test_event_event_or_histo_fails() {
    BinaryOperationMDTestHelper::doTest("DivideMD", "event_A", "histo_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("DivideMD", "histo_A", "event_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("DivideMD", "event_A", "event_A", "out", false /*fails*/);
    BinaryOperationMDTestHelper::doTest("DivideMD", "scalar", "event_A", "out", false /*fails*/);
  }

  /** Get a MDEventWorkspace and check that all events have the given
   * signal/error */
  void checkMDEWSignal(const std::string &wsName, signal_t expectedSignal, signal_t expectedError) {
    IMDEventWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<IMDEventWorkspace>(wsName);
    TS_ASSERT(ws);
    if (!ws)
      return;
    auto it = ws->createIterator(nullptr);
    do {
      TS_ASSERT_EQUALS(it->getNumEvents(), 1);
      TS_ASSERT_DELTA(it->getInnerSignal(0), expectedSignal, 1e-5);
      TS_ASSERT_DELTA(it->getInnerError(0), expectedError, 1e-5);
    } while (it->next());
  }

  /// Divide events by a scalar
  void test_event_scalar() {
    BinaryOperationMDTestHelper::doTest("DivideMD", "event_A", "scalar", "out");
    checkMDEWSignal("out", 1. / 3., sqrt(12. / 81.));
  }
};
