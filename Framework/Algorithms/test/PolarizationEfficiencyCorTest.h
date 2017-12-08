#ifndef MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_
#define MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarizationEfficiencyCor.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

using Mantid::Algorithms::PolarizationEfficiencyCor;

class PolarizationEfficiencyCorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationEfficiencyCorTest *createSuite() { return new PolarizationEfficiencyCorTest(); }
  static void destroySuite( PolarizationEfficiencyCorTest *suite ) { delete suite; }

  void tearDown() override {
    using namespace Mantid::API;
    AnalysisDataService::Instance().clear();
  }

  void test_Init() {
    PolarizationEfficiencyCor alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }

  void test_IdealCaseFullCorrections() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nBins{3};
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    const double yVal = 2.3;
    Counts counts{yVal, yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = ws00->clone();
    MatrixWorkspace_sptr ws10 = ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws01));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws10));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    for (size_t i = 0; i != 4; ++i) {
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS->getItem(i));
      for (size_t j = 0; j != nHist; ++j) {
        ws->mutableY(j) *= static_cast<double>(i + 1);
        ws->mutableE(j) *= static_cast<double>(i + 1);
      }
    }
    auto effWS = efficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4)
    const std::array<std::string, 4> POL_DIRS{{"++", "+-", "-+", "--"}};
    for (size_t i = 0; i != 4; ++i) {
      const std::string wsName = OUTWS_NAME + std::string("_") + POL_DIRS[i];
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(wsName));
      TS_ASSERT(ws)
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t j = 0; j != nHist; ++j) {
        const auto &xs = ws->x(j);
        const auto &ys = ws->y(j);
        const auto &es = ws->e(j);
        TS_ASSERT_EQUALS(ys.size(), nBins)
        for (size_t k = 0; k != nBins; ++k) {
          TS_ASSERT_EQUALS(xs[k], edges[k])
          TS_ASSERT_EQUALS(ys[k], yVal * static_cast<double>(i + 1))
          TS_ASSERT_EQUALS(es[k], std::sqrt(yVal) * static_cast<double>(i + 1))
        }
      }
    }
  }

  void test_IdealCaseThreeInputs10Missing() {
    threeInputsTest("10");
  }

  void test_IdealCaseThreeInputs01Missing() {
    threeInputsTest("01");
  }

  void test_IdealCaseTwoInputsWithAnalyzer() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nBins{3};
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    const double yVal = 2.3;
    Counts counts{yVal, yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws11 = ws00->clone();
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    for (size_t i = 0; i != nHist; ++i) {
      ws11->mutableY(i) *= 2.;
      ws11->mutableE(i) *= 2.;
    }

    auto effWS = efficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Flippers", "00, 11"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4)
    const std::array<std::string, 4> POL_DIRS{{"++", "+-", "-+", "--"}};
    for (size_t i = 0; i != 4; ++i) {
      const auto &dir = POL_DIRS[i];
      const std::string wsName = OUTWS_NAME + std::string("_") + dir;
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(wsName));
      TS_ASSERT(ws)
      const double expected = [yVal, &dir]() {
        if (dir == "++") {
          return yVal;
        } else if (dir == "--") {
          return 2. * yVal;
        } else {
          return 0.;
        }
      }();
      const double expectedError = [yVal, &dir]() {
        if (dir == "++") {
          return std::sqrt(yVal);
        } else if (dir == "--") {
          return 2. * std::sqrt(yVal);
        } else {
            return 0.;
        }
      }();
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t j = 0; j != nHist; ++j) {
        const auto &xs = ws->x(j);
        const auto &ys = ws->y(j);
        const auto &es = ws->e(j);
        TS_ASSERT_EQUALS(ys.size(), nBins)
        for (size_t k = 0; k != nBins; ++k) {
          TS_ASSERT_EQUALS(xs[k], edges[k])
          TS_ASSERT_EQUALS(ys[k], expected)
          TS_ASSERT_EQUALS(es[k], expectedError)
        }
      }
    }
  }

private:
  Mantid::API::MatrixWorkspace_sptr efficiencies(const Mantid::HistogramData::BinEdges &edges) {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    const auto nBins = edges.size() - 1;
    constexpr size_t nHist{4};
    Counts counts(nBins, 0.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(nHist, Histogram(edges, counts));
    ws->mutableY(0) = 1.;
    ws->mutableY(1) = 1.;
    auto axis = make_unique<TextAxis>(4);
    axis->setLabel(0, "F1");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    ws->replaceAxis(1, axis.release());
    return ws;
  }

  void threeInputsTest(const std::string &missingFlipperConf) {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nBins{3};
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    const double yVal = 2.3;
    Counts counts{yVal, yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr wsXX = ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(wsXX));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    for (size_t i = 0; i != 3; ++i) {
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(inputWS->getItem(i));
      for (size_t j = 0; j != nHist; ++j) {
        ws->mutableY(j) *= static_cast<double>(i + 1);
        ws->mutableE(j) *= static_cast<double>(i + 1);
      }
    }
    auto effWS = efficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    const std::string presentFlipperConf = missingFlipperConf == "01" ? "10" : "01";
    const std::string flipperConf = "00, " + presentFlipperConf + ", 11";
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Flippers", flipperConf))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4)
    const std::array<std::string, 4> POL_DIRS{{"++", "+-", "-+", "--"}};
    for (size_t i = 0; i != 4; ++i) {
      const auto &dir = POL_DIRS[i];
      const std::string wsName = OUTWS_NAME + std::string("_") + dir;
      MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(wsName));
      TS_ASSERT(ws)
      const double expected = [yVal, &dir]() {
        if (dir == "++") {
          return yVal;
        } else if (dir == "--") {
          return 3. * yVal;
        } else {
          return 2. * yVal;
        }
      }();
      const double expectedError = [yVal, &dir, &missingFlipperConf]() {
        if (dir == "++") {
          return std::sqrt(yVal);
        } else if (dir == "--") {
          return 3. * std::sqrt(yVal);
        } else {
          std::string conf = std::string(dir.front() == '+' ? "0" : "1") + std::string(dir.back() == '+' ? "0" : "1");
          if (conf != missingFlipperConf) {
            return 2. * std::sqrt(yVal);
          } else {
            return 0.;
          }
        }
      }();
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t j = 0; j != nHist; ++j) {
        const auto &xs = ws->x(j);
        const auto &ys = ws->y(j);
        const auto &es = ws->e(j);
        TS_ASSERT_EQUALS(ys.size(), nBins)
        for (size_t k = 0; k != nBins; ++k) {
          TS_ASSERT_EQUALS(xs[k], edges[k])
          TS_ASSERT_EQUALS(ys[k], expected)
          TS_ASSERT_EQUALS(es[k], expectedError)
        }
      }
    }
  }
};


#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_ */
