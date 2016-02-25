#ifndef ASYMMETRYCALCTEST_H_
#define ASYMMETRYCALCTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "MantidDataHandling/LoadMuonNexus2.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/GroupDetectors.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAlgorithms/AsymmetryCalc.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include <stdexcept>
#include <algorithm>

using namespace Mantid::Algorithms;
using namespace Mantid::API;

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
    MatrixWorkspace_sptr data;
    TS_ASSERT_THROWS_NOTHING(data = loadDataFile());
    TS_ASSERT(data);
    AsymmetryCalc asymCalc;
    try {
      asymCalc.initialize();
      asymCalc.setChild(true);
      asymCalc.setProperty("InputWorkspace", data);
      asymCalc.setPropertyValue("OutputWorkspace", "__Unused");
      asymCalc.setPropertyValue("Alpha", "1.0");
      asymCalc.setPropertyValue("ForwardSpectra",
                                "1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
      asymCalc.setPropertyValue(
          "BackwardSpectra", "17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32");
      TS_ASSERT_EQUALS(asymCalc.execute(), true);
    } catch (std::runtime_error &err) {
      TS_FAIL(err.what());
    }

    // Check the result
    MatrixWorkspace_const_sptr outputWS =
        asymCalc.getProperty("OutputWorkspace");
    TS_ASSERT_DELTA(outputWS->dataY(0)[100], 0.2965, 0.005);
    TS_ASSERT(!outputWS->isHistogramData());
  }

  void test_single_spectra() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(3, 10);
    for (size_t i = 0; i < ws->getNumberHistograms(); ++i) {
      auto &y = ws->dataY(i);
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
    TS_ASSERT_EQUALS(outputWS->readY(0)[0], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT_EQUALS(outputWS->readY(0)[6], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT_EQUALS(outputWS->readY(0)[9], -0.5); // == (1 - 3)/(1 + 3)
    TS_ASSERT(!outputWS->isHistogramData());
  }

  void test_yUnitLabel() {
    auto ws = WorkspaceCreationHelper::Create2DWorkspace(2, 1);

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

private:
  /// Load data from file
  MatrixWorkspace_sptr loadDataFile() {
    Mantid::DataHandling::LoadMuonNexus2 loader;
    loader.initialize();
    loader.setChild(true);
    loader.setPropertyValue("Filename", "emu00006473.nxs");
    loader.setPropertyValue("OutputWorkspace", "__Unused");
    loader.execute();
    if (loader.isExecuted()) {
      Workspace_sptr outWS = loader.getProperty("OutputWorkspace");
      auto matrixWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outWS);
      if (matrixWS) {
        return matrixWS;
      } else {
        throw std::runtime_error("Failed to cast loaded workspace");
      }
    } else {
      throw std::runtime_error("Failed to load test data file");
    }
  }
};

#endif /*ASYMMETRYCALCTEST_H_*/
