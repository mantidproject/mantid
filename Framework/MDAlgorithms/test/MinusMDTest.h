#ifndef MANTID_MDALGORITHMS_MINUSMDTEST_H_
#define MANTID_MDALGORITHMS_MINUSMDTEST_H_

#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidMDAlgorithms/MinusMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidAPI/FrameworkManager.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class MinusMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    MinusMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void mask_workspace(const int mask_workspace) {
    if (mask_workspace == 1) {
      FrameworkManager::Instance().exec(
          "MaskMD", 6, "Workspace", "MinusMDTest_lhs", "Dimensions",
          "Axis0,Axis1,Axis2", "Extents", "0,10,0,10,0,10");
    } else if (mask_workspace == 2) {
      FrameworkManager::Instance().exec(
          "MaskMD", 6, "Workspace", "MinusMDTest_rhs", "Dimensions",
          "Axis0,Axis1,Axis2", "Extents", "0,10,0,10,0,10");
    }
  }

  void test_histo_histo() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "histo_B",
                                              "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), -1.0, 1e-5);
  }

  void test_histo_scalar() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("MinusMD", "histo_A", "scalar",
                                              "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), -1.0, 1e-5);
    BinaryOperationMDTestHelper::doTest("MinusMD", "scalar", "histo_A", "out",
                                        false /*fails*/);
  }

  void do_test(bool lhs_file, bool rhs_file, int inPlace, int mask_ws_num = 0) {
    AnalysisDataService::Instance().clear();
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs =
        MDAlgorithmsTestHelper::makeFileBackedMDEW("MinusMDTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs =
        MDAlgorithmsTestHelper::makeFileBackedMDEW("MinusMDTest_rhs", rhs_file);
    std::string outWSName = "MinusMDTest_out";
    if (inPlace == 1)
      outWSName = "MinusMDTest_lhs";
    else if (inPlace == 2)
      outWSName = "MinusMDTest_rhs";

    mask_workspace(mask_ws_num);

    MinusMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("LHSWorkspace", "MinusMDTest_lhs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("RHSWorkspace", "MinusMDTest_rhs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", outWSName));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from data service.
    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            outWSName));
    TS_ASSERT(ws);
    if (!ws)
      return;

    // Check the results
    if (inPlace == 1) {
      TS_ASSERT(ws == lhs);
    } else if (inPlace == 2) {
      TS_ASSERT(ws == rhs);
    }

    if ((lhs_file || rhs_file) && !((inPlace == 1) && !lhs_file && rhs_file)) {
      TSM_ASSERT(
          "If either input WS is file backed, then the output should be too.",
          ws->getBoxController()->isFileBacked());
    }
    if (mask_ws_num == 0) {
      TS_ASSERT_EQUALS(ws->getNPoints(), 20000);
    } else {
      TS_ASSERT_EQUALS(ws->getNPoints(), 10000);
    }

    auto it = ws->createIterator();
    if (mask_ws_num == 0) {
      while (it->next()) {
        // Signal of all boxes is zero since they got subtracted
        TS_ASSERT_DELTA(it->getSignal(), 0.0, 1e-5);
        // But errors are not zero, since they get summed
        TS_ASSERT_LESS_THAN(0, it->getError());
      }
    } else if (mask_ws_num == 1) {
      while (it->next()) {
        // Signal of all boxes is -ve as events were subtracted from masked
        TS_ASSERT_LESS_THAN(it->getSignal(), 0.0);
        // But errors are not zero, since they get summed
        // TS_ASSERT_LESS_THAN(0, it->getError());
      }
    } else if (mask_ws_num == 2) {
      while (it->next()) {
        // Signal of all boxes is +ve as masked workspace subtracted
        // (subtract 0)
        TS_ASSERT_LESS_THAN(0.0, it->getSignal());
        // But errors are not zero, since they get summed
        // TS_ASSERT_LESS_THAN(0, it->getError());
      }
    }

    if (mask_ws_num == 0) {
      TSM_ASSERT("If the workspace is file-backed, then it needs updating.",
                 ws->fileNeedsUpdating());
    }
    // cleanup
    std::string realFile;
    if ((inPlace == 1) && rhs->isFileBacked()) {
      realFile = rhs->getBoxController()->getFileIO()->getFileName();
      rhs->clearFileBacked(false);
      MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    if ((inPlace == 2) && lhs->isFileBacked()) {
      realFile = lhs->getBoxController()->getFileIO()->getFileName();
      lhs->clearFileBacked(false);
      MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    if (ws->isFileBacked()) {
      realFile = ws->getBoxController()->getFileIO()->getFileName();
      ws->clearFileBacked(false);
      MDEventsTestHelper::checkAndDeleteFile(realFile);
    }
    AnalysisDataService::Instance().remove(outWSName);
  }

  void test_mem_minus_mem() { do_test(false, false, 0); }

  void test_mem_minus_mem_inPlace() { do_test(false, false, 1); }

  void test_file_minus_mem() { do_test(true, false, 0); }

  void test_file_minus_mem_inPlace() { do_test(true, false, 1); }

  void test_mem_minus_file_inPlace() { do_test(false, true, 1); }

  void test_file_minus_file() { do_test(true, true, 0); }

  void test_file_minus_file_inPlace() { do_test(true, true, 1); }

  void test_mem_masked_plus_mem() { do_test(false, false, 0, 1); }

  void test_masked_file_plus_file_inPlace() { do_test(true, true, 1, 1); }
};

#endif /* MANTID_MDALGORITHMS_MINUSMDTEST_H_ */
