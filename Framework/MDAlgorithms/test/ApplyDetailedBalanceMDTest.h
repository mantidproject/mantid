// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataObjects/MDEventWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidMDAlgorithms/ApplyDetailedBalanceMD.h"
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/MergeMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class ApplyDetailedBalanceMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    ApplyDetailedBalanceMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //---------------------------------------------------------------------------------------------
  /**
   * @brief Test apply detailed balance to 1 run
   */
  void test_1run() {
    // Check whether the MD to test does exist
    auto singlemd = Mantid::API::AnalysisDataService::Instance().retrieve(mMDWorkspace1Name);
    TS_ASSERT(singlemd);

    // specify the output
    std::string outputname("DetailedBalanceSingleQ3Test");

    // Calculate detailed balance for the single MDEventWorkspace
    ApplyDetailedBalanceMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mMDWorkspace1Name);
    alg.setProperty("Temperature", "SampleTemp");
    alg.setProperty("OutputWorkspace", outputname);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Verify
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputname));

    bool equals = compareMDEvents(outputname, GoldDetailedBalanceSingleWSName);
    TS_ASSERT(equals);

    // Clean up
    cleanWorkspace(outputname, true);
  }

  /**
   * @brief Test applying detailed balance to 2 merged runs
   */
  void test_merged_runs() {
    auto mergedmd = Mantid::API::AnalysisDataService::Instance().retrieve(mMergedWorkspaceName);
    TS_ASSERT(mergedmd);

    // specify the output
    std::string outputname("DetailedBalanceMergedQ3Test");

    ApplyDetailedBalanceMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mMergedWorkspaceName);
    alg.setProperty("Temperature", "SampleTemp");
    alg.setProperty("OutputWorkspace", outputname);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Verify
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputname));

    bool equals = compareMDEvents(outputname, gold_detail_balanced_merged_name, false);
    TS_ASSERT(equals);

    // clean up
    cleanWorkspace(outputname, true);
  }

  /**
   * @brief Test input workspace with |Q| and without temperature
   */
  void test_q1d_run() {
    auto q1dmd = Mantid::API::AnalysisDataService::Instance().retrieve(mMDWorkspaceQ1Dname);
    TS_ASSERT(q1dmd);

    ApplyDetailedBalanceMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mMDWorkspaceQ1Dname);
    alg.setProperty("Temperature", "SampleTemp");
    alg.setProperty("OutputWorkspace", "OutputDetaiedBalanceQ1D");

    // Expect to fail due to missing temperature sample log
    TS_ASSERT_THROWS_ANYTHING(alg.execute());

    // Set temperature explicitly
    alg.setProperty("Temperature", "1.2345");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Check existence and clean
    cleanWorkspace("OutputDetaiedBalanceQ1D", true);
  }

  /**
   * @brief Clean workspace from ADS if it does exist
   */
  void cleanWorkspace(const std::string &wsname, bool assert_existence) {
    bool ws_exist = Mantid::API::AnalysisDataService::Instance().doesExist(wsname);
    // assert existence
    if (assert_existence)
      TS_ASSERT(ws_exist);
    // clean from ADS
    if (ws_exist)
      Mantid::API::AnalysisDataService::Instance().remove(wsname);
  }

  void setUp() override {
    // Define workspace names
    mEventWSName = "DetailedBalanceRawEvent";
    mMDWorkspace1Name = "DetailedBalanceInputSingleMDEvent";
    mMergedWorkspaceName = "DetailedBalanceInputMergedMDEvent";
    mMDWorkspaceQ1Dname = "DetaledBalanceInputQ1DMDEvent";
    // Gold workspace names
    gold_detail_balanced_merged_name = "DetaildBalancedMergedGoldMD";
    GoldDetailedBalanceSingleWSName = "DetailedBalanceSingleGoldMD";

    // Prepare (first) sample workspace
    createSampleWorkspace(mEventWSName);
    // add sample log Ei
    addSampleLog(mEventWSName, "Ei", "20.", "Number");
    // move bank 1
    moveBank(mEventWSName, "bank1", 3, 3);
    // move bank 2
    moveBank(mEventWSName, "bank2", -3, -3);
    // ad sample log for temperature
    addSampleLog(mEventWSName, "SampleTemp", "25.0", "Number Series");
    // set geoniometer
    setGoniometer(mEventWSName, "Axis0", "0,0,1,0,1");
    // convert to MD
    convertToMD(mEventWSName, mMDWorkspace1Name, "Q3D");

    // Prepare the 2nd MDEventWorkspace
    const std::string event_ws_name2("DetailedBalance2WS");
    const std::string md_ws_name2("DetailedBalance2MD");
    createSampleWorkspace(event_ws_name2);
    // add sample log Ei
    addSampleLog(event_ws_name2, "Ei", "20.", "Number");
    // move bank 1
    moveBank(event_ws_name2, "bank1", 3, 3);
    // move bank 2
    moveBank(event_ws_name2, "bank2", -3, -3);
    // ad sample log for temperature
    addSampleLog(event_ws_name2, "SampleTemp", "250.0", "Number Series");
    // set geoniometer
    setGoniometer(event_ws_name2, "Axis0", "30,0,1,0,1");
    // convert to MD
    convertToMD(event_ws_name2, md_ws_name2, "Q3D");

    // Prepare the 3rd MDEventWorkspace for |Q| and without sample temperature
    const std::string event_ws_name3("DetailedBalance3WS");
    createSampleWorkspace(event_ws_name3);
    // add sample log Ei
    addSampleLog(event_ws_name3, "Ei", "20.", "Number");
    // move bank 1
    moveBank(event_ws_name3, "bank1", 3, 3);
    // move bank 2
    moveBank(event_ws_name3, "bank2", -3, -3);
    // set geoniometer
    setGoniometer(event_ws_name3, "Axis0", "30,0,1,0,1");
    // convert to MD
    convertToMD(event_ws_name3, mMDWorkspaceQ1Dname, "|Q|");

    // Merge 2 workspace
    std::string workspaces(mMDWorkspace1Name + ", " + md_ws_name2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", mMergedWorkspaceName);
    merge_alg.execute();

    // Calculate the expected result from existing algorithms
    calculate_detailed_balance(mEventWSName, event_ws_name2, GoldDetailedBalanceSingleWSName,
                               gold_detail_balanced_merged_name);

    // clean the temporary workspaces
    Mantid::API::AnalysisDataService::Instance().remove(event_ws_name2);
    Mantid::API::AnalysisDataService::Instance().remove(md_ws_name2);

    // FIXME: clean DetailedBalance3WS
    // FIXME: clean DetailedBalance2WS
  }

  void tearDown() override {
    // clean up
    bool exist = Mantid::API::AnalysisDataService::Instance().doesExist(mEventWSName);
    TS_ASSERT(exist);
    Mantid::API::AnalysisDataService::Instance().remove(mEventWSName);

    // single MD workspace
    bool single_exist = Mantid::API::AnalysisDataService::Instance().doesExist(mMDWorkspace1Name);
    TS_ASSERT(single_exist);
    Mantid::API::AnalysisDataService::Instance().remove(mMDWorkspace1Name);

    // merged MD workspace
    bool merge_exist = Mantid::API::AnalysisDataService::Instance().doesExist(mMergedWorkspaceName);
    TS_ASSERT(merge_exist);
    Mantid::API::AnalysisDataService::Instance().remove(mMergedWorkspaceName);
  }

private:
  std::string mEventWSName;
  std::string mMDWorkspace1Name;
  std::string mMergedWorkspaceName;
  std::string mMDWorkspaceQ1Dname;
  // gold data (workspace) names
  std::string gold_detail_balanced_merged_name;
  std::string GoldDetailedBalanceSingleWSName;

  /**
   * @brief Create an EventWorkspace with flat background in unit of DeltaE
   */
  void createSampleWorkspace(const std::string &event_ws_name, const double &xmin = -10, const double &xmax = 19.,
                             const double &binwidth = 0.5) {
    // create sample workspace
    auto create_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    create_alg->initialize();
    create_alg->setPropertyValue("WorkspaceType", "Event");
    create_alg->setPropertyValue("Function", "Flat background");
    create_alg->setProperty("BankPixelWidth", 1);
    create_alg->setProperty("XUnit", "DeltaE");
    create_alg->setProperty("XMin", xmin);
    create_alg->setProperty("XMax", xmax);
    create_alg->setProperty("BinWidth", binwidth);
    create_alg->setPropertyValue("OutputWorkspace", event_ws_name);
    create_alg->execute();
  }

  /**
   * @brief Add sample log to a workspace
   */
  void addSampleLog(const std::string &event_ws_name, const std::string &log_name, const std::string &log_text,
                    const std::string &log_type) {
    auto addlog_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("AddSampleLog");
    addlog_alg->initialize();
    addlog_alg->setPropertyValue("Workspace", event_ws_name);
    addlog_alg->setPropertyValue("LogName", log_name);
    addlog_alg->setPropertyValue("LogText", log_text);
    addlog_alg->setPropertyValue("LogType", log_type);
    addlog_alg->execute();
  }

  /**
   * @brief Move a bank in the workspace
   */
  void moveBank(const std::string &event_ws_name, const std::string bank_name, const double &x_shift,
                const double &z_shift) {
    Mantid::DataHandling::MoveInstrumentComponent move_alg;
    move_alg.initialize();
    move_alg.setPropertyValue("Workspace", event_ws_name);
    move_alg.setPropertyValue("ComponentName", bank_name);
    move_alg.setProperty("X", x_shift);
    move_alg.setProperty("Z", z_shift);
    move_alg.setProperty("RelativePosition", false);
    move_alg.execute();
  }

  /**
   * @brief Set Goniometer axis
   */
  void setGoniometer(const std::string &event_ws_name, const std::string &axis_name, const std::string &axis_value) {
    auto setgon_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("SetGoniometer");
    setgon_alg->initialize();
    setgon_alg->setPropertyValue("Workspace", event_ws_name);
    setgon_alg->setPropertyValue(axis_name, axis_value);
    setgon_alg->execute();
  }

  /**
   * @brief Convert to MD workspace
   */
  void convertToMD(const std::string &event_ws_name, const std::string &md_ws_name, const std::string &q_dimensions) {
    ConvertToMD convert_alg;
    convert_alg.initialize();
    convert_alg.setPropertyValue("InputWorkspace", event_ws_name);
    convert_alg.setPropertyValue("OutputWorkspace", md_ws_name);
    convert_alg.setPropertyValue("QDimensions", q_dimensions);
    convert_alg.execute();
  }

  /**
   * @brief Calculate detailed balance, convert to MD and merge as the old way
   *
   */
  void calculate_detailed_balance(const std::string &event_ws_1, const std::string &event_ws_2,
                                  const std::string &output_sigle_md_name, const std::string &output_merged_md_name) {
    const std::string temp_event_ws1("DetailedBalanceTempEvent1");
    const std::string temp_event_ws2("DetailedBalanceTempEvent2");
    const std::string temp_md2("DetailedBalanceMD2GoldTemp");

    // apply detailed balance and convert to MD for event workspace 1
    applyDetailedBalance(event_ws_1, temp_event_ws1);
    convertToMD(temp_event_ws1, output_sigle_md_name, "Q3D");
    // apply detailed balance and convert to MD for event workspace 2
    applyDetailedBalance(event_ws_2, temp_event_ws2);
    convertToMD(temp_event_ws2, temp_md2, "Q3D");

    // Merge 2 workspace
    std::string workspaces(output_sigle_md_name + ", " + temp_md2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", output_merged_md_name);
    merge_alg.execute();
  }

  /**
   * @brief Apply detailed balance to event workspace
   */
  void applyDetailedBalance(const std::string &input_ws_name, const std::string &output_ws_name) {
    auto apply_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ApplyDetailedBalance");
    apply_alg->initialize();
    apply_alg->setProperty("InputWorkspace", input_ws_name);
    apply_alg->setProperty("Temperature", "SampleTemp");
    apply_alg->setProperty("OutputWorkspace", output_ws_name);
    apply_alg->execute();
  }

  /**
   * @brief compare MD events
   */
  bool compareMDEvents(const std::string &ws1, const std::string &ws2, const bool &checkBoxID = true) {
    // Compare number of MDEvents
    API::IMDEventWorkspace_sptr md1 =
        std::dynamic_pointer_cast<IMDEventWorkspace>(API::AnalysisDataService::Instance().retrieve(ws1));
    API::IMDEventWorkspace_sptr md2 =
        std::dynamic_pointer_cast<IMDEventWorkspace>(API::AnalysisDataService::Instance().retrieve(ws2));

    // compare number of events
    if (md1->getNEvents() != md2->getNEvents()) {
      return false;
    }

    // Compare MDWorkspaces
    CompareMDWorkspaces compare_alg;
    compare_alg.initialize();
    compare_alg.setPropertyValue("Workspace1", ws1);
    compare_alg.setPropertyValue("Workspace2", ws2);
    compare_alg.setProperty("Tolerance", 0.0001);
    compare_alg.setProperty("CheckEvents", true);
    compare_alg.setProperty("IgnoreBoxID", !checkBoxID);
    compare_alg.execute();
    TS_ASSERT(compare_alg.isExecuted());

    // retrieve result
    bool equals = compare_alg.getProperty("Equals");

    return equals;
  }
};
