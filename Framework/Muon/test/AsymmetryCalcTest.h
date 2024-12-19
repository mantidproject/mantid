// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidMuon/AsymmetryCalc.h"
#include <algorithm>
#include <stdexcept>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

/**
 * This is a test class that exists to test the method validateInputs()
 */
class TestAsymmetryCalc : public Mantid::Algorithms::AsymmetryCalc {
public:
  std::map<std::string, std::string> wrapValidateInputs() { return this->validateInputs(); }
};

class AsymmetryCalcTest : public CxxTest::TestSuite {
public:
  void testName() {
    AsymmetryCalc asymCalc;
    TS_ASSERT_EQUALS(asymCalc.name(), "AsymmetryCalc")
  }

  void testCategory() {
    AsymmetryCalc asymCalc;
    TS_ASSERT_EQUALS(asymCalc.category(), "Muon")
  }

  void testInit() {
    AsymmetryCalc asymCalc;
    asymCalc.initialize();
    TS_ASSERT(asymCalc.isInitialized())
  }

  void testProperties() {
    AsymmetryCalc asymCalc;
    asymCalc.initialize();
    asymCalc.setProperty("Alpha", "1.0");
    TS_ASSERT_EQUALS(asymCalc.getPropertyValue("Alpha"), "1");
  }

  void testExecuteOnDataFile() {
    auto const workspace = loadFile("emu00006473.nxs");
    AsymmetryCalc asymCalc;
    try {
      asymCalc.initialize();
      asymCalc.setChild(true);
      asymCalc.setProperty("InputWorkspace", workspace);
      asymCalc.setPropertyValue("Alpha", "1.0");
      asymCalc.setPropertyValue("ForwardSpectra", "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
      asymCalc.setPropertyValue("BackwardSpectra", "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32");
      TS_ASSERT_EQUALS(asymCalc.execute(), true);
    } catch (std::runtime_error &err) {
      TS_FAIL(err.what());
    }

    // Check the result
    MatrixWorkspace_const_sptr outputWS = asymCalc.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(outputWS->y(0)[100], 0.2965, 0.005);
    TS_ASSERT(!outputWS->isHistogramData());
  }

  void test_single_spectra() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(3, 10);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto &y = ws->mutableY(i);
      std::fill(y.begin(), y.end(), static_cast<double>(i + 1));
    }

    AsymmetryCalc alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "__Unused");
    alg.setPropertyValue("ForwardSpectra", "1");
    alg.setPropertyValue("BackwardSpectra", "3");
    alg.execute();

    MatrixWorkspace_const_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(outputWS->y(0)[0], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT_EQUALS(outputWS->y(0)[6], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT_EQUALS(outputWS->y(0)[9], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT(!outputWS->isHistogramData());
  }

  void test_yUnitLabel() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(2, 1);

    AsymmetryCalc alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", ws);
    alg.setPropertyValue("OutputWorkspace", "__Unused");
    alg.execute();

    MatrixWorkspace_const_sptr result = alg.getProperty("OutputWorkspace");

    TS_ASSERT(result);

    if (result) {
      TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
    }
  }

  void test_validateInputs() {
    auto ws = WorkspaceCreationHelper::create2DWorkspace(2, 1);
    AsymmetryCalc asymCalc;
    asymCalc.initialize();
    asymCalc.setChild(true);
    asymCalc.setProperty("InputWorkspace", ws);
    asymCalc.setPropertyValue("OutputWorkspace", "__Unused");
    asymCalc.setPropertyValue("ForwardSpectra", "1");
    asymCalc.setPropertyValue("BackwardSpectra", "3");
    // Bad spectrum number for BackwardSpectra
    TS_ASSERT_THROWS(asymCalc.execute(), const std::runtime_error &);
    asymCalc.setPropertyValue("BackwardSpectra", "1");
    asymCalc.setPropertyValue("ForwardSpectra", "3");
    // Bad spectrum number for ForwardSpectra
    TS_ASSERT_THROWS(asymCalc.execute(), const std::runtime_error &);
  }

  /**
   * Test that the algorithm can handle a WorkspaceGroup as input without
   * crashing
   * We have to use the ADS to test WorkspaceGroups
   */
  void testValidateInputsWithWSGroup() {
    auto ws1 = std::static_pointer_cast<Workspace>(WorkspaceCreationHelper::create2DWorkspace(2, 1));
    auto ws2 = std::static_pointer_cast<Workspace>(WorkspaceCreationHelper::create2DWorkspace(2, 1));
    AnalysisDataService::Instance().add("workspace1", ws1);
    AnalysisDataService::Instance().add("workspace2", ws2);
    auto group = std::make_shared<WorkspaceGroup>();
    AnalysisDataService::Instance().add("group", group);
    group->add("workspace1");
    group->add("workspace2");
    TestAsymmetryCalc calc;
    calc.initialize();
    calc.setChild(true);
    TS_ASSERT_THROWS_NOTHING(calc.setPropertyValue("InputWorkspace", "group"));
    calc.setPropertyValue("OutputWorkspace", "__Unused");
    calc.setPropertyValue("ForwardSpectra", "1");
    calc.setPropertyValue("BackwardSpectra", "2");
    TS_ASSERT_THROWS_NOTHING(calc.wrapValidateInputs());
    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr loadFile(std::string const &filename) {
    Mantid::DataHandling::Load loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("Filename", filename);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT_EQUALS(loader.isExecuted(), true);

    TS_ASSERT_EQUALS("LoadMuonNexus", loader.getPropertyValue("LoaderName"));
    TS_ASSERT_EQUALS("1", loader.getPropertyValue("LoaderVersion"));

    Workspace_sptr outWS = loader.getProperty("OutputWorkspace");
    return std::dynamic_pointer_cast<MatrixWorkspace>(outWS);
  }
};
