// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAlgorithms/FindPeaksConvolve.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::FindPeaksConvolve;

namespace {
/// Load focussed data file - from FindPeaksTest
void loadNexusProcessed(const std::string &filename, const std::string &wsname) {
  Mantid::DataHandling::LoadNexusProcessed loader;
  loader.initialize();
  loader.setProperty("Filename", filename);
  loader.setProperty("OutputWorkspace", wsname);
  loader.execute();
  TS_ASSERT(loader.isExecuted());
  TS_ASSERT(Mantid::API::AnalysisDataService::Instance().doesExist(wsname));
}

Mantid::API::Algorithm_sptr set_up_alg(const std::string &input_ws_name, const std::string &output_ws_name) {
  auto alg = Mantid::API::AlgorithmManager::Instance().createUnmanaged("FindPeaksConvolve");
  // auto alg = Mantid::API::Algorithm::createAlgorithm("FindPeaksConvolve");
  // Don't put output in ADS by default
  alg->setChild(true);
  TS_ASSERT_THROWS_NOTHING(alg->initialize());
  TS_ASSERT(alg->isInitialized())
  TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", input_ws_name));
  TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", output_ws_name));
  return alg;
}

const std::string INPUT_TEST_WS_NAME = "FindPeaksTest_peaksWS"; // Data reused from FindPeaksTest
const std::string OUTPUT_TEST_WS_NAME = "FindPeaksConvolve_final_output";
} // anonymous namespace

class FindPeaksConvolveTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FindPeaksConvolveTest *createSuite() { return new FindPeaksConvolveTest(); }
  static void destroySuite(FindPeaksConvolveTest *suite) { delete suite; }

  void setUp() override {
    // Load data file into ADS once
    if (!Mantid::API::AnalysisDataService::Instance().doesExist(INPUT_TEST_WS_NAME)) {
      loadNexusProcessed("focussed.nxs", INPUT_TEST_WS_NAME);
    }
  }

  void test_exec() {
    // auto alg = set_up_alg(INPUT_TEST_WS_NAME, OUTPUT_TEST_WS_NAME);
    int64_t nhist = 1;
    int64_t nbins = 17;
    bool ishist = false;
    double xval(0), yval(0), eval(0), dxval(1);
    std::string wsName = "test_ws";

    Mantid::API::MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
        WorkspaceCreationHelper::create2DWorkspaceWithValuesAndXerror(nhist, nbins, ishist, xval, yval, eval, dxval));
    std::vector<double> yValues{1, 1, 1, 1, 1.5, 2, 3, 5, 8, 5, 3, 2, 1.5, 1, 1, 1, 1};

    for (std::size_t j = 0; j < yValues.size(); ++j) {
      ws->dataY(0)[j] = yValues[j];
      ws->dataX(0)[j] = j;
      ws->dataE(0)[j] = 1;
    }

    Mantid::API::AnalysisDataService::Instance().add(wsName, ws);

    auto alg = set_up_alg(wsName, OUTPUT_TEST_WS_NAME);
    alg->setProperty("EstimatedPeakExtent", "5");
    TS_ASSERT_THROWS_NOTHING(alg->execute(););
    TS_ASSERT(alg->isExecuted());
  }

  void test_Something() { TS_FAIL("You forgot to write a test!"); }
};