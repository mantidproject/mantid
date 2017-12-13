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

#include <Eigen/Dense>

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
    auto effWS = idealEfficiencies(edges);
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

    auto effWS = idealEfficiencies(edges);
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

  void test_IdealCaseTwoInputsNoAnalyzer() {
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
    auto effWS = idealEfficiencies(edges);
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
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Analyzer", false))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 2)
    const std::array<std::string, 2> POL_DIRS{{"++", "--"}};
    for (size_t i = 0; i != 2; ++i) {
      const auto &dir = POL_DIRS[i];
      const std::string wsName = OUTWS_NAME + std::string("_") + dir;
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

  void test_IdealCaseDirectBeamCorrections() {
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
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    auto effWS = idealEfficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Flippers", "00"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())
    WorkspaceGroup_sptr outputWS = alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 1)
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(OUTWS_NAME + std::string("_++")));
    TS_ASSERT(ws)
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
    for (size_t i = 0; i != nHist; ++i) {
      const auto &xs = ws->x(i);
      const auto &ys = ws->y(i);
      const auto &es = ws->e(i);
      TS_ASSERT_EQUALS(ys.size(), nBins)
      for (size_t j = 0; j != nBins; ++j) {
        TS_ASSERT_EQUALS(xs[j], edges[j])
        TS_ASSERT_EQUALS(ys[j], yVal)
        TS_ASSERT_EQUALS(es[j], std::sqrt(yVal))
      }
    }
  }

  void test_FullCorrections() {
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
    const double F1 = effWS->y(0).front();
    const double F1e = effWS->e(0).front();
    const double F2 = effWS->y(1).front();
    const double F2e = effWS->e(1).front();
    const double P1 = effWS->y(2).front();
    const double P1e = effWS->e(2).front();
    const double P2 = effWS->y(3).front();
    const double P2e = effWS->e(3).front();
    const Eigen::Vector4d y{ws00->y(0).front(), ws01->y(0).front(), ws10->y(0).front(), ws11->y(0).front()};
    const auto expected = correction(y, F1, F2, P1, P2);
    const Eigen::Vector4d e{ws00->e(0).front(), ws01->e(0).front(), ws10->e(0).front(), ws11->e(0).front()};
    const auto expectedError = error(y, e, F1, F1e, F2, F2e, P1, P1e, P2, P2e);
    MatrixWorkspace_sptr ppWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(OUTWS_NAME + std::string("_++")));
    MatrixWorkspace_sptr pmWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(OUTWS_NAME + std::string("_+-")));
    MatrixWorkspace_sptr mpWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(OUTWS_NAME + std::string("_-+")));
    MatrixWorkspace_sptr mmWS = boost::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(OUTWS_NAME + std::string("_--")));
    TS_ASSERT(ppWS)
    TS_ASSERT(pmWS)
    TS_ASSERT(mpWS)
    TS_ASSERT(mmWS)
    TS_ASSERT_EQUALS(ppWS->getNumberHistograms(), nHist)
    TS_ASSERT_EQUALS(pmWS->getNumberHistograms(), nHist)
    TS_ASSERT_EQUALS(mpWS->getNumberHistograms(), nHist)
    TS_ASSERT_EQUALS(mmWS->getNumberHistograms(), nHist)
    for (size_t j = 0; j != nHist; ++j) {
      const auto &ppX = ppWS->x(j);
      const auto &ppY = ppWS->y(j);
      const auto &ppE = ppWS->e(j);
      const auto &pmX = pmWS->x(j);
      const auto &pmY = pmWS->y(j);
      const auto &pmE = pmWS->e(j);
      const auto &mpX = mpWS->x(j);
      const auto &mpY = mpWS->y(j);
      const auto &mpE = mpWS->e(j);
      const auto &mmX = mmWS->x(j);
      const auto &mmY = mmWS->y(j);
      const auto &mmE = mmWS->e(j);
      TS_ASSERT_EQUALS(ppY.size(), nBins)
      TS_ASSERT_EQUALS(pmY.size(), nBins)
      TS_ASSERT_EQUALS(mpY.size(), nBins)
      TS_ASSERT_EQUALS(mmY.size(), nBins)
      for (size_t k = 0; k != nBins; ++k) {
        TS_ASSERT_EQUALS(ppX[k], edges[k])
        TS_ASSERT_EQUALS(pmX[k], edges[k])
        TS_ASSERT_EQUALS(mpX[k], edges[k])
        TS_ASSERT_EQUALS(mmX[k], edges[k])
        TS_ASSERT_DELTA(ppY[k], expected[0], 1e-12)
        TS_ASSERT_DELTA(pmY[k], expected[1], 1e-12)
        TS_ASSERT_DELTA(mpY[k], expected[2], 1e-12)
        TS_ASSERT_DELTA(mmY[k], expected[3], 1e-12)
        TS_ASSERT_DELTA(ppE[k], expectedError[0], 1e-12)
        TS_ASSERT_DELTA(pmE[k], expectedError[1], 1e-12)
        TS_ASSERT_DELTA(mpE[k], expectedError[2], 1e-12)
        TS_ASSERT_DELTA(mmE[k], expectedError[3], 1e-12)
      }
    }
  }

  void test_FailureWhenEfficiencyHistogramIsMissing() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    Counts counts{0., 0., 0.};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(1, Histogram(edges, counts));
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    auto effWS = idealEfficiencies(edges);
    // Rename F1 to something else.
    auto axis = make_unique<TextAxis>(4);
    axis->setLabel(0, "__wrong_histogram_label");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    effWS->replaceAxis(1, axis.release());
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Flippers", "00"))
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error)
    TS_ASSERT(!alg.isExecuted())
  }

  void test_FailureWhenEfficiencyXDataMismatches() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    Counts counts{0., 0., 0.};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(1, Histogram(edges, counts));
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    auto effWS = idealEfficiencies(edges);
    // Change a bin edge of one of the histograms.
    auto &xs = effWS->mutableX(0);
    xs[xs.size() / 2] *= 1.01;
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Flippers", "00"))
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error)
    TS_ASSERT(!alg.isExecuted())
  }

  void test_FailureWhenNumberOfHistogramsInInputWorkspacesMismatch() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    Counts counts{0., 0., 0.};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = ws00->clone();
    MatrixWorkspace_sptr ws10 = create<Workspace2D>(nHist + 1, Histogram(edges, counts));
    MatrixWorkspace_sptr ws11 = ws00->clone();
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws01));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws10));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    auto effWS = idealEfficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error)
    TS_ASSERT(!alg.isExecuted())
  }

  void test_FailureWhenAnInputWorkspaceIsMissing() {
    using namespace Mantid::API;
    using namespace Mantid::DataObjects;
    using namespace Mantid::HistogramData;
    using namespace Mantid::Kernel;
    constexpr size_t nHist{2};
    BinEdges edges{0.3, 0.6, 0.9, 1.2};
    Counts counts{0., 0., 0.};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();
    WorkspaceGroup_sptr inputWS = boost::make_shared<WorkspaceGroup>();
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws00));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws01));
    inputWS->addWorkspace(boost::dynamic_pointer_cast<Workspace>(ws11));
    auto effWS = idealEfficiencies(edges);
    constexpr char *OUTWS_NAME{"output"};
    PolarizationEfficiencyCor alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS))
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", OUTWS_NAME))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Efficiencies", effWS))
    TS_ASSERT_THROWS(alg.execute(), std::runtime_error)
    TS_ASSERT(!alg.isExecuted())
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
    ws->mutableY(0) = 0.95;
    ws->mutableE(0) = 0.01;
    ws->mutableY(1) = 0.92;
    ws->mutableE(1) = 0.02;
    ws->mutableY(2) = 0.05;
    ws->mutableE(2) = 0.015;
    ws->mutableY(3) = 0.04;
    ws->mutableE(3) = 0.03;
    auto axis = make_unique<TextAxis>(4);
    axis->setLabel(0, "F1");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    ws->replaceAxis(1, axis.release());
    return ws;
  }

  Mantid::API::MatrixWorkspace_sptr idealEfficiencies(const Mantid::HistogramData::BinEdges &edges) {
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
    auto effWS = idealEfficiencies(edges);
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

  void invertedF1(Eigen::Matrix4d &m, const double f1) {
    m << f1, 0., 0., 0.,
         0., f1, 0., 0.,
         f1 - 1., 0., 1., 0.,
         0., f1 - 1., 0., 1.;
    m *= 1. / f1;
  }

  void invertedF1Derivative(Eigen::Matrix4d &m, const double f1) {
    m << 0., 0., 0., 0.,
         0., 0., 0., 0.,
         1., 0., -1., 0.,
         0., 1., 0., -1.;
    m *= 1. / (f1 * f1);
  }

  void invertedF2(Eigen::Matrix4d &m, const double f2) {
    m << f2, 0., 0., 0.,
         f2 - 1., 1., 0., 0.,
         0., 0., f2, 0.,
         0., 0., f2 - 1., 1.;
    m *= 1. / f2;
  }

  void invertedF2Derivative(Eigen::Matrix4d &m, const double f2) {
    m << 0., 0., 0., 0.,
         1., -1., 0., 0.,
         0., 0., 0., 0.,
         0., 0., 1., -1.;
    m *= 1. / (f2 * f2);
  }

  void invertedP1(Eigen::Matrix4d &m, const double p1) {
    m << p1 - 1., 0., p1, 0.,
         0., p1 - 1., 0., p1,
         p1, 0., p1 - 1., 0.,
         0., p1, 0., p1 - 1.;
    m *= 1. / (2. * p1 - 1.);
  }

  void invertedP1Derivative(Eigen::Matrix4d &m, const double p1) {
    m << 1., 0., -1., 0.,
         0., 1., 0., -1.,
         -1., 0., 1., 0.,
         0., -1., 0., 1.;
    m *= 1. / (2. * p1 - 1.) / (2. * p1 - 1.);
  }

  void invertedP2(Eigen::Matrix4d &m, const double p2) {
    m << p2 - 1., p2, 0., 0.,
         p2, p2 - 1., 0., 0.,
         0., 0., p2 - 1., p2,
         0., 0., p2, p2 - 1.;
    m *= 1. / (2. * p2 - 1.);
  }

  void invertedP2Derivative(Eigen::Matrix4d &m, const double p2) {
    m << 1., -1., 0., 0.,
         -1., 1., 0., 0.,
         0., 0., 1., -1.,
         0., 0., -1., 1.;
    m *= 1. / (2. * p2 - 1.) / (2. * p2 - 1.);
  }

  Eigen::Vector4d correction(const Eigen::Vector4d &y, const double f1, const double f2, const double p1, const double p2) {
    Eigen::Matrix4d F1;
    invertedF1(F1, f1);
    Eigen::Matrix4d F2;
    invertedF2(F2, f2);
    Eigen::Matrix4d P1;
    invertedP1(P1, p1);
    Eigen::Matrix4d P2;
    invertedP2(P2, p2);
    const auto inverted = P2 * P1 * F2 * F1;
    return static_cast<Eigen::Vector4d>(inverted * y);
  }

  Eigen::Vector4d error(const Eigen::Vector4d &y, const Eigen::Vector4d &e, const double f1, const double f1e, const double f2, const double f2e, const double p1, const double p1e, const double p2, const double p2e) {
    Eigen::Matrix4d F1;
    invertedF1(F1, f1);
    Eigen::Matrix4d dF1;
    invertedF1Derivative(dF1, f1);
    dF1 *= f1e;
    Eigen::Matrix4d F2;
    invertedF2(F2, f2);
    Eigen::Matrix4d dF2;
    invertedF2Derivative(dF2, f2);
    dF2 *= f2e;
    Eigen::Matrix4d P1;
    invertedP1(P1, p1);
    Eigen::Matrix4d dP1;
    invertedP1Derivative(dP1, p1);
    dP1 *= p1e;
    Eigen::Matrix4d P2;
    invertedP2(P2, p2);
    Eigen::Matrix4d dP2;
    invertedP2Derivative(dP2, p2);
    dP2 *= p2e;
    const auto p2Error = (dP2 * P1 * F2 * F1 * y).array();
    const auto p1Error = (P2 * dP1 * F2 * F1 * y).array();
    const auto f2Error = (P2 * P1 * dF2 * F1 * y).array();
    const auto f1Error = (P2 * P1 * F2 * dF1 * y).array();
    const auto inverted = (P2 * P1 * F2 * F1).array();
    const auto yError = ((inverted * inverted).matrix() * (e.array() * e.array()).matrix()).array();
    return (p2Error * p2Error + p1Error * p1Error + f2Error * f2Error + f1Error * f1Error + yError).sqrt().matrix();
  }
};


#endif /* MANTID_ALGORITHMS_POLARIZATIONEFFICIENCYCORTEST_H_ */
