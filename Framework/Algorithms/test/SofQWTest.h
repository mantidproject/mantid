#ifndef SOFQWTEST_H_
#define SOFQWTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAlgorithms/SofQW.h"
#include "MantidAlgorithms/SofQCommon.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidKernel/Unit.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataObjects/Workspace2D.h"

#include <boost/make_shared.hpp>

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

    Mantid::API::Workspace_sptr loadedWS =
        loader.getProperty("OutputWorkspace");
    auto inWS =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(loadedWS);
    WorkspaceHelpers::makeDistribution(inWS);
    return inWS;
  }

  template <typename SQWType>
  static Mantid::API::MatrixWorkspace_sptr
  runSQW(const std::string &method = "") {
    auto inWS = loadTestFile();

    SQWType sqw;
    sqw.initialize();
    sqw.setRethrows(true);
    // Cannot be marked as child or history is not recorded
    TS_ASSERT_THROWS_NOTHING(sqw.setProperty("InputWorkspace", inWS));
    const std::string wsname{"_tmp_"};
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("OutputWorkspace", wsname));
    TS_ASSERT_THROWS_NOTHING(
        sqw.setPropertyValue("QAxisBinning", "0.5,0.25,2"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EMode", "Indirect"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EFixed", "1.84"));
    TS_ASSERT_THROWS_NOTHING(sqw.setProperty("ReplaceNaNs", true));
    if (!method.empty())
      sqw.setPropertyValue("Method", method);
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

  void testExecWithDefaultMethodUsesSofQWCentre() {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQW>();

    TS_ASSERT(isAlgorithmInHistory(*result, "SofQWCentre"));

    TS_ASSERT_EQUALS(result->getAxis(0)->length(), 1904);
    TS_ASSERT_EQUALS(result->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_DELTA((*(result->getAxis(0)))(0), -0.5590, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(999), -0.0971, 0.0001);
    TS_ASSERT_DELTA((*(result->getAxis(0)))(1900), 0.5728, 0.0001);

    TS_ASSERT_EQUALS(result->getAxis(1)->length(), 7);
    TS_ASSERT_EQUALS(result->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(0), 0.5);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(3), 1.25);
    TS_ASSERT_EQUALS((*(result->getAxis(1)))(6), 2.0);

    const double delta(1e-08);
    TS_ASSERT_DELTA(result->readY(0)[1160], 54.85624399, delta);
    TS_ASSERT_DELTA(result->readE(0)[1160], 0.34252858, delta);
    TS_ASSERT_DELTA(result->readY(1)[1145], 22.72491806, delta);
    TS_ASSERT_DELTA(result->readE(1)[1145], 0.19867742, delta);
    TS_ASSERT_DELTA(result->readY(2)[1200], 6.76047436, delta);
    TS_ASSERT_DELTA(result->readE(2)[1200], 0.10863549, delta);
    TS_ASSERT_DELTA(result->readY(3)[99], 0.16439574, delta);
    TS_ASSERT_DELTA(result->readE(3)[99], 0.03414360, delta);
    TS_ASSERT_DELTA(result->readY(4)[1654], 0.069311442, delta);
    TS_ASSERT_DELTA(result->readE(4)[1654], 0.007573484, delta);
    TS_ASSERT_DELTA(result->readY(5)[1025], 0.226287179, delta);
    TS_ASSERT_DELTA(result->readE(5)[1025], 0.02148236, delta);
  }

  void testExecUsingDifferentMethodChoosesDifferentAlgorithm() {
    auto result = SofQWTest::runSQW<Mantid::Algorithms::SofQW>("Polygon");

    TS_ASSERT(isAlgorithmInHistory(*result, "SofQWPolygon"));
    // results are checked in the dedicated algorithm test
  }

  void testExecNansReplaced() {
    auto result =
        SofQWTest::runSQW<Mantid::Algorithms::SofQW>("NormalisedPolygon");
    bool nanFound = false;

    for (size_t i = 0; i < result->getNumberHistograms(); i++) {
      if (std::find_if(result->y(i).begin(), result->y(i).end(), [](double v) {
            return std::isnan(v);
          }) != result->y(i).end()) {
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
    const std::vector<double> expectedEBinEdges{-0.5, -0.4, -0.3, -0.2,
                                                -0.1, 0.1,  0.3,  0.4};
    const std::vector<double> qBinParams{0.5, 0.1, 1.0, 0.2, 2.};
    const std::vector<double> expectedQBinEdges{0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
                                                1.2, 1.4, 1.6, 1.8, 2.};
    std::vector<double> qAxis;
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<
        Mantid::DataObjects::Workspace2D>(*inWS, qBinParams, qAxis, eBinParams,
                                          emodeProperties);
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
    const std::vector<double> expectedQBinEdges{0.5, 0.75, 1., 1.25,
                                                1.5, 1.75, 2.};
    std::vector<double> qAxis;
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<
        Mantid::DataObjects::Workspace2D>(*inWS, qBinParams, qAxis, eBinParams,
                                          emodeProperties);
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
    auto outWS = Mantid::Algorithms::SofQW::setUpOutputWorkspace<
        Mantid::DataObjects::Workspace2D>(*inWS, qBinParams, qAxis, eBinParams,
                                          emodeProperties);
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
  static bool isAlgorithmInHistory(const Mantid::API::MatrixWorkspace &result,
                                   const std::string &name) {
    // Loaded nexus file has 13 other entries
    const auto &wsHistory = result.getHistory();
    const auto &lastAlg = wsHistory.getAlgorithmHistory(wsHistory.size() - 1);
    const auto child = lastAlg->getChildAlgorithmHistory(0);
    return (child->name() == name);
  }
};

#endif /*SOFQWTEST_H_*/
