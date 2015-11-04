#ifndef MANTID_MDALGORITHMS_CREATEMDTEST_H_
#define MANTID_MDALGORITHMS_CREATEMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidMDAlgorithms/CreateMD.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <stdexcept>
#include <MantidAPI/IMDEventWorkspace_fwd.h>

using Mantid::MDAlgorithms::CreateMD;

class CreateMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateMDTest *createSuite() { return new CreateMDTest(); }
  static void destroySuite(CreateMDTest *suite) { delete suite; }

  void test_init() {
    CreateMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_must_have_at_least_one_input_workspace() {
    CreateMD alg;
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS(alg.setPropertyValue("DataSources", ""),
                     std::invalid_argument);
  }

  void test_psi_right_size() {
    auto sample_ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    Mantid::API::AnalysisDataService::Instance().add("__CreateMDTest_sample",
                                                     sample_ws);

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "__CreateMDTest_sample");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0");
    alg.setPropertyValue("u", "0,0,1");
    alg.setPropertyValue("v", "1,0,0");
    alg.setPropertyValue("Psi", "0,0,0"); // Too large
    alg.setPropertyValue("Gl", "0");      // Right size
    alg.setPropertyValue("Gs", "0");      // Right size

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_gl_right_size() {
    auto sample_ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    Mantid::API::AnalysisDataService::Instance().add("__CreateMDTest_sample",
                                                     sample_ws);

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "__CreateMDTest_sample");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0");
    alg.setPropertyValue("u", "0,0,1");
    alg.setPropertyValue("v", "1,0,0");
    alg.setPropertyValue("Psi", "0");  // Right size
    alg.setPropertyValue("Gl", "0,0"); // Too large
    alg.setPropertyValue("Gs", "0");   // Right size

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_gs_right_size() {
    auto sample_ws = WorkspaceCreationHelper::Create2DWorkspace(1, 1);
    Mantid::API::AnalysisDataService::Instance().add("__CreateMDTest_sample",
                                                     sample_ws);

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "__CreateMDTest_sample");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0");
    alg.setPropertyValue("u", "0,0,1");
    alg.setPropertyValue("v", "1,0,0");
    alg.setPropertyValue("Psi", "0");  // Right size
    alg.setPropertyValue("Gl", "0");   // Right size
    alg.setPropertyValue("Gs", "0,0"); // Too large

    TS_ASSERT_THROWS(alg.execute(), std::runtime_error);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_execute_single_workspace() {
    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "CNCS_7860_event.nxs");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0");
    alg.setPropertyValue("u", "1,0,0");
    alg.setPropertyValue("v", "0,1,0");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "__CreateMDTest_mdworkspace"));

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_mdworkspace");
  }

  void test_execute_multi_file() {
    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources",
                         "CNCS_7860_event.nxs,CNCS_7860_event.nxs");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0,13.0");
    alg.setPropertyValue("u", "1,0,0");
    alg.setPropertyValue("v", "0,1,0");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "__CreateMDTest_mdworkspace"));

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_mdworkspace");
  }
};

#endif /* MANTID_MDALGORITHMS_CREATEMDTEST_H_ */