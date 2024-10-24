// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// local
#include "MantidMDAlgorithms/DgsScatteredTransmissionCorrectionMD.h"

// 3rd party
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

class DgsScatteredTransmissionCorrectionMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    DgsScatteredTransmissionCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //---------------------------------------------------------------------------------------------

  void test_single_run() {
    std::vector<std::string> qDims = {"Q3D", "|Q|"};
    for (auto qDim : qDims) {
      createEventWs("events", "20.");
      // convert, then correct
      convertToMD("events", "md", qDim);
      applyCorrectionToMD("md", 1. / 11);
      // correct, then convert
      applyCorrectionToEvents("events", 1. / 11);
      convertToMD("events", "expected", qDim);
      TS_ASSERT(compareMDEventWorkspaces("md", "expected", true))
      std::vector<std::string> leftOvers{"events", "md", "expected"};
      cleanup(leftOvers);
    }
  }

  void test_merged_runs() {
    double factor(1. / 11);
    createEventWs("events1", "20.", "0,0,1,0,1");
    createEventWs("events2", "20.", "30,0,1,0,1");
    // convert, merge, then correct
    convertToMD("events1", "md1", "Q3D");
    convertToMD("events2", "md2", "Q3D");
    mergeMD("md1", "md2", "md");
    applyCorrectionToMD("md", factor);
    binMD("md", "Q_sample_x,-9,9,1", "Q_sample_y,-9,9,1", "Q_sample_z,-9,9,1", "DeltaE, -10,19,100");
    // correct, convert, then merge
    applyCorrectionToEvents("events1", factor);
    applyCorrectionToEvents("events2", factor);
    convertToMD("events1", "md1", "Q3D");
    convertToMD("events2", "md2", "Q3D");
    mergeMD("md1", "md2", "expected");
    binMD("expected", "Q_sample_x,-9,9,1", "Q_sample_y,-9,9,1", "Q_sample_z,-9,9,1", "DeltaE, -10,19,100");
    TS_ASSERT(compareMDHistoWorkspaces("md", "expected"));
    std::vector<std::string> leftOvers{"events1", "md1", "events2", "md2", "md", "expected"};
    cleanup(leftOvers);
  }

private:
  /// Create an EventWorkspace with flat background in units of DeltaE
  void createSampleWorkspace(std::string outputWorkspace, double xmin = -10, double xmax = 19., double binwidth = 0.5) {
    if (AnalysisDataService::Instance().doesExist(outputWorkspace))
      AnalysisDataService::Instance().remove(outputWorkspace);
    // create sample workspace
    auto create_alg = AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    create_alg->initialize();
    create_alg->setPropertyValue("WorkspaceType", "Event");
    create_alg->setPropertyValue("Function", "Flat background");
    create_alg->setProperty("BankPixelWidth", 1);
    create_alg->setProperty("XUnit", "DeltaE");
    create_alg->setProperty("XMin", xmin);
    create_alg->setProperty("XMax", xmax);
    create_alg->setProperty("BinWidth", binwidth);
    create_alg->setProperty("NumEvents", 1000);
    create_alg->setProperty("OutputWorkspace", outputWorkspace);
    create_alg->execute();
    TS_ASSERT(create_alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
  }

  /// Add sample log to a workspace
  void addSampleLog(std::string inputWorkspace, std::string log_name, std::string log_text, std::string log_type) {
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("AddSampleLog");
    alg->initialize();
    alg->setPropertyValue("Workspace", inputWorkspace);
    alg->setPropertyValue("LogName", log_name);
    alg->setPropertyValue("LogText", log_text);
    alg->setPropertyValue("LogType", log_type);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(inputWorkspace))
  }

  /// Move a bank in the workspace
  void moveBank(std::string inputWorkspace, std::string bankName, double xShift, double zShift) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("MoveInstrumentComponent");
    alg->initialize();
    alg->setPropertyValue("Workspace", inputWorkspace);
    alg->setPropertyValue("ComponentName", bankName);
    alg->setProperty("X", xShift);
    alg->setProperty("Z", zShift);
    alg->setProperty("RelativePosition", false);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(inputWorkspace))
  }

  /// Set Goniometer axis
  void setGoniometer(std::string inputWorkspace, std::string axisName, std::string axisValue) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("SetGoniometer");
    alg->initialize();
    alg->setPropertyValue("Workspace", inputWorkspace);
    alg->setPropertyValue(axisName, axisValue);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(inputWorkspace))
  }

  /// Convert to MD workspace
  void convertToMD(std::string inputWorkspace, std::string outputWorkspace, std::string qDimensions) {
    if (AnalysisDataService::Instance().doesExist(outputWorkspace))
      AnalysisDataService::Instance().remove(outputWorkspace);
    auto alg = AlgorithmManager::Instance().createUnmanaged("ConvertToMD");
    alg->initialize();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWorkspace);
    alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->setProperty("QDimensions", qDimensions);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
  }

  /// Add or rescale the "TOF" of an event. In these tests, the "TOF" is DeltaE
  void scaleX(std::string InputWorkspace, double factor, std::string operation, std::string outputWorkspace = "") {
    if (outputWorkspace.size() == 0)
      outputWorkspace = InputWorkspace;
    auto alg = AlgorithmManager::Instance().createUnmanaged("ScaleX");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", InputWorkspace);
    alg->setProperty("Factor", factor);
    alg->setProperty("Operation", operation);
    alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
  }

  /// Generate an event workspace in units of DeltaE with an initial energy
  void createEventWs(std::string outputWorkspace, std::string Ei, std::string goniometer = "0,0,1,0,1") {
    if (AnalysisDataService::Instance().doesExist(outputWorkspace))
      AnalysisDataService::Instance().remove(outputWorkspace);
    double e = std::stod(Ei);
    createSampleWorkspace(outputWorkspace, -e / 2., e - 1.);
    addSampleLog(outputWorkspace, "deltaE-mode", "Direct", "String");
    addSampleLog(outputWorkspace, "Ei", Ei, "Number");
    moveBank(outputWorkspace, "bank1", 3, 3);
    moveBank(outputWorkspace, "bank2", -3, -3);
    setGoniometer(outputWorkspace, "Axis0", goniometer);
  }

  /// Apply the transmission correction to an input MD workspace
  void applyCorrectionToMD(std::string inputWorkspace, double factor, std::string outputWorkspace = "") {
    if (outputWorkspace.size() == 0)
      outputWorkspace = inputWorkspace;
    DgsScatteredTransmissionCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", inputWorkspace);
    alg.setProperty("ExponentFactor", factor);
    alg.setProperty("OutputWorkspace", outputWorkspace);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
  }

  /// Apply the transmission correction to an input events workspace in units of DeltaE
  void applyCorrectionToEvents(std::string inputWorkspace, double factor, std::string outputWorkspace = "") {
    if (outputWorkspace.size() == 0)
      outputWorkspace = inputWorkspace; // in-place changes
    // Get Ei from the logs
    auto ws = std::dynamic_pointer_cast<IEventWorkspace>(AnalysisDataService::Instance().retrieve(inputWorkspace));
    double Ei = ws->getEFixed();
    // Change X-axis from deltaE to Ef
    scaleX(inputWorkspace, -Ei, "Add", outputWorkspace); // deltaE-Ei becomes -Ef
    scaleX(outputWorkspace, -1., "Multiply");            // the X-axis becomes Ef
    // ExponentialCorrection algorithm multiplies signal by C0*exp(-C1*x), where x is Ef
    auto alg = AlgorithmManager::Instance().createUnmanaged("ExponentialCorrection");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", outputWorkspace);
    alg->setProperty("Operation", "Multiply");
    alg->setProperty("C0", 1.0);
    alg->setProperty("C1", -factor); // negative, because we want to apply exp(factor*Ef)
    alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
    // Change X-axis from Ef to deltaE
    scaleX(outputWorkspace, -1., "Multiply"); // the X-axis becomes -Ef
    scaleX(outputWorkspace, Ei, "Add");       // Ei - Ef converts back to DeltaE
  }

  void mergeMD(std::string md1, std::string md2, std::string outputWorkspace) {
    if (AnalysisDataService::Instance().doesExist(outputWorkspace))
      AnalysisDataService::Instance().remove(outputWorkspace);
    auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("MergeMD");
    alg->initialize();
    TS_ASSERT(alg->isInitialized())
    std::string workspaces(md1 + ", " + md2);
    alg->setProperty("InputWorkspaces", workspaces);
    alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace));
  }

  // Integrate over the Q-dimensions and bin over the DeltaE dimension
  void binMD(std::string inputWorkspace, std::string qBinningX, std::string qBinningY, std::string qBinningZ,
             std::string eBinning, std::string outputWorkspace = "") {
    if (outputWorkspace.size() == 0)
      outputWorkspace = inputWorkspace; // in-place changes
    auto alg = AlgorithmManager::Instance().createUnmanaged("BinMD");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWorkspace);
    alg->setProperty("AxisAligned", true);
    alg->setProperty("AlignedDim0", qBinningX);
    alg->setProperty("AlignedDim1", qBinningY);
    alg->setProperty("AlignedDim2", qBinningZ);
    alg->setProperty("AlignedDim3", eBinning);
    alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(outputWorkspace))
  }

  bool compareMDEventWorkspaces(std::string ws1, std::string ws2, bool checkEvents = false) {
    auto md1 = std::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(ws1));
    auto md2 = std::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(ws2));
    if (md1->getNEvents() != md2->getNEvents())
      return false;
    auto alg = AlgorithmManager::Instance().createUnmanaged("CompareMDWorkspaces");
    alg->initialize();
    alg->setPropertyValue("Workspace1", ws1);
    alg->setPropertyValue("Workspace2", ws2);
    alg->setProperty("Tolerance", 0.001);
    alg->setProperty("CheckEvents", checkEvents);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    return alg->getProperty("Equals");
  }

  bool compareMDHistoWorkspaces(std::string ws1, std::string ws2) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("CompareMDWorkspaces");
    alg->initialize();
    alg->setPropertyValue("Workspace1", ws1);
    alg->setPropertyValue("Workspace2", ws2);
    alg->setProperty("Tolerance", 0.0001);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    return alg->getProperty("Equals");
  }

  /// Delete a series of input workspaces using the Analysis Data Service
  void cleanup(const std::vector<std::string> &workspaces) {
    for (const auto &ws : workspaces) {
      if (AnalysisDataService::Instance().doesExist(ws))
        AnalysisDataService::Instance().remove(ws);
    }
  }
};
