// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/System.h"
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
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/MergeMD.h"
#include "MantidMDAlgorithms/PolarizationAngleCorrectionMD.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class PolarizationAngleCorrectionMDTest : public CxxTest::TestSuite {
public:
  //---------------------------------------------------------------------------------------------
  void test_Init() {
    PolarizationAngleCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //---------------------------------------------------------------------------------------------
  /**
   * @brief Test input workspace to test failed cases
   */
  void test_Failures() {
    auto q1dmd = Mantid::API::AnalysisDataService::Instance().retrieve(mQ1DWorkspaceName);
    TS_ASSERT(q1dmd);

    PolarizationAngleCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mQ1DWorkspaceName);
    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("PolarizationAngle", -181.));
    alg.setProperty("PolarizationAngle", 10.);

    TS_ASSERT_THROWS_ANYTHING(alg.setProperty("Precision", 1.1));
    alg.setProperty("Precision", 0.2);

    alg.setProperty("OutputWorkspace", "ExpectToFail");

    // Expect to fail due to missing temperature sample log
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  //---------------------------------------------------------------------------------------------
  /**
   * @brief Test apply detailed balance to 1 run
   */
  void test_1run_qlab() {
    // Check whether the MD to test does exist
    auto singlemd = Mantid::API::AnalysisDataService::Instance().retrieve(mQLabWorkspaceName);
    TS_ASSERT(singlemd);

    // specify the output
    std::string outputname("PolarizationAngleSingleQlabTest");

    // Calculate detailed balance for the single MDEventWorkspace
    PolarizationAngleCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mQLabWorkspaceName);
    alg.setProperty("PolarizationAngle", -10.);
    alg.setProperty("Precision", 0.2);
    alg.setProperty("OutputWorkspace", outputname);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Verify
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputname));

    bool equals = compareMDEvents(outputname, mGoldCorrectedQLabWSName);
    TS_ASSERT(equals);

    // Clean up
    cleanWorkspace(outputname, false);
  }

  void test_1run_qsample() {
    // Check whether the MD to test does exist
    auto singlemd = Mantid::API::AnalysisDataService::Instance().retrieve(mQLabWorkspaceName);
    TS_ASSERT(singlemd);

    // specify the output
    std::string outputname("PolarizationAngleSingleQsampleTest");

    // Calculate detailed balance for the single MDEventWorkspace
    PolarizationAngleCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mQSampleWorkspaceName);
    alg.setProperty("PolarizationAngle", -10.);
    alg.setProperty("Precision", 0.2);
    alg.setProperty("OutputWorkspace", outputname);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Verify
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputname));

    bool equals = compareMDEvents(outputname, mGoldCorrectedQSampleWSName);
    TS_ASSERT(equals);

    // Clean up
    cleanWorkspace(outputname, false);
  }

  /**
   * @brief Test applying detailed balance to 2 merged runs
   */
  void Future_test_merged_runs() {
    auto mergedmd = Mantid::API::AnalysisDataService::Instance().retrieve(mQSampleMergedWorkspaceName);
    TS_ASSERT(mergedmd);

    // specify the output
    std::string outputname("PolarizationAngleMergedQ3Test");

    PolarizationAngleCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", mQSampleMergedWorkspaceName);
    alg.setProperty("Temperature", "SampleTemp");
    alg.setProperty("OutputWorkspace", outputname);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Verify
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputname));

    bool equals = compareMDEvents(outputname, mGoldCorrectedQSampleMergedWSName);
    TS_ASSERT(equals);

    // clean up
    cleanWorkspace(outputname, false);
  }

  void setUp() override {
    /*
     * # import mantid algorithms, numpy and matplotlib
       from mantid.simpleapi import *
       import matplotlib.pyplot as plt
       import numpy as np

       we1 = CreateSampleWorkspace(WorkspaceType='Event',
                                  Function='Flat background',
                                  BankPixelWidth=1,
                                  XUnit='DeltaE',
                                  XMin=-10,
                                  XMax=19,
                                  BinWidth=0.5)
       AddSampleLog(Workspace=we1,LogName='Ei', LogText='20.', LogType='Number')
       MoveInstrumentComponent(Workspace=we1, ComponentName='bank1', X=3, Z=3, RelativePosition=False)
       MoveInstrumentComponent(Workspace=we1, ComponentName='bank2', X=-3, Z=-3, RelativePosition=False)
       we2=CloneWorkspace(we1)
       SetGoniometer(Workspace=we1, Axis0='0,0,1,0,1')
       SetGoniometer(Workspace=we2, Axis0='30,0,1,0,1')

       # old way
       we1c = HyspecScharpfCorrection(InputWorkspace=we1,
                                      PolarizationAngle=-10,
                                      Precision=0.2)
       md1c = ConvertToMD(InputWorkspace=we1c, QDimensions='Q3D')
       we2c = HyspecScharpfCorrection(InputWorkspace=we2,
                                      PolarizationAngle=-10,
                                      Precision=0.2)
       md2c = ConvertToMD(InputWorkspace=we2c, QDimensions='Q3D')
       mdpac = MergeMD(InputWorkspaces='md1c, md2c')

       # new way
       md1 = ConvertToMD(InputWorkspace=we1, QDimensions='Q3D')
       md2 = ConvertToMD(InputWorkspace=we2, QDimensions='Q3D')
       md = MergeMD(InputWorkspaces='md1,md2')
       # mdpacMD = PolarizationAngleCorrectionMD(InputWorkspace=md, PolarizationAngle=-10, Precision=0.2)

     */
    // Define workspace names
    std::string event_ws_0("PolarizationAngleRawEvent0");
    std::string event_ws_1("PolarizationAngleRawEvent0");

    mQSampleWorkspaceName = "PolarizationAngleInputQSampleMDEvent";
    mQLabWorkspaceName = "PolarizationAngleInputQLabMDEvent";
    mQSampleMergedWorkspaceName = "PolarizationAngleInputMergedQSampleMDEvent";
    mQ1DWorkspaceName = "PolarizationAngleInputQ1DMDEvent";

    // Gold workspace names
    mGoldCorrectedQSampleWSName = "PAGoldCorrectedQSample";
    mGoldCorrectedQLabWSName = "PAGoldCorrectedQLab";
    mGoldCorrectedQSampleMergedWSName = "PAGoldCorrectedMergedQSample";

    // Prepare first set of workspaces
    std::string axis00("0,0,1,0,1");
    generateTestSet(event_ws_0, mQSampleWorkspaceName, mQLabWorkspaceName, mQ1DWorkspaceName, axis00);

    //    // Prepare (first) sample workspace
    //    createSampleWorkspace(mEventWSName);
    //    // add sample log Ei
    //    addSampleLog(mEventWSName, "Ei", "20.", "Number");
    //    // move bank 1
    //    moveBank(mEventWSName, "bank1", 3, 3);
    //    // move bank 2
    //    moveBank(mEventWSName, "bank2", -3, -3);
    //    // set geoniometer
    //    setGoniometer(mEventWSName, "Axis0", "0,0,1,0,1");
    //    // convert to MD
    //    convertToMD(mEventWSName, mMDWorkspace1Name, "Q3D");

    // Prepare the 2nd MDEventWorkspace
    const std::string md_ws_name2("PolarizationAngle2MD");
    const std::string axis01("30,0,1,0,1");
    generateTestSet(event_ws_1, md_ws_name2, "", "", axis01);

    //    const std::string event_ws_name2("PolarizationAngle2WS");
    //    createSampleWorkspace(event_ws_name2);
    //    // add sample log Ei
    //    addSampleLog(event_ws_name2, "Ei", "20.", "Number");
    //    // move bank 1
    //    moveBank(event_ws_name2, "bank1", 3, 3);
    //    // move bank 2
    //    moveBank(event_ws_name2, "bank2", -3, -3);
    //    // set geoniometer
    //    setGoniometer(event_ws_name2, "Axis0", "30,0,1,0,1");
    //    // convert to MD
    //    convertToMD(event_ws_name2, md_ws_name2, "Q3D");

    //    // Prepare the 3rd MDEventWorkspace for |Q| and without sample temperature
    //    const std::string event_ws_name3("PolarizationAngle3WS");
    //    createSampleWorkspace(event_ws_name3);
    //    // add sample log Ei
    //    addSampleLog(event_ws_name3, "Ei", "20.", "Number");
    //    // move bank 1
    //    moveBank(event_ws_name3, "bank1", 3, 3);
    //    // move bank 2
    //    moveBank(event_ws_name3, "bank2", -3, -3);
    //    // set geoniometer
    //    setGoniometer(event_ws_name3, "Axis0", "30,0,1,0,1");
    //    // convert to MD
    //    convertToMD(event_ws_name3, mMDWorkspaceQ1Dname, "|Q|");

    // Merge 2 workspace
    std::string workspaces(mQSampleWorkspaceName + ", " + md_ws_name2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", mQSampleMergedWorkspaceName);
    merge_alg.execute();

    // Calculate the expected result from existing algorithms
    calculate_polarization_angle_correction(event_ws_0, event_ws_1, mGoldCorrectedQSampleWSName,
                                            mGoldCorrectedQLabWSName, mGoldCorrectedQSampleMergedWSName);

    // clean the temporary workspaces
    Mantid::API::AnalysisDataService::Instance().remove(event_ws_0);
    Mantid::API::AnalysisDataService::Instance().remove(event_ws_1);
    Mantid::API::AnalysisDataService::Instance().remove(md_ws_name2);
  }

  void tearDown() override {
    // clean up
    // single MD workspace
    cleanWorkspace(mQSampleWorkspaceName, true);
    cleanWorkspace(mQLabWorkspaceName, true);
    cleanWorkspace(mQSampleMergedWorkspaceName, true);
    cleanWorkspace(mQ1DWorkspaceName, true);
    // gold worksapces
    cleanWorkspace(mGoldCorrectedQLabWSName, true);
  }

private:
  // MDEventWorkspaces
  std::string mQSampleWorkspaceName;
  std::string mQLabWorkspaceName;
  std::string mQSampleMergedWorkspaceName;
  std::string mQ1DWorkspaceName;
  // Gold data (workspace) names
  std::string mGoldCorrectedQSampleWSName;
  std::string mGoldCorrectedQLabWSName;
  std::string mGoldCorrectedQSampleMergedWSName;

  //---------------------------------------------------------------------------------------------
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

  /**
   * @brief Generate 1 set of test data/workspaces
   * @param event_ws_name
   * @param sample_md_name
   * @param lab_md_name
   * @param axis0
   */
  void generateTestSet(const std::string &event_ws_name, const std::string &sample_md_name,
                       const std::string &lab_md_name, const std::string &q1d_md_name, const std::string &axis0) {
    //        mQSampleWorkspaceName, mQLabWorkspaceName, axis0);

    // Prepare (first) sample workspace
    createSampleWorkspace(event_ws_name);
    // add sample log Ei
    addSampleLog(event_ws_name, "Ei", "20.", "Number");
    // move bank 1
    moveBank(event_ws_name, "bank1", 3, 3);
    // move bank 2
    moveBank(event_ws_name, "bank2", -3, -3);
    // set geoniometer
    setGoniometer(event_ws_name, "Axis0", axis0);
    // convert to MD
    if (sample_md_name != "")
      convertToMD(event_ws_name, sample_md_name, "Q3D", "Q_sample");
    // convert to MD
    if (lab_md_name != "")
      convertToMD(event_ws_name, lab_md_name, "Q3D", "Q_lab");
    // convert to Q1D
    if (q1d_md_name != "")
      convertToMD(event_ws_name, q1d_md_name, "|Q|", "");
  }

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
  void convertToMD(const std::string &event_ws_name, const std::string &md_ws_name, const std::string &q_dimensions,
                   const std::string &q3dframe) {
    ConvertToMD convert_alg;
    convert_alg.initialize();
    convert_alg.setPropertyValue("InputWorkspace", event_ws_name);
    convert_alg.setPropertyValue("OutputWorkspace", md_ws_name);
    convert_alg.setPropertyValue("QDimensions", q_dimensions);
    if (q3dframe != "")
      convert_alg.setPropertyValue("Q3DFrames", q3dframe);
    convert_alg.execute();
  }

  /**
   * @brief Calculate detailed balance, convert to MD and merge as the old way
   *
   */
  void calculate_polarization_angle_correction(const std::string &event_ws_1, const std::string &event_ws_2,
                                               const std::string &corrected_qsample_name,
                                               const std::string &corrected_qlab_name,
                                               const std::string &corrected_qsample_merged_name) {
    const std::string temp_event_ws1("PolarizationAngleTempEvent1");
    const std::string temp_event_ws2("PolarizationAngleTempEvent2");
    const std::string temp_md2("PolarizationAngleMD2GoldTemp");

    // apply detailed balance and convert to MD for event workspace 1
    applyPolarizationAngleCorrection(event_ws_1, temp_event_ws1);
    convertToMD(temp_event_ws1, corrected_qsample_name, "Q3D", "Q_sample");
    convertToMD(temp_event_ws1, corrected_qlab_name, "Q3D", "Q_lab");

    // apply detailed balance and convert to MD for event workspace 2
    applyPolarizationAngleCorrection(event_ws_2, temp_event_ws2);
    convertToMD(temp_event_ws2, temp_md2, "Q3D", "Q_sample");

    // Merge 2 workspace
    std::string workspaces(corrected_qsample_name + ", " + temp_md2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", corrected_qsample_merged_name);
    merge_alg.execute();
  }

  /**
   * @brief Apply detailed balance to event workspace
   */
  void applyPolarizationAngleCorrection(const std::string &input_ws_name, const std::string &output_ws_name) {
    auto apply_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("HyspecScharpfCorrection");
    apply_alg->initialize();
    apply_alg->setProperty("InputWorkspace", input_ws_name);
    apply_alg->setProperty("PolarizationAngle", -10.);
    apply_alg->setProperty("Precision", 0.2);
    apply_alg->setProperty("OutputWorkspace", output_ws_name);
    apply_alg->execute();
  }

  /**
   * @brief compare MD events
   */
  bool compareMDEvents(const std::string &ws1, const std::string &ws2) {
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
    compare_alg.setProperty("Tolerance", 0.001);
    compare_alg.setProperty("CheckEvents", false);
    compare_alg.execute();
    TS_ASSERT(compare_alg.isExecuted());

    // retrieve result
    bool equals = compare_alg.getProperty("Equals");

    return equals;
  }
};
