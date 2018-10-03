// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDEVENTS_PLUSMDEWTEST_H_
#define MANTID_MDEVENTS_PLUSMDEWTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/BoxControllerNeXusIO.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidMDAlgorithms/PlusMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
#include "MantidTestHelpers/MDAlgorithmsTestHelper.h"

#include <Poco/File.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

class PlusMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void mask_workspace(const int mask_workspace) {
    if (mask_workspace == 1) {
      FrameworkManager::Instance().exec(
          "MaskMD", 6, "Workspace", "PlusMDTest_lhs", "Dimensions",
          "Axis0,Axis1,Axis2", "Extents", "0,10,0,10,0,10");
    } else if (mask_workspace == 2) {
      FrameworkManager::Instance().exec(
          "MaskMD", 6, "Workspace", "PlusMDTest_rhs", "Dimensions",
          "Axis0,Axis1,Axis2", "Extents", "0,10,0,10,0,10");
    }
  }

  void do_test(bool lhs_file, bool rhs_file, int inPlace,
               bool deleteFile = true, int mask_ws_num = 0) {
    AnalysisDataService::Instance().clear();
    // Make two input workspaces
    MDEventWorkspace3Lean::sptr lhs =
        MDAlgorithmsTestHelper::makeFileBackedMDEW("PlusMDTest_lhs", lhs_file);
    MDEventWorkspace3Lean::sptr rhs =
        MDAlgorithmsTestHelper::makeFileBackedMDEW("PlusMDTest_rhs", rhs_file);
    std::string outWSName = "PlusMDTest_out";
    if (inPlace == 1)
      outWSName = "PlusMDTest_lhs";
    else if (inPlace == 2)
      outWSName = "PlusMDTest_rhs";

    mask_workspace(mask_ws_num);

    PlusMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("LHSWorkspace", "PlusMDTest_lhs"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("RHSWorkspace", "PlusMDTest_rhs"));
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

    if (mask_ws_num == 0) {
      TSM_ASSERT("If the workspace is file-backed, then it needs updating.",
                 ws->fileNeedsUpdating());
    }

    if (ws->isFileBacked()) {

      // Run SaveMD so as to update the file in the back
      FrameworkManager::Instance().exec("SaveMD", 4, "InputWorkspace",
                                        outWSName.c_str(), "UpdateFileBackEnd",
                                        "1");

      Mantid::API::BoxController_sptr bc = ws->getBoxController();
      std::cout << bc->getFileIO()->getFreeSpaceMap().size()
                << " entries in the free space map\n";

      auto loader = dynamic_cast<BoxControllerNeXusIO *>(bc->getFileIO());
      TS_ASSERT(loader);
      if (!loader)
        return;
      std::vector<uint64_t> freeSpaceMap;
      loader->getFreeSpaceVector(freeSpaceMap);
      uint64_t freeSpace(0);
      for (size_t i = 0; i < freeSpaceMap.size() / 2; i++) {
        freeSpace += freeSpaceMap[2 * i + 1];
      }

      ::NeXus::File *file = loader->getFile();
      // The file should have an entry of 20000 points too (with some error due
      // to the free space blocks). This means the file back-end was updated
      if (mask_ws_num == 0) {
        TS_ASSERT_EQUALS(file->getInfo().dims[0], 20000 + freeSpace);
      } else {
        TS_ASSERT_EQUALS(file->getInfo().dims[0], 10000 + freeSpace);
      }

      // Close the file so you can delete it. Otherwise the following test gets
      // confused.
      if (deleteFile) {
        std::string fileName =
            ws->getBoxController()->getFileIO()->getFileName();
        ws->clearFileBacked(false);
        Poco::File(fileName).remove();
      }
    }
    // cleanup
    if ((inPlace == 1) && rhs->isFileBacked()) {
      std::string fileName =
          rhs->getBoxController()->getFileIO()->getFileName();
      rhs->clearFileBacked(false);
      Poco::File(fileName).remove();
    }
    if ((inPlace == 2) && lhs->isFileBacked()) {
      std::string fileName =
          lhs->getBoxController()->getFileIO()->getFileName();
      lhs->clearFileBacked(false);
      Poco::File(fileName).remove();
    }
    if (ws->isFileBacked()) {
      std::string fileName = ws->getBoxController()->getFileIO()->getFileName();
      ws->clearFileBacked(false);
      Poco::File(fileName).remove();
    }
  }

  void test_mem_plus_mem() { do_test(false, false, 0); }

  void test_mem_plus_mem_inPlace() { do_test(false, false, 1); }

  void test_mem_plus_mem_inPlace_ofRHS() { do_test(false, false, 2); }

  void test_file_plus_mem() { do_test(true, false, 0); }

  void test_file_plus_mem_inPlace() { do_test(true, false, 1); }

  void test_mem_plus_file() { do_test(false, true, 0); }

  void test_mem_plus_file_inPlace() { do_test(false, true, 1); }

  void test_file_plus_file() { do_test(true, true, 0); }

  void test_file_plus_file_inPlace() { do_test(true, true, 1); }

  void test_file_plus_file_inPlace_ofRHS() { do_test(true, true, 2); }

  void test_mem_masked_plus_mem() { do_test(false, false, 0, true, 1); }

  void test_mem_masked_plus_file() { do_test(false, true, 0, true, 1); }

  void test_masked_file_plus_file_inPlace() { do_test(true, true, 1, true, 1); }

  void test_histo_histo() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "histo_B",
                                              "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 5.0, 1e-5);
  }

  void test_histo_scalar() {
    MDHistoWorkspace_sptr out;
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "scalar",
                                              "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 5.0, 1e-5);
    out = BinaryOperationMDTestHelper::doTest("PlusMD", "scalar", "histo_A",
                                              "out");
    TS_ASSERT_DELTA(out->getSignalAt(0), 5.0, 1e-5);
  }

  void test_event_scalar_fails() {
    BinaryOperationMDTestHelper::doTest("PlusMD", "event_A", "scalar", "out",
                                        false /*fails*/);
    BinaryOperationMDTestHelper::doTest("PlusMD", "scalar", "event_A", "out",
                                        false /*fails*/);
  }

  void test_event_histo_fails() {
    BinaryOperationMDTestHelper::doTest("PlusMD", "event_A", "histo_A", "out",
                                        false /*fails*/);
    BinaryOperationMDTestHelper::doTest("PlusMD", "histo_A", "event_A", "out",
                                        false /*fails*/);
  }
};

#endif /* MANTID_MDEVENTS_PLUSMDEWTEST_H_ */
