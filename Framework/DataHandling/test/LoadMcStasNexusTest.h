// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <fstream>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMcStasNexus.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this
// test case.
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

//
// test does:
//
class LoadMcStasNexusTest : public CxxTest::TestSuite {
public:
  void testInit() {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void testExec() {
    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    outputSpace = "LoadMcStasNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // Now set it...
    // specify name of file to load workspace from
    inputFile = "mcstas.h5";
    algToBeTested.setPropertyValue("Filename", inputFile);

    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());
    //
    //  test workspace created by LoadMcStasNexus
    WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS(output->getNumberOfEntries(), 4);
    // int ii;
    // std::cin >> ii;
    MatrixWorkspace_sptr outputItem1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_1");
    TS_ASSERT_EQUALS(outputItem1->getNumberHistograms(), 1);
    MatrixWorkspace_sptr outputItem2 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace + "_2");
    TS_ASSERT_EQUALS(outputItem2->getNumberHistograms(), 128);

    AnalysisDataService::Instance().remove(outputSpace + "_1");
    AnalysisDataService::Instance().remove(outputSpace + "_2");
  }

  void test_run_via_load() {
    // We are verifying that the confidence information provided by the loader is good
    std::string inputFile = "mcstas.h5";
    Load loader;
    loader.initialize();
    loader.setChild(true);
    loader.setProperty("Filename", inputFile);
    loader.setPropertyValue("OutputWorkspace", "dummy");
    TS_ASSERT_EQUALS(loader.getPropertyValue("LoaderName"), "LoadMcStasNexus");
    loader.execute();
    Workspace_sptr out = loader.getProperty("OutputWorkspace");
    auto asMatrixOut = std::dynamic_pointer_cast<MatrixWorkspace>(out);
  }

  void test_cannot_run_via_load() {
    // We are verifying that the confidence information provided by the loader is idenfiying unsuitable files
    std::string inputFile = "POLREF00014966.nxs";
    Load loader;
    loader.initialize();
    loader.setChild(true);
    loader.setProperty("Filename", inputFile);
    TS_ASSERT_DIFFERS(loader.getPropertyValue("LoaderName"), "LoadMcStasNexus");
  }

private:
  LoadMcStasNexus algToBeTested;
  std::string inputFile;
  std::string outputSpace;
};

class LoadMcStasNexusTestPerformance : public CxxTest::TestSuite {
public:
  static LoadMcStasNexusTestPerformance *createSuite() { return new LoadMcStasNexusTestPerformance(); }

  static void destroySuite(LoadMcStasNexusTestPerformance *suite) { delete suite; }

  void setUp() override {
    if (!loadMcStasNexusAlg.isInitialized())
      loadMcStasNexusAlg.initialize();

    outputSpace = "LoadMcStasNexusTest";
    loadMcStasNexusAlg.setPropertyValue("OutputWorkspace", outputSpace);

    inputFile = "mcstas.h5";
    loadMcStasNexusAlg.setPropertyValue("Filename", inputFile);
  }

  void tearDown() override {
    AnalysisDataService::Instance().remove("LoadMcStasNexusTest_1");
    AnalysisDataService::Instance().remove("LoadMcStasNexusTest_2");
  }

  void testExec() { loadMcStasNexusAlg.execute(); }

private:
  LoadMcStasNexus loadMcStasNexusAlg;
  std::string inputFile;
  std::string outputSpace;
};
