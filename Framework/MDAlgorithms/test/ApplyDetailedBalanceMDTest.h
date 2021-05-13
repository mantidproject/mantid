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
#include "MantidMDAlgorithms/ApplyDetailedBalanceMD.h"
#include "MantidMDAlgorithms/ConvertToMD.h"
#include "MantidMDAlgorithms/MergeMD.h"
#include "MantidTestHelpers/BinaryOperationMDTestHelper.h"
// only needed for local test
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/SaveNexus.h"
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"
#include "MantidMDAlgorithms/SaveMD.h"

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

  /* Test apply detailed balance to 1 run
   */
  void test_1run() {
    // Check whether the MD to test does exist
    auto singlemd = Mantid::API::AnalysisDataService::Instance().retrieve(mMDWorkspace1Name);
    TS_ASSERT(singlemd);
  }

  void setUp() override {
    // Define workspace names
    mEventWSName = "DetailedBalanceSampleWS";
    mMDWorkspace1Name = "DetailedBalanceMDEvent";
    mMergedWorkspaceName = "DetailedBalanceMergedMDEvent";

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

    // Merge 2 workspace
    std::string workspaces(mMDWorkspace1Name + ", " + md_ws_name2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", mMergedWorkspaceName);
    merge_alg.execute();

    // Calculate the expected result from existing algorithms
    calculate_detailed_balance(mEventWSName, event_ws_name2, "DetaildBalancedGoldMD");

    //    // save for verification
    //    SaveMD save_alg;
    //    save_alg.initialize();
    //    save_alg.setProperty("InputWorkspace", mMDWorkspace1Name);
    //    save_alg.setProperty("Filename", "/tmp/detailed_balance_1md.nxs");
    //    save_alg.execute();
    //    TS_ASSERT(save_alg.isExecuted());
    //    // merged one
    //    save_alg.setProperty("InputWorkspace", mMergedWorkspaceName);
    //    save_alg.setProperty("Filename", "/tmp/detailed_balance_merged_md.nxs");
    //    save_alg.execute();

    // clean the temporary workspaces
    Mantid::API::AnalysisDataService::Instance().remove(event_ws_name2);
    Mantid::API::AnalysisDataService::Instance().remove(md_ws_name2);
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
   */
  void calculate_detailed_balance(const std::string &event_ws_1, const std::string &event_ws_2,
                                  const std::string &output_md_name) {
    const std::string temp_event_ws1("DetailedBalanceTempEvent1");
    const std::string temp_event_ws2("DetailedBalanceTempEvent2");
    const std::string temp_md1("DetailedBalanceMD1GoldTemp");
    const std::string temp_md2("DetailedBalanceMD2GoldTemp");

    applyDetailedBalance(event_ws_1, temp_event_ws1);
    convertToMD(temp_event_ws1, temp_md1, "Q3D");
    applyDetailedBalance(event_ws_2, temp_event_ws2);
    convertToMD(temp_event_ws2, temp_md2, "Q3D");

    // Merge 2 workspace
    std::string workspaces(temp_md1 + ", " + temp_md2);
    MergeMD merge_alg;
    merge_alg.initialize();
    merge_alg.setProperty("InputWorkspaces", workspaces);
    merge_alg.setProperty("OutputWorkspace", output_md_name);
    merge_alg.execute();
  }

  void applyDetailedBalance(const std::string &input_ws_name, const std::string &output_ws_name) {
    auto apply_alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("ApplyDetailedBalance");
    apply_alg->initialize();
    apply_alg->setProperty("InputWorkspace", input_ws_name);
    apply_alg->setProperty("Temperature", "SampleTemp");
    apply_alg->setProperty("OutputWorkspace", output_ws_name);
    apply_alg->execute();
  }

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
AddSampleLog(Workspace=we1,LogName='SampleTemp', LogText='25.0', LogType='Number Series')
AddSampleLog(Workspace=we2,LogName='SampleTemp', LogText='250.0', LogType='Number Series')
SetGoniometer(Workspace=we1, Axis0='0,0,1,0,1')
SetGoniometer(Workspace=we2, Axis0='30,0,1,0,1')

# old way
weadb1 = ApplyDetailedBalance(InputWorkspace=we1, Temperature='SampleTemp')
mdabd1 = ConvertToMD(InputWorkspace=weadb1, QDimensions='Q3D')
weadb2 = ApplyDetailedBalance(InputWorkspace=we2, Temperature='SampleTemp')
mdabd2 = ConvertToMD(InputWorkspace=weadb2, QDimensions='Q3D')
mdabd = MergeMD(InputWorkspaces='mdabd1, mdabd2')

# new way
md1 = ConvertToMD(InputWorkspace=we1, QDimensions='Q3D')
md2 = ConvertToMD(InputWorkspace=we2, QDimensions='Q3D')
md = MergeMD(InputWorkspaces='md1,md2')

   */
};
