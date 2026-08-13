// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/SofQCommon.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Unit.h"
#include <cxxtest/TestSuite.h>

#include <memory>

using namespace Mantid::API;

class SofQWTest : public CxxTest::TestSuite {
public:
  static Mantid::API::MatrixWorkspace_sptr loadTestFile() {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setChild(true);
    loader.setProperty("Filename", "IRS26173_ipg.nxs");
    loader.setPropertyValue("OutputWorkspace", "__unused");
    loader.execute();

    Mantid::API::Workspace_sptr loadedWS = loader.getProperty("OutputWorkspace");
    auto inWS = std::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(loadedWS);
    WorkspaceHelpers::makeDistribution(inWS);
    return inWS;
  }

  template <typename SQWType> static Mantid::API::MatrixWorkspace_sptr runSQW() {
    auto inWS = loadTestFile();

    SQWType sqw;
    sqw.initialize();
    sqw.setRethrows(true);
    // Cannot be marked as child or history is not recorded
    TS_ASSERT_THROWS_NOTHING(sqw.setProperty("InputWorkspace", inWS));
    const std::string wsname{"_tmp_"};
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("OutputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("QAxisBinning", "0.5,0.25,2"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EFixed", "1.84"));
    TS_ASSERT_THROWS_NOTHING(sqw.setProperty("ReplaceNaNs", true));
    TS_ASSERT_THROWS_NOTHING(sqw.execute());
    TS_ASSERT(sqw.isExecuted());

    auto &dataStore = Mantid::API::AnalysisDataService::Instance();
    auto result = dataStore.retrieveWS<Mantid::API::MatrixWorkspace>(wsname);
    dataStore.remove(wsname);

    return result;
  }

  void testName() {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_EQUALS(sqw.name(), "SofQW");
  }

  void testVersion() {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_EQUALS(sqw.version(), 1);
  }

  void testInit() {
    Mantid::Algorithms::SofQW sqw;
    TS_ASSERT_THROWS_NOTHING(sqw.initialize());
    TS_ASSERT(sqw.isInitialized());
  }

  void testExecNansReplaced() {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQW>();
    bool nanFound = false;

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      if (std::find_if(result->y(i).begin(), result->y(i).end(), [](double v) { return std::isnan(v); }) !=
          result->y(i).end()) {
        nanFound = true;
        break; // NaN found in workspace, no need to keep searching
      }
    }

    TS_ASSERT(!nanFound);
  }

  void testSetUpOutputWorkspace() {
    auto inWS = loadTestFile();
    Mantid::Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    alg.setProperty("EFixed", 1.84);
    Mantid::Algorithms::SofQCommon emodeProperties;
    emodeProperties.initCachedValues(*inWS, &alg);
    const std::vector<double> eBinParams{-0.5, 0.1, -0.1, 0.2, 0.4};
    const std::vector<double> expectedEBinEdges{-0.5, -0.4, -0.3, -0.2, -0.1, 0.1, 0.3, 0.4};
    const std::vector<double> qBinParams{0.5, 0.1, 1.0, 0.2, 2.};
    const std::vector<double> expectedQBinEdges{0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.2, 1.4, 1.6, 1.8, 2.};
    std::vector<double> qAxis;
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<Mantid::DataObjects::Workspace2D>(
        *inWS, qBinParams, qAxis, eBinParams, emodeProperties);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), expectedQBinEdges.size() - 1)
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      const auto &x = outWS->x(i);
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_DELTA(x[j], expectedEBinEdges[j], 1e-12)
      }
    }
    const auto axis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis->length(), qAxis.size())
    TS_ASSERT_EQUALS(qAxis.size(), expectedQBinEdges.size())
    for (size_t i = 0; i < qAxis.size(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), qAxis[i])
      TS_ASSERT_DELTA(qAxis[i], expectedQBinEdges[i], 1e-12)
    }
  }

  void testSetUpOutputWorkspaceWithEnergyBinWidthOnly() {
    auto inWS = loadTestFile();

    Mantid::Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    alg.setProperty("EFixed", 1.84);
    Mantid::Algorithms::SofQCommon emodeProperties;
    emodeProperties.initCachedValues(*inWS, &alg);
    const double dE{0.3};
    const std::vector<double> eBinParams{dE};
    std::vector<double> expectedEBinEdges;
    const auto firstEdge = inWS->x(0).front();
    const auto lastEdge = inWS->x(0).back();
    auto currentEdge = firstEdge;
    while (currentEdge < lastEdge) {
      expectedEBinEdges.emplace_back(currentEdge);
      currentEdge += dE;
    }
    expectedEBinEdges.emplace_back(lastEdge);
    const std::vector<double> qBinParams{0.5, 0.25, 2.};
    const std::vector<double> expectedQBinEdges{0.5, 0.75, 1., 1.25, 1.5, 1.75, 2.};
    std::vector<double> qAxis;
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<Mantid::DataObjects::Workspace2D>(
        *inWS, qBinParams, qAxis, eBinParams, emodeProperties);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), expectedQBinEdges.size() - 1)
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      const auto &x = outWS->x(i);
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_DELTA(x[j], expectedEBinEdges[j], 1e-12)
      }
    }
    const auto axis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis->length(), qAxis.size())
    TS_ASSERT_EQUALS(qAxis.size(), expectedQBinEdges.size())
    for (size_t i = 0; i < qAxis.size(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), qAxis[i])
      TS_ASSERT_DELTA(qAxis[i], expectedQBinEdges[i], 1e-12)
    }
  }

  void testSetUpOutputWorkspaceWithQBinWidthOnly() {
    auto inWS = loadTestFile();

    Mantid::Algorithms::SofQW alg;
    alg.initialize();
    alg.setProperty("EMode", "Indirect");
    alg.setProperty("EFixed", 1.84);
    Mantid::Algorithms::SofQCommon emodeProperties;
    emodeProperties.initCachedValues(*inWS, &alg);
    const std::vector<double> eBinParams{-0.3, 0.2, 0.5};
    const std::vector<double> expectedEBinEdges{-0.3, -0.1, 0.1, 0.3, 0.5};
    const double dQ{0.023};
    const std::vector<double> qBinParams{dQ};
    std::vector<double> qAxis;
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<Mantid::DataObjects::Workspace2D>(
        *inWS, qBinParams, qAxis, eBinParams, emodeProperties);
    for (size_t i = 0; i < outWS->getNumberHistograms(); ++i) {
      const auto &x = outWS->x(i);
      for (size_t j = 0; j < x.size(); ++j) {
        TS_ASSERT_DELTA(x[j], expectedEBinEdges[j], 1e-12)
      }
    }
    const auto axis = outWS->getAxis(1);
    TS_ASSERT_EQUALS(axis->length(), qAxis.size())
    for (size_t i = 0; i < qAxis.size(); ++i) {
      TS_ASSERT_EQUALS(axis->getValue(i), qAxis[i])
    }
    // Test only the Q bin width, not the actual edges.
    for (size_t i = 0; i < qAxis.size() - 1; ++i) {
      const auto delta = qAxis[i + 1] - qAxis[i];
      TS_ASSERT_DELTA(delta, dQ, 1e-12);
    }
  }

private:
  static bool isAlgorithmInHistory(const Mantid::API::MatrixWorkspace &result, const std::string &name) {
    // Loaded nexus file has 13 other entries
    const auto &wsHistory = result.getHistory();
    const auto &lastAlg = wsHistory.getAlgorithmHistory(wsHistory.size() - 1);
    const auto child = lastAlg->getChildAlgorithmHistory(0);
    return (child->name() == name);
  }
};
