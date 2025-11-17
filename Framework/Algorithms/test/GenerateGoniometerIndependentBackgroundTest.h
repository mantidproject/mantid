// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/GenerateGoniometerIndependentBackground.h"
#include "MantidDataObjects/EventWorkspace.h"
#include <filesystem>

using Mantid::Algorithms::GenerateGoniometerIndependentBackground;

class GenerateGoniometerIndependentBackgroundTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GenerateGoniometerIndependentBackgroundTest *createSuite() {
    return new GenerateGoniometerIndependentBackgroundTest();
  }
  static void destroySuite(GenerateGoniometerIndependentBackgroundTest *suite) { delete suite; }

  void setUp() override {
    createSyntheticWS(1000, "ws1");
    createSyntheticWS(2000, "ws2");
    createSyntheticWS(3000, "ws3");
    createSyntheticWS(4000, "ws4");
    createSyntheticWS(1000, "histogram", "Histogram");
    createSyntheticWS(4000, "highPC", "Event", 1, 10000., "100.0");
    createSyntheticWS(4000, "diffInstrument", "Event", 1, 10000., "10.0", "somethingDifferent");
    createSyntheticWS(4000, "diffNumHist", "Event", 2, 10000.);
    createSyntheticWS(4000, "diffNumBins", "Event", 1, 1000.);

    auto create = Mantid::API::AlgorithmManager::Instance().create("CreateGroupingWorkspace");
    create->initialize();
    create->setProperty("InputWorkspace", "ws1");
    create->setProperty("GroupDetectorsBy", "bank");
    create->setProperty("OutputWorkspace", "groups");
    create->execute();

    auto saveGrouping = Mantid::API::AlgorithmManager::Instance().create("SaveDetectorsGrouping");
    saveGrouping->initialize();
    saveGrouping->setProperty("InputWorkspace", "groups");
    saveGrouping->setProperty("OutputFile", "groups.xml");
    saveGrouping->execute();
  }

  void tearDown() override {
    Mantid::API::AnalysisDataService::Instance().clear();
    if (std::filesystem::exists("groups.xml"))
      std::filesystem::remove("groups.xml");
  }

  void test_exec() {
    // for reference the intensity in each bin for each workspace is
    // ws1 500
    // ws2 1000
    // ws3 1500
    // ws4 2000

    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 0., 1., 500.);     // ws1
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 0., 50., 750.);    // (ws1+ws2)/2
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 0., 75., 1000.);   // (ws1+ws2+ws3)/3
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 0., 100., 1250.);  // (ws1+ws2+ws3+ws4)/4
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 50., 100., 1750.); // (ws3+ws4)/2
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 99., 100., 2000.); // ws4
    runTest(std::vector<std::string>({"ws1", "ws2", "ws3", "ws4"}), 25., 75., 1250.);  // (ws2+ws3)/2
    runTest(std::vector<std::string>({"ws1", "ws2"}), 23., 24., 500.);                 // ws1
    runTest(std::vector<std::string>({"ws1", "ws2"}), 67., 68., 1000.);                // ws2
  }

  void test_input_workspace_number() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Requires at least 2 input workspaces")

    // single input workspace
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1"}));
    issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Requires at least 2 input workspaces")

    // two workspaces should have no issues
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"}));
    issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 0)
  }

  void test_input_different_proton_charge() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "highPC"}));

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Proton charge must not vary more than 1%")
  }

  void test_input_different_num_histograms() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "diffNumHist"}));

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Number of spectra mismatch.")
  }

  void test_input_different_num_bins() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "diffNumBins"}));

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Size mismatch.")
  }

  void test_input_different_instrument() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "diffInstrument"}));

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Instrument name mismatch.")
  }

  void test_input_not_event_workspace() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "histogram"}));

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 1)
    TS_ASSERT_EQUALS(issues["InputWorkspaces"], "Workspace \"histogram\" is not an EventWorkspace")
  }

  void test_min_greater_than_max() {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    alg.setProperty("InputWorkspaces", std::vector<std::string>({"ws1", "ws2"}));
    alg.setProperty("PercentMin", 75.);
    alg.setProperty("PercentMax", 25.);

    auto issues = alg.validateInputs();
    TS_ASSERT_EQUALS(issues.size(), 2)
    TS_ASSERT_EQUALS(issues["PercentMin"], "PercentMin must be less than PercentMax")
    TS_ASSERT_EQUALS(issues["PercentMax"], "PercentMin must be less than PercentMax")
  }

private:
  void createSyntheticWS(const int numEvents, const std::string wsname, const std::string workspaceType = "Event",
                         const int bankPixelWidth = 1, const double binWidth = 10000,
                         const std::string protonCharge = "10.0", const std::string instrumentName = "fake") const {
    auto create = Mantid::API::AlgorithmManager::Instance().create("CreateSampleWorkspace");
    create->initialize();
    create->setPropertyValue("WorkspaceType", workspaceType);
    create->setPropertyValue("Function", "Flat background");
    create->setProperty("NumBanks", 2);
    create->setProperty("BankPixelWidth", bankPixelWidth);
    create->setProperty("BinWidth", binWidth);
    create->setProperty("NumEvents", numEvents);
    create->setProperty("InstrumentName", instrumentName);
    create->setPropertyValue("OutputWorkspace", wsname);
    create->execute();

    auto addLog = Mantid::API::AlgorithmManager::Instance().create("AddSampleLog");
    addLog->initialize();
    addLog->setProperty("Workspace", wsname);
    addLog->setProperty("LogName", "gd_prtn_chrg");
    addLog->setProperty("LogText", protonCharge);
    addLog->setProperty("LogType", "Number");
    addLog->execute();
  }

  void runTest(const std::vector<std::string> inputWS, const double percentMin, const double percentMax,
               const double expectedResult) const {
    GenerateGoniometerIndependentBackground alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspaces", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("GroupingFile", "groups.xml"))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "result"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PercentMin", percentMin))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PercentMax", percentMax))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    Mantid::DataObjects::EventWorkspace_sptr result =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::EventWorkspace>("result");
    TS_ASSERT(result)
    TS_ASSERT_DELTA(result->readY(0).at(0), expectedResult, 1e-4)
  }
};
