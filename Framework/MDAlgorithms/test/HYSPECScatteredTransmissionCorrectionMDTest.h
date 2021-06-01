// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// local
#include "MantidMDAlgorithms/HYSPECScatteredTransmissionCorrectionMD.h"

// 3rd party
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IEventWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::MDAlgorithms;

class HYSPECScatteredTransmissionCorrectionMDTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    HYSPECScatteredTransmissionCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_validation() {
    createEventWs("events", "20.");
    convertToMD("events", "md", "Q3D");
    TS_ASSERT_THROWS(applyCorrectionToMD("md", 0.0), const std::runtime_error &); // must be positive
    std::vector<std::string> leftOvers{"events", "md"};
    cleanup(leftOvers);
  }

  //---------------------------------------------------------------------------------------------
  /**
   * @brief Test apply correction to 1 run
   */
  void test_single_run() {
    std::vector<std::string> qDims = {"Q3D", "|Q|"};
    for (const auto qDim : qDims) {
      createEventWs("events", "20.");
      // convert, then correct
      convertToMD("events", "md", qDim);
      applyCorrectionToMD("md", 1. / 11);
      // correct, then convert
      applyCorrectionToEvents("events", 1. / 11);
      convertToMD("events", "expected", "Q3D");
      TS_ASSERT(compareMDWorkspaces("md", "expected"))
      std::vector<std::string> leftOvers{"events", "md", "expected"};
      cleanup(leftOvers);
    }
  }

  void test_merged_runs() {
    std::vector<std::string> qDims = {"Q3D", "|Q|"};
    for (const auto qDim : qDims) {
      createEventWs("events1", "20.");
      createEventWs("events2", "30.");
      // convert, merge, then correct
      convertToMD("events1", "md1", qDim);
      convertToMD("events2", "md2", qDim);
      mergeMD("md1", "md2", "md");
      applyCorrectionToMD("md", 1. / 11);
      // correct, convert, then merge
      applyCorrectionToEvents("events1", 1. / 11);
      applyCorrectionToEvents("events2", 1. / 11);
      convertToMD("events1", "md1", qDim);
      convertToMD("events2", "md2", qDim);
      mergeMD("md1", "md2", "expected");
      TS_ASSERT(compareMDWorkspaces("md", "expected"))
      std::vector<std::string> leftOvers{"events1", "md1", "events2", "md2", "md", "expected"};
      cleanup(leftOvers);
    }
  }

  /**
   * @brief Create an EventWorkspace with flat background in unit of DeltaE
   */
  void createSampleWorkspace(const std::string &event_ws_name, const double &xmin = -10, const double &xmax = 19.,
                             const double &binwidth = 0.5) {
    // create sample workspace
    auto create_alg = AlgorithmManager::Instance().createUnmanaged("CreateSampleWorkspace");
    create_alg->initialize();
    create_alg->setPropertyValue("WorkspaceType", "Event");
    create_alg->setPropertyValue("Function", "Flat background");
    create_alg->setProperty("InstrumentName", "HYSPEC");
    create_alg->setProperty("BankPixelWidth", 1);
    create_alg->setProperty("XUnit", "DeltaE");
    create_alg->setProperty("XMin", xmin);
    create_alg->setProperty("XMax", xmax);
    create_alg->setProperty("BinWidth", binwidth);
    create_alg->setProperty("NumEvents", 1000);
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
    auto alg = AlgorithmManager::Instance().createUnmanaged("MoveInstrumentComponent");
    alg->initialize();
    alg->setPropertyValue("Workspace", event_ws_name);
    alg->setPropertyValue("ComponentName", bank_name);
    alg->setProperty("X", x_shift);
    alg->setProperty("Z", z_shift);
    alg->setProperty("RelativePosition", false);
    alg->execute();
  }

  /**
   * @brief Set Goniometer axis
   */
  void setGoniometer(const std::string &event_ws_name, const std::string &axis_name, const std::string &axis_value) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("SetGoniometer");
    alg->initialize();
    alg->setPropertyValue("Workspace", event_ws_name);
    alg->setPropertyValue(axis_name, axis_value);
    alg->execute();
  }

  /**
   * @brief Convert to MD workspace
   */
  void convertToMD(const std::string &event_ws_name, const std::string &md_ws_name, const std::string &q_dimensions) {
    auto alg = AlgorithmManager::Instance().createUnmanaged("ConvertToMD");
    alg->initialize();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", event_ws_name);
    alg->setPropertyValue("OutputWorkspace", md_ws_name);
    alg->setPropertyValue("QDimensions", q_dimensions);
    alg->execute();
  }

  void scaleX(std::string event_ws_name, double factor, std::string operation, std::string outputWorkspace = "") {
    auto alg = AlgorithmManager::Instance().createUnmanaged("ScaleX");
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", event_ws_name);
    alg->setProperty("Factor", factor);
    alg->setProperty("Operation", operation);
    if (outputWorkspace.size() > 0)
      alg->setProperty("OutputWorkspace", outputWorkspace);
    alg->execute();
  }

  void createEventWs(const std::string outputWsName, std::string Ei) {
    double e = std::stod(Ei);
    createSampleWorkspace(outputWsName, -e / 2., e - 1.);
    addSampleLog(outputWsName, "deltaE-mode", "Direct", "String");
    addSampleLog(outputWsName, "Ei", Ei, "Number");
    moveBank(outputWsName, "bank1", 3, 3);
    moveBank(outputWsName, "bank2", -3, -3);
    setGoniometer(outputWsName, "Axis0", "0,0,1,0,1");
  }

  void applyCorrectionToMD(std::string inputWorkspace, double factor, const std::string outputWorkspace = "") {
    HYSPECScatteredTransmissionCorrectionMD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    alg.setPropertyValue("InputWorkspace", inputWorkspace);
    alg.setProperty("ExponentFactor", factor);
    if (outputWorkspace.size() > 0)
      alg.setProperty("OutputWorkspace", outputWorkspace);
    alg.execute();
    TS_ASSERT(alg.isExecuted());
  }

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
    alg->execute();
    // Change X-axis from Ef to deltaE
    scaleX(outputWorkspace, -1., "Multiply"); // the X-axis becomes -Ef
    scaleX(outputWorkspace, Ei, "Add");       // Ei - Ef converts back to DeltaE
  }

  void mergeMD(std::string md1, std::string md2, std::string outputWorkspace) {
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

  bool compareMDWorkspaces(std::string ws1, std::string ws2) {
    auto md1 = std::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(ws1));
    auto md2 = std::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve(ws2));

    if (md1->getNEvents() != md2->getNEvents())
      return false;

    auto alg = AlgorithmManager::Instance().createUnmanaged("CompareMDWorkspaces");
    alg->initialize();
    alg->setPropertyValue("Workspace1", ws1);
    alg->setPropertyValue("Workspace2", ws2);
    alg->setProperty("Tolerance", 0.0001);
    alg->setProperty("CheckEvents", false);
    alg->execute();
    TS_ASSERT(alg->isExecuted());
    return alg->getProperty("Equals");
  }

  void cleanup(const std::vector<std::string> &workspaces) {
    for (const auto ws : workspaces) {
      if (AnalysisDataService::Instance().doesExist(ws))
        AnalysisDataService::Instance().remove(ws);
    }
  }
};