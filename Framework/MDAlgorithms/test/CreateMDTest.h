// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MDALGORITHMS_CREATEMDTEST_H_
#define MANTID_MDALGORITHMS_CREATEMDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IMDEventWorkspace_fwd.h"
#include "MantidMDAlgorithms/CreateMD.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <Poco/File.h>
#include <stdexcept>

using Mantid::MDAlgorithms::CreateMD;

class CreateMDTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CreateMDTest *createSuite() { return new CreateMDTest(); }
  static void destroySuite(CreateMDTest *suite) { delete suite; }

  void createTwoTestWorkspaces() {
    auto sim_alg = Mantid::API::AlgorithmManager::Instance().create(
        "CreateSimulationWorkspace");
    sim_alg->initialize();
    sim_alg->setPropertyValue("Instrument", "MAR");
    sim_alg->setPropertyValue("BinParams", "-3,1,3");
    sim_alg->setPropertyValue("UnitX", "DeltaE");
    sim_alg->setPropertyValue("OutputWorkspace", "data_source_1");
    sim_alg->execute();

    sim_alg->setPropertyValue("OutputWorkspace", "data_source_2");
    sim_alg->execute();

    auto log_alg =
        Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
    log_alg->initialize();
    log_alg->setProperty("Workspace", "data_source_1");
    log_alg->setPropertyValue("LogName", "Ei");
    log_alg->setPropertyValue("LogText", "3.0");
    log_alg->setPropertyValue("LogType", "Number");
    log_alg->execute();

    log_alg->setProperty("Workspace", "data_source_2");
    log_alg->execute();
  }

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
                     const std::invalid_argument &);
  }

  void test_psi_right_size() {
    auto sample_ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
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

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_gl_right_size() {
    auto sample_ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
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

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_gs_right_size() {
    auto sample_ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
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

    TS_ASSERT_THROWS(alg.execute(), const std::runtime_error &);

    // Clean up
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_sample");
  }

  void test_execute_single_workspace() {
    auto sim_alg = Mantid::API::AlgorithmManager::Instance().create(
        "CreateSimulationWorkspace");
    sim_alg->initialize();
    sim_alg->setPropertyValue("Instrument", "MAR");
    sim_alg->setPropertyValue("BinParams", "-3,1,3");
    sim_alg->setPropertyValue("UnitX", "DeltaE");
    sim_alg->setPropertyValue("OutputWorkspace", "data_source_1");
    sim_alg->execute();

    auto log_alg =
        Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
    log_alg->initialize();
    log_alg->setProperty("Workspace", "data_source_1");
    log_alg->setPropertyValue("LogName", "Ei");
    log_alg->setPropertyValue("LogText", "3.0");
    log_alg->setPropertyValue("LogType", "Number");
    log_alg->execute();

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "data_source_1");
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
    createTwoTestWorkspaces();

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "data_source_1,data_source_2");
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

  void test_execute_filebackend() {
    createTwoTestWorkspaces();

    CreateMD alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace");
    alg.setPropertyValue("DataSources", "data_source_1,data_source_2");
    alg.setPropertyValue("Alatt", "1,1,1");
    alg.setPropertyValue("Angdeg", "90,90,90");
    alg.setPropertyValue("Efix", "12.0,13.0");
    alg.setPropertyValue("u", "1,0,0");
    alg.setPropertyValue("v", "0,1,0");

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "__CreateMDTest_mdworkspace"));

    alg.setPropertyValue("OutputWorkspace", "__CreateMDTest_mdworkspace_fb");
    alg.setProperty("Filename", "CreateMDTest_filebackend.nxs");
    alg.setProperty("FileBackEnd", true);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(
        "__CreateMDTest_mdworkspace_fb"));

    auto compare_alg =
        Mantid::API::AlgorithmManager::Instance().create("CompareMDWorkspaces");
    compare_alg->initialize();
    compare_alg->setProperty("Workspace1", "__CreateMDTest_mdworkspace_fb");
    compare_alg->setProperty("Workspace2", "__CreateMDTest_mdworkspace");
    compare_alg->setProperty("CheckEvents", false);
    compare_alg->setProperty("IgnoreBoxID", true);
    TSM_ASSERT_THROWS_NOTHING(
        "Workspaces with and without filebackend should be the same",
        compare_alg->execute(););

    std::string filename = alg.getPropertyValue("Filename");
    TSM_ASSERT("File was indeed created", Poco::File(filename).exists());

    // Clean up workspaces
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_mdworkspace");
    Mantid::API::AnalysisDataService::Instance().remove(
        "__CreateMDTest_mdworkspace_fb");

    if (Poco::File(filename).exists())
      Poco::File(filename).remove();
  }
};

#endif /* MANTID_MDALGORITHMS_CREATEMDTEST_H_ */
