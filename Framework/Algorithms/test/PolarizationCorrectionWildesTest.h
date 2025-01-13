// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/PolarizationCorrectionWildes.h"
#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TextAxis.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"

#include <Eigen/Dense>
#include <algorithm>
#include <functional>

using Mantid::Algorithms::PolarizationCorrectionWildes;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Algorithms;
using namespace Mantid::Algorithms::PolarizationCorrectionsHelpers;

// namespace
class PolarizationCorrectionWildesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolarizationCorrectionWildesTest *createSuite() { return new PolarizationCorrectionWildesTest(); }
  static void destroySuite(PolarizationCorrectionWildesTest *suite) { delete suite; }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_Init() {
    PolarizationCorrectionWildes alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_IdealCaseFullCorrections() { runIdealCaseFullCorrections("00,01,10,11", {"++", "+-", "-+", "--"}); }

  void test_IdealCaseFullCorrectionsReorderedInputs() {
    runIdealCaseFullCorrections("11,00,10,01", {"++", "+-", "-+", "--"});
  }

  void test_IdealCaseFullCorrectionsReorderedOutputs() {
    runIdealCaseFullCorrections("00,11,10,01", {"--", "++", "-+", "+-"}, true);
  }

  void test_IdealCaseThreeInputs10Missing() { idealThreeInputsTest("10", "00,01,11", {"++", "+-", "-+", "--"}); }

  void test_IdealCaseThreeInputs10MissingReorderedInput() {
    idealThreeInputsTest("10", "01,00,11", {"++", "+-", "-+", "--"});
  }

  void test_IdealCaseThreeInputs10MissingReorderedOutput() {
    idealThreeInputsTest("10", "01,00,11", {"--", "+-", "-+", "++"}, true);
  }

  void test_IdealCaseThreeInputs01Missing() { idealThreeInputsTest("01", "00,10,11", {"++", "+-", "-+", "--"}); }

  void test_IdealCaseThreeInputs01MissingReorderedInput() {
    idealThreeInputsTest("01", "11,00,10", {"++", "+-", "-+", "--"});
  }

  void test_IdealCaseTwoInputsWithAnalyzer() {
    constexpr size_t nHist{2};
    constexpr size_t numClones{2};
    Counts counts{yVal, 4.2 * yVal, yVal};

    auto wsList = createWorkspaceList(numClones);
    auto wsNames = generateWorkspaceNames(numClones);
    setupWorkspacesForIdealCasesTwoInput(wsNames, wsList, nHist);

    auto effWS = idealEfficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, "00, 11");
    validateNumberOfEntries(outputWS, 4);

    const std::array<std::string, 4> POL_DIRS{{"++", "+-", "-+", "--"}};
    for (size_t i = 0; i != 4; ++i) {
      const auto &dir = POL_DIRS[i];
      const std::string wsName = m_outputWSName + std::string("_") + dir;
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(wsName));
      TS_ASSERT(ws)
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t j = 0; j != nHist; ++j) {
        const auto &xs = ws->x(j);
        const auto &ys = ws->y(j);
        const auto &es = ws->e(j);
        TS_ASSERT_EQUALS(ys.size(), nBins)
        for (size_t k = 0; k != nBins; ++k) {
          const double y = counts[k];
          const double expected = [y, &dir]() {
            if (dir == "++") {
              return y;
            } else if (dir == "--") {
              return 2. * y;
            } else {
              return 0.;
            }
          }();
          const double expectedError = [y, &dir]() {
            if (dir == "++") {
              return std::sqrt(y);
            } else if (dir == "--") {
              return 2. * std::sqrt(y);
            } else {
              return 0.;
            }
          }();
          TS_ASSERT_EQUALS(xs[k], edges[k])
          TS_ASSERT_EQUALS(ys[k], expected)
          TS_ASSERT_EQUALS(es[k], expectedError)
        }
      }
    }
  }

  void test_IdealCaseTwoInputsNoAnalyzer() {
    constexpr size_t nHist{2};
    constexpr size_t numClones{2};
    // confirm the counts in this method if this is correct
    Counts counts{yVal, 4.2 * yVal, yVal};

    auto wsList = createWorkspaceList(numClones);
    auto wsNames = generateWorkspaceNames(numClones);
    setupWorkspacesForIdealCasesTwoInput(wsNames, wsList, nHist);

    auto effWS = idealEfficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, "0, 1");

    compareCorrectionResults(
        outputWS, {"_++", "_--"}, nHist, nBins, edges, counts,
        [](size_t wsIndex, double c) { return c * static_cast<double>(wsIndex + 1); },
        [](size_t wsIndex, double c) { return std::sqrt(c) * static_cast<double>(wsIndex + 1); });
  }

  void test_IdealCaseDirectBeamCorrections() {
    constexpr size_t nHist{2};
    Counts counts{yVal, 4.2 * yVal, yVal};

    auto const ws00 = createWorkspace();
    const std::string wsName{"ws00"};
    addWorkspaceToService(wsName, ws00);

    auto effWS = idealEfficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsName, effWS, "0");

    validateNumberOfEntries(outputWS, 1);
    MatrixWorkspace_sptr ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(m_outputWSName + std::string("_++")));
    TS_ASSERT(ws)
    TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
    for (size_t i = 0; i != nHist; ++i) {
      const auto &xs = ws->x(i);
      const auto &ys = ws->y(i);
      const auto &es = ws->e(i);
      TS_ASSERT_EQUALS(ys.size(), nBins)
      for (size_t j = 0; j != nBins; ++j) {
        const double y = counts[j];
        TS_ASSERT_EQUALS(xs[j], edges[j])
        TS_ASSERT_EQUALS(ys[j], y)
        TS_ASSERT_EQUALS(es[j], std::sqrt(y))
      }
    }
  }

  void test_FullCorrections() {
    constexpr size_t nHist{2};
    constexpr size_t numClones{4};
    Counts counts{yVal, yVal, yVal};

    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);

    auto effWS = efficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS);

    fullFourInputsResultsCheck(outputWS, wsList[0], wsList[1], wsList[2], wsList[3], effWS, counts);
  }

  void test_ThreeInputsWithMissing01FlipperConfiguration() { threeInputsTest("01"); }

  void test_ThreeInputsWithMissing10FlipperConfiguration() { threeInputsTest("10"); }

  void test_TwoInputsWithAnalyzer() {
    constexpr size_t nHist{2};
    constexpr size_t numClones{2};
    Counts counts{yVal, yVal, yVal};

    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);
    MatrixWorkspace_sptr ws01 = nullptr;
    MatrixWorkspace_sptr ws10 = nullptr;

    auto effWS = efficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, "00, 11");

    validateNumberOfEntries(outputWS, 4);
    solveMissingIntensities(wsList[0], ws01, ws10, wsList[1], effWS);
    const double F1 = effWS->y(0).front();
    const double F1e = effWS->e(0).front();
    const double F2 = effWS->y(1).front();
    const double F2e = effWS->e(1).front();
    const double P1 = effWS->y(2).front();
    const double P1e = effWS->e(2).front();
    const double P2 = effWS->y(3).front();
    const double P2e = effWS->e(3).front();
    const Eigen::Vector4d y{wsList[0]->y(0).front(), ws01->y(0).front(), ws10->y(0).front(), wsList[1]->y(0).front()};
    const auto expected = correction(y, F1, F2, P1, P2);
    const Eigen::Vector4d e{wsList[0]->e(0).front(), ws01->e(0).front(), ws10->e(0).front(), wsList[1]->e(0).front()};
    const auto expectedError = error(y, e, F1, F1e, F2, F2e, P1, P1e, P2, P2e);
    // This test constructs the expected missing I01 and I10 intensities
    // slightly different from what the algorithm does: I10 is solved
    // first and then I01 is solved using all I00, I10 and I11. This
    // results in slightly larger errors estimates for I01 and thus for
    // the final corrected expected intensities.
    std::vector<double> errorTolerances = {1e-5, 1e-2, 1e-2, 1e-5};
    compareCorrectionResults(
        outputWS, {"_++", "_+-", "_-+", "_--"}, nHist, nBins, edges, counts,
        [&expected](size_t i, double) { return expected[i]; },
        [&expectedError](size_t i, double) { return expectedError[i]; }, std::move(errorTolerances));
  }

  void test_TwoInputsWithoutAnalyzer() {
    constexpr size_t nHist{2};
    constexpr size_t numClones{2};
    Counts counts{yVal, yVal, yVal};

    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);

    auto effWS = efficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, "0, 1");

    validateNumberOfEntries(outputWS, 2);
    const double F1 = effWS->y(0).front();
    const double F1e = effWS->e(0).front();
    const double P1 = effWS->y(2).front();
    const double P1e = effWS->e(2).front();
    const Eigen::Vector2d y{wsList[0]->y(0).front(), wsList[1]->y(0).front()};
    const auto expected = correctionWithoutAnalyzer(y, F1, P1);
    const Eigen::Vector2d e{wsList[0]->e(0).front(), wsList[1]->e(0).front()};
    const auto expectedError = errorWithoutAnalyzer(y, e, F1, F1e, P1, P1e);
    compareCorrectionResults(
        outputWS, {"_++", "_--"}, nHist, nBins, edges, counts, [&expected](size_t i, double) { return expected[i]; },
        [&expectedError](size_t i, double) { return expectedError[i]; });
  }

  void test_directBeamOnlyInput() {
    constexpr size_t nHist{2};
    Counts counts{yVal, yVal, yVal};

    auto const ws00 = createWorkspace(nHist, counts);
    const std::string wsName{"ws00"};
    addWorkspaceToService(wsName, ws00);

    auto effWS = efficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsName, effWS, "0");
    validateNumberOfEntries(outputWS, 1);

    const auto P1 = effWS->y(2).front();
    const auto P1e = effWS->e(2).front();
    const auto P2 = effWS->y(3).front();
    const auto P2e = effWS->e(3).front();
    const double y{ws00->y(0).front()};
    const auto inverted = 1. / (1. - P2 - P1 + 2. * P1 * P2);
    const auto expected = inverted * y;
    const double e{ws00->e(0).front()};
    const auto errorP1 = P1e * y * (2. * P1 - 1.) * inverted * inverted;
    const auto errorP2 = P2e * y * (2. * P2 - 1.) * inverted * inverted;
    const auto errorY = e * e * inverted * inverted;
    const auto expectedError = std::sqrt(errorP1 * errorP1 + errorP2 * errorP2 + errorY);
    compareCorrectionResults(
        outputWS, {"_++"}, nHist, nBins, edges, counts, [&expected](size_t, double) { return expected; },
        [&expectedError](size_t, double) { return expectedError; });
  }

  void test_FailureWhenEfficiencyHistogramIsMissing() {
    const std::string wsName{"ws00"};
    auto [ws00, effWS] = setupFailureWorkspaceAndEfficiencies();

    // Rename F1 to something else
    auto axis = std::make_unique<TextAxis>(4);
    axis->setLabel(0, "__wrong_histogram_label");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    effWS->replaceAxis(1, std::move(axis));

    runCorrectionWildes(wsName, effWS, "0", "", false);
  }

  void test_FailureWhenEfficiencyXDataMismatchess() {
    auto [ws00, effWS] = setupFailureWorkspaceAndEfficiencies();

    auto &xs = effWS->mutableX(0);
    xs[xs.size() / 2] *= 1.01;

    runCorrectionWildes("ws00", effWS, "0", "", false);
  }

  void test_FailureWhenNumberOfHistogramsInInputWorkspacesMismatch() {
    constexpr size_t nHist{2};
    Counts counts{0., 0., 0.};

    auto const ws00 = createWorkspace(nHist, counts);
    MatrixWorkspace_sptr ws01 = ws00->clone();
    MatrixWorkspace_sptr ws10 = create<Workspace2D>(nHist + 1, Histogram(edges, counts));
    MatrixWorkspace_sptr ws11 = ws00->clone();
    const std::vector<std::string> wsNames{{"ws00", "ws01", "ws10", "ws11"}};
    std::vector<MatrixWorkspace_sptr> wsList = {ws00, ws01, ws10, ws11};
    addWorkspacesToService(wsNames, wsList);

    auto effWS = idealEfficiencies(edges);
    runCorrectionWildes(wsNames, effWS, "", "", false);
  }

  void test_FailureWhenAnInputWorkspaceIsMissing() {
    Counts counts{0., 0., 0.};
    constexpr size_t numClones{3};

    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    addWorkspacesToService(wsNames, wsList);

    PolarizationCorrectionWildes alg;
    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS(alg.setPropertyValue("InputWorkspaces", "ws00, ws01, ws10, ws11"), const std::invalid_argument &)
  }

  void test_IdealCrossPolarizationCaseFullCorrections() {
    auto crossPolarizationEffWS = idealEfficiencies(edges, false);
    // Cross polarized ideal efficiencies should give us the same ouput as for ideal efficiencies but in the reverse
    // order
    const std::array<std::string, 4> POL_DIRS{{"--", "-+", "+-", "++"}};
    idealCaseFullCorrectionsTest(edges, crossPolarizationEffWS, POL_DIRS);
  }

  void test_SpinStateOrderInOutputWorkspaceGroup() {
    Counts counts{2.3, 9.6, 2.3};
    constexpr size_t numClones{4};

    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    addWorkspacesToService(wsNames, wsList);

    auto effWS = idealEfficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS);

    validateNumberOfEntries(outputWS, 4);
    const std::array<std::string, 4> OUTPUT_ORDER{{"++", "+-", "-+", "--"}};
    for (size_t i = 0; i < 4; ++i) {
      auto ws = outputWS->getItem(i);
      TS_ASSERT(ws)
      const std::string expectedName = m_outputWSName + std::string("_") + OUTPUT_ORDER[i];
      TS_ASSERT_EQUALS(ws->getName(), expectedName)
    }
  }

  void test_SpinStateAddedToSampleLogWhenRequested() {
    constexpr size_t numClones{4};
    Counts counts{yVal, yVal, yVal};

    const std::vector<std::string> expectedLogValues = {SpinStatesORSO::PP, SpinStatesORSO::PM, SpinStatesORSO::MP,
                                                        SpinStatesORSO::MM};

    prepareWorkspacesAndRunCorrectionWithSampleState(counts, edges, numClones, true, expectedLogValues);
  }

  void test_SpinStateAddedToSampleLogWhenRequestedNoAnalyser() {
    constexpr size_t numClones{2};
    Counts counts{yVal, yVal, yVal};

    const std::vector<std::string> expectedLogValues = {SpinStatesORSO::PO, SpinStatesORSO::MO};

    prepareWorkspacesAndRunCorrectionWithSampleState(counts, edges, numClones, true, expectedLogValues, "0, 1");
  }

  void test_SpinStateNotAddedToSampleLogByDefault() {
    constexpr size_t numClones{4};
    Counts counts{yVal, yVal, yVal};

    prepareWorkspacesAndRunCorrectionWithSampleState(counts, edges, numClones, false, {});
  }

private:
  const std::string m_outputWSName{"output"};
  const double yVal{2.3};
  const BinEdges edges{0.3, 0.6, 0.9, 1.2};
  static constexpr size_t nBins{3};

  std::unique_ptr<PolarizationCorrectionWildes> createWildesAlg(const std::vector<std::string> &inputWorkspaces,
                                                                Mantid::API::MatrixWorkspace_sptr effWs,
                                                                const std::string &flippers = "",
                                                                const std::string &spinStates = "") {
    auto alg = std::make_unique<PolarizationCorrectionWildes>();
    alg->setChild(true);
    alg->setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg->initialize())
    TS_ASSERT(alg->isInitialized())

    if (inputWorkspaces.size() == 1) {
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspaces", inputWorkspaces[0]))
    } else {
      TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspaces", inputWorkspaces))
    }

    TS_ASSERT_THROWS_NOTHING(alg->setPropertyValue("OutputWorkspace", m_outputWSName))

    if (!flippers.empty()) {
      alg->setProperty("Flippers", flippers);
    }

    if (!spinStates.empty()) {
      alg->setProperty("SpinStates", spinStates);
    }

    TS_ASSERT_THROWS_NOTHING(alg->setProperty("Efficiencies", effWs))

    return alg;
  }

  Mantid::API::WorkspaceGroup_sptr runAlg(std::unique_ptr<PolarizationCorrectionWildes> alg,
                                          const bool expectedToWork = true) {
    if (expectedToWork) {
      TS_ASSERT_THROWS_NOTHING(alg->execute())
      TS_ASSERT(alg->isExecuted())
    } else {
      TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &)
      TS_ASSERT(!alg->isExecuted())
      return nullptr;
    }

    Mantid::API::WorkspaceGroup_sptr outputWS = alg->getProperty("OutputWorkspace");
    TS_ASSERT(outputWS)
    return outputWS;
  }

  Mantid::API::WorkspaceGroup_sptr runCorrectionWildes(const std::string &inputWorkspace,
                                                       Mantid::API::MatrixWorkspace_sptr effWs,
                                                       const std::string &flippers = "",
                                                       const std::string &spinStates = "",
                                                       const bool expectedToWork = true) {
    return runCorrectionWildes(std::vector<std::string>{inputWorkspace}, effWs, flippers, spinStates, expectedToWork);
  }

  Mantid::API::WorkspaceGroup_sptr runCorrectionWildes(const std::vector<std::string> &inputWorkspaces,
                                                       Mantid::API::MatrixWorkspace_sptr effWs,
                                                       const std::string &flippers = "",
                                                       const std::string &spinStates = "",
                                                       const bool expectedToWork = true) {
    auto alg = createWildesAlg(inputWorkspaces, effWs, flippers, spinStates);
    return runAlg(std::move(alg), expectedToWork);
  }

  void compareCorrectionResults(const WorkspaceGroup_sptr wsGrp, const std::vector<std::string> &pols,
                                const size_t nHist, const size_t nBins, const BinEdges &edges, const Counts &counts,
                                const std::function<double(size_t, double)> &expectedY,
                                const std::function<double(size_t, double)> &expectedError,
                                std::vector<double> errorTolerances = std::vector<double>{1e-12}) {
    bool checkErrorBound = errorTolerances.size() > 1;
    if (wsGrp->size() > 1) {
      errorTolerances.resize(wsGrp->size(), errorTolerances[0]);
    }

    for (size_t wsIndex = 0; wsIndex < wsGrp->size(); ++wsIndex) {
      MatrixWorkspace_sptr ws =
          std::dynamic_pointer_cast<MatrixWorkspace>(wsGrp->getItem(m_outputWSName + pols[wsIndex]));
      TS_ASSERT(ws)
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t histIndex = 0; histIndex < nHist; ++histIndex) {
        const auto &x = ws->x(histIndex);
        const auto &y = ws->y(histIndex);
        const auto &e = ws->e(histIndex);
        TS_ASSERT_EQUALS(y.size(), nBins)
        for (size_t binIndex = 0; binIndex < nBins; ++binIndex) {
          const double c = counts[binIndex];
          TS_ASSERT_EQUALS(x[binIndex], edges[binIndex])
          TS_ASSERT_DELTA(y[binIndex], expectedY(wsIndex, c), 1e-12)
          TS_ASSERT_DELTA(e[binIndex], expectedError(wsIndex, c), errorTolerances[wsIndex])
          if (checkErrorBound) {
            TS_ASSERT_LESS_THAN(e[binIndex], expectedError(wsIndex, c))
          }
        }
      }
    }
  }

  std::vector<double> convertEigenToVector(const Eigen::Vector2d &v) { return {v[0], v[1]}; }

  std::vector<double> convertEigenToVector(const Eigen::Vector4d &v) { return {v[0], v[1], v[2], v[3]}; }

  Mantid::API::MatrixWorkspace_sptr efficiencies(const Mantid::HistogramData::BinEdges &edges) {
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
    auto axis = std::make_unique<TextAxis>(4);
    axis->setLabel(0, "F1");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    ws->replaceAxis(1, std::move(axis));
    return ws;
  }

  Mantid::API::MatrixWorkspace_sptr idealEfficiencies(const Mantid::HistogramData::BinEdges &edges,
                                                      const bool isNormalPolarization = true) {
    const auto nBins = edges.size() - 1;
    constexpr size_t nHist{4};
    Counts counts(nBins, 0.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(nHist, Histogram(edges, counts));
    ws->mutableY(0) = 1.;
    ws->mutableY(1) = 1.;
    if (isNormalPolarization) {
      // Normal polarization should have 1 for all efficiency factors
      // Cross polarization should have 0 for the polarizer/normaliser (i.e. P1/P2) efficiency factors
      ws->mutableY(2) = 1.;
      ws->mutableY(3) = 1.;
    }
    auto axis = std::make_unique<TextAxis>(4);
    axis->setLabel(0, "F1");
    axis->setLabel(1, "F2");
    axis->setLabel(2, "P1");
    axis->setLabel(3, "P2");
    ws->replaceAxis(1, std::move(axis));
    return ws;
  }

  void idealCaseFullCorrectionsTest(const Mantid::HistogramData::BinEdges &edges,
                                    const Mantid::API::MatrixWorkspace_sptr &effWS,
                                    const std::array<std::string, 4> &outputSpinStates,
                                    const std::string &flipperConfig = "00,01,10,11",
                                    const std::string &spinStates = "") {
    constexpr size_t nHist{2};
    Counts counts{yVal, 4.2 * yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = ws00->clone();
    MatrixWorkspace_sptr ws10 = ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();

    std::vector<std::string> wsNames{"ws00", "ws01", "ws10", "ws11"};
    const std::vector<MatrixWorkspace_sptr> wsList{ws00, ws01, ws10, ws11};
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);

    // Re-order the input workspace names to match the input flipper configuration
    const auto &flipperConfigVec = splitSpinStateString(flipperConfig);
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "00").value()] = ws00->getName();
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "01").value()] = ws01->getName();
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "10").value()] = ws10->getName();
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "11").value()] = ws11->getName();

    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, flipperConfig, spinStates);
    auto pols = std::vector<std::string>(4);
    std::transform(outputSpinStates.cbegin(), outputSpinStates.cend(), pols.begin(),
                   [](const std::string &s) { return "_" + s; });
    if (!spinStates.empty()) {
      compareCorrectionResults(
          outputWS, pols, nHist, nBins, edges, counts,
          [&](size_t wsIndex, double c) { return c * static_cast<double>(getPolIndex(pols[wsIndex]) + 1); },
          [&](size_t wsIndex, double c) { return std::sqrt(c) * static_cast<double>(getPolIndex(pols[wsIndex]) + 1); });
    } else {
      compareCorrectionResults(
          outputWS, pols, nHist, nBins, edges, counts,
          [](size_t wsIndex, double c) { return c * static_cast<double>(wsIndex + 1); },
          [](size_t wsIndex, double c) { return std::sqrt(c) * static_cast<double>(wsIndex + 1); });
    }
  }

  void idealThreeInputsTest(const std::string &missingFlipperConf, const std::string &flipperConfig,
                            const std::vector<std::string> &outputWsOrder, const bool useSpinStates = false) {
    constexpr size_t nHist{2};
    Counts counts{yVal, 4.2 * yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr wsXX = ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();

    const std::string presentFlipperConf = missingFlipperConf == "01" ? "10" : "01";
    std::vector<std::string> wsNames{"ws00", "wsXX", "ws11"};
    const std::vector<MatrixWorkspace_sptr> wsList{ws00, wsXX, ws11};
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);

    // Re-order the input workspace names to match the input flipper configuration
    const auto &flipperConfigVec = splitSpinStateString(flipperConfig);
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "00").value()] = ws00->getName();
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, presentFlipperConf).value()] = wsXX->getName();
    wsNames[indexOfWorkspaceForSpinState(flipperConfigVec, "11").value()] = ws11->getName();

    std::string spinStates = "";
    if (useSpinStates) {
      spinStates = outputWsOrder[0] + "," + outputWsOrder[1] + "," + outputWsOrder[2] + "," + outputWsOrder[3];
    }

    auto effWS = idealEfficiencies(edges);
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, flipperConfig, spinStates);

    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4)
    for (size_t i = 0; i != 4; ++i) {
      const auto &dir = outputWsOrder[i];
      const std::string wsName = m_outputWSName + std::string("_") + dir;
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(wsName));
      TS_ASSERT(ws)
      TS_ASSERT_EQUALS(ws->getNumberHistograms(), nHist)
      for (size_t j = 0; j != nHist; ++j) {
        const auto &xs = ws->x(j);
        const auto &ys = ws->y(j);
        const auto &es = ws->e(j);
        TS_ASSERT_EQUALS(ys.size(), nBins)
        for (size_t k = 0; k != nBins; ++k) {
          const double y = counts[k];
          const double expected = [y, &dir]() {
            if (dir == "++") {
              return y;
            } else if (dir == "--") {
              return 3. * y;
            } else {
              return 2. * y;
            }
          }();
          const double expectedError = [y, &dir, &missingFlipperConf]() {
            if (dir == "++") {
              return std::sqrt(y);
            } else if (dir == "--") {
              return 3. * std::sqrt(y);
            } else {
              std::string conf =
                  std::string(dir.front() == '+' ? "0" : "1") + std::string(dir.back() == '+' ? "0" : "1");
              if (conf != missingFlipperConf) {
                return 2. * std::sqrt(y);
              } else {
                return 0.;
              }
            }
          }();
          TS_ASSERT_EQUALS(xs[k], edges[k])
          TS_ASSERT_EQUALS(ys[k], expected)
          TS_ASSERT_EQUALS(es[k], expectedError)
        }
      }
    }
  }

  void threeInputsTest(const std::string &missingFlipperConf) {
    constexpr size_t nHist{2};
    Counts counts{yVal, yVal, yVal};
    MatrixWorkspace_sptr ws00 = create<Workspace2D>(nHist, Histogram(edges, counts));
    MatrixWorkspace_sptr ws01 = missingFlipperConf == "01" ? nullptr : ws00->clone();
    MatrixWorkspace_sptr ws10 = missingFlipperConf == "10" ? nullptr : ws00->clone();
    MatrixWorkspace_sptr ws11 = ws00->clone();
    const std::vector<std::string> wsNames{{"ws00", "wsXX", "ws11"}};
    const std::array<MatrixWorkspace_sptr, 3> wsList{{ws00, ws01 != nullptr ? ws01 : ws10, ws11}};
    for (size_t i = 0; i != 3; ++i) {
      for (size_t j = 0; j != nHist; ++j) {
        wsList[i]->mutableY(j) *= static_cast<double>(i + 1);
        wsList[i]->mutableE(j) *= static_cast<double>(i + 1);
      }
      AnalysisDataService::Instance().addOrReplace(wsNames[i], wsList[i]);
    }
    auto effWS = efficiencies(edges);
    const std::string presentFlipperConf = missingFlipperConf == "01" ? "10" : "01";
    const std::string flipperConf = "00, " + presentFlipperConf + ", 11";
    WorkspaceGroup_sptr outputWS = runCorrectionWildes(wsNames, effWS, flipperConf);
    TS_ASSERT_EQUALS(outputWS->getNumberOfEntries(), 4)
    solveMissingIntensity(ws00, ws01, ws10, ws11, effWS);
    fullFourInputsResultsCheck(outputWS, ws00, ws01, ws10, ws11, effWS, counts);
  }

  void fullFourInputsResultsCheck(Mantid::API::WorkspaceGroup_sptr &outputWS, Mantid::API::MatrixWorkspace_sptr &ws00,
                                  Mantid::API::MatrixWorkspace_sptr &ws01, Mantid::API::MatrixWorkspace_sptr &ws10,
                                  Mantid::API::MatrixWorkspace_sptr &ws11, Mantid::API::MatrixWorkspace_sptr &effWS,
                                  const Counts &counts) {
    const auto nHist = ws00->getNumberHistograms();
    const auto nBins = ws00->y(0).size();
    const auto edges = ws00->binEdges(0);
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
    compareCorrectionResults(
        outputWS, std::vector<std::string>{"_++", "_+-", "_-+", "_--"}, nHist, nBins, edges, counts,
        [&expected](size_t i, double) { return expected[i]; },
        [&expectedError](size_t i, double) { return expectedError[i]; });
  }

  Eigen::Matrix4d invertedF1(const double f1) {
    Eigen::Matrix4d m;
    m << f1, 0., 0., 0., 0., f1, 0., 0., f1 - 1., 0., 1., 0., 0., f1 - 1., 0., 1.;
    m *= 1. / f1;
    return m;
  }

  Eigen::Matrix4d invertedF1Derivative(const double f1) {
    Eigen::Matrix4d m;
    m << 0., 0., 0., 0., 0., 0., 0., 0., 1., 0., -1., 0., 0., 1., 0., -1.;
    m *= 1. / (f1 * f1);
    return m;
  }

  Eigen::Matrix4d invertedF2(const double f2) {
    Eigen::Matrix4d m;
    m << f2, 0., 0., 0., f2 - 1., 1., 0., 0., 0., 0., f2, 0., 0., 0., f2 - 1., 1.;
    m *= 1. / f2;
    return m;
  }

  Eigen::Matrix4d invertedF2Derivative(const double f2) {
    Eigen::Matrix4d m;
    m << 0., 0., 0., 0., 1., -1., 0., 0., 0., 0., 0., 0., 0., 0., 1., -1.;
    m *= 1. / (f2 * f2);
    return m;
  }

  Eigen::Matrix4d invertedP1(const double p1) {
    Eigen::Matrix4d m;
    m << p1, 0., p1 - 1., 0., 0., p1, 0., p1 - 1., p1 - 1., 0., p1, 0., 0., p1 - 1., 0., p1;
    m *= 1. / (2. * p1 - 1.);
    return m;
  }

  Eigen::Matrix4d invertedP1Derivative(const double p1) {
    Eigen::Matrix4d m;
    m << 1., 0., -1., 0., 0., 1., 0., -1., -1., 0., 1., 0., 0., -1., 0., 1.;
    m *= 1. / (2. * p1 - 1.) / (2. * p1 - 1.);
    return m;
  }

  Eigen::Matrix4d invertedP2(const double p2) {
    Eigen::Matrix4d m;
    m << p2, p2 - 1., 0., 0., p2 - 1., p2, 0., 0., 0., 0., p2, p2 - 1., 0., 0., p2 - 1., p2;
    m *= 1. / (2. * p2 - 1.);
    return m;
  }

  Eigen::Matrix4d invertedP2Derivative(const double p2) {
    Eigen::Matrix4d m;
    m << 1., -1., 0., 0., -1., 1., 0., 0., 0., 0., 1., -1., 0., 0., -1., 1.;
    m *= 1. / (2. * p2 - 1.) / (2. * p2 - 1.);
    return m;
  }

  std::vector<double> correction(const Eigen::Vector4d &y, const double f1, const double f2, const double p1,
                                 const double p2) {
    const Eigen::Matrix4d F1 = invertedF1(f1);
    const Eigen::Matrix4d F2 = invertedF2(f2);
    const Eigen::Matrix4d P1 = invertedP1(p1);
    const Eigen::Matrix4d P2 = invertedP2(p2);
    const Eigen::Matrix4d inverted = (P2 * P1 * F2 * F1).matrix();
    const Eigen::Vector4d c = (inverted * y).matrix();
    return convertEigenToVector(c);
  }

  std::vector<double> error(const Eigen::Vector4d &y, const Eigen::Vector4d &e, const double f1, const double f1e,
                            const double f2, const double f2e, const double p1, const double p1e, const double p2,
                            const double p2e) {
    const Eigen::Matrix4d F1 = invertedF1(f1);
    const Eigen::Matrix4d dF1 = f1e * invertedF1Derivative(f1);
    const Eigen::Matrix4d F2 = invertedF2(f2);
    const Eigen::Matrix4d dF2 = f2e * invertedF2Derivative(f2);
    const Eigen::Matrix4d P1 = invertedP1(p1);
    const Eigen::Matrix4d dP1 = p1e * invertedP1Derivative(p1);
    const Eigen::Matrix4d P2 = invertedP2(p2);
    const Eigen::Matrix4d dP2 = p2e * invertedP2Derivative(p2);
    const auto p2Error = (dP2 * P1 * F2 * F1 * y).array();
    const auto p1Error = (P2 * dP1 * F2 * F1 * y).array();
    const auto f2Error = (P2 * P1 * dF2 * F1 * y).array();
    const auto f1Error = (P2 * P1 * F2 * dF1 * y).array();
    const auto inverted = (P2 * P1 * F2 * F1).array();
    const auto yError = ((inverted * inverted).matrix() * (e.array() * e.array()).matrix()).array();
    const Eigen::Vector4d err =
        (p2Error * p2Error + p1Error * p1Error + f2Error * f2Error + f1Error * f1Error + yError).sqrt().matrix();
    return convertEigenToVector(err);
  }

  std::vector<double> correctionWithoutAnalyzer(const Eigen::Vector2d &y, const double f1, const double p1) {
    Eigen::Matrix2d F1;
    F1 << f1, 0., f1 - 1., 1.;
    F1 *= 1. / f1;
    Eigen::Matrix2d P1;
    P1 << p1, p1 - 1., p1 - 1., p1;
    P1 *= 1. / (2. * p1 - 1.);
    const Eigen::Vector2d c = (P1 * F1).matrix() * y;
    return convertEigenToVector(c);
  }

  std::vector<double> errorWithoutAnalyzer(const Eigen::Vector2d &y, const Eigen::Vector2d &e, const double f1,
                                           const double f1e, const double p1, const double p1e) {
    Eigen::Matrix2d F1;
    F1 << f1, 0, f1 - 1., 1.;
    F1 *= 1. / f1;
    Eigen::Matrix2d dF1;
    dF1 << 0., 0., 1., -1.;
    dF1 *= f1e / (f1 * f1);
    Eigen::Matrix2d P1;
    P1 << p1, p1 - 1., p1 - 1., p1;
    P1 *= 1. / (2. * p1 - 1.);
    Eigen::Matrix2d dP1;
    dP1 << 1., -1., -1., 1.;
    dP1 *= p1e / ((2. * p1 - 1.) * (2. * p1 - 1.));
    const auto p1Error = (dP1 * F1 * y).array();
    const auto f1Error = (P1 * dF1 * y).array();
    const auto inverted = (P1 * F1).array();
    const auto yError = ((inverted * inverted).matrix() * (e.array() * e.array()).matrix()).array();
    const Eigen::Vector2d err = (p1Error * p1Error + f1Error * f1Error + yError).sqrt().matrix();
    return convertEigenToVector(err);
  }

  void solveMissingIntensity(const Mantid::API::MatrixWorkspace_sptr &ppWS, Mantid::API::MatrixWorkspace_sptr &pmWS,
                             Mantid::API::MatrixWorkspace_sptr &mpWS, const Mantid::API::MatrixWorkspace_sptr &mmWS,
                             const Mantid::API::MatrixWorkspace_sptr &effWS) {
    const auto &F1 = effWS->y(0);
    const auto &F2 = effWS->y(1);
    const auto &P1 = effWS->y(2);
    const auto &P2 = effWS->y(3);
    if (!pmWS) {
      pmWS = mpWS->clone();
      for (size_t wsIndex = 0; wsIndex != pmWS->getNumberHistograms(); ++wsIndex) {
        const auto &ppY = ppWS->y(wsIndex);
        auto &pmY = pmWS->mutableY(wsIndex);
        auto &pmE = pmWS->mutableE(wsIndex);
        const auto &mpY = mpWS->y(wsIndex);
        const auto &mmY = mmWS->y(wsIndex);
        for (size_t binIndex = 0; binIndex != mpY.size(); ++binIndex) {
          pmY[binIndex] = -(2 * ppY[binIndex] * F2[binIndex] * P2[binIndex] - P2[binIndex] * mmY[binIndex] -
                            2 * mpY[binIndex] * F2[binIndex] * P2[binIndex] + mpY[binIndex] * P2[binIndex] -
                            ppY[binIndex] * P2[binIndex] + P1[binIndex] * mmY[binIndex] -
                            2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] + ppY[binIndex] * P1[binIndex] -
                            P1[binIndex] * mpY[binIndex] + ppY[binIndex] * F1[binIndex] + mpY[binIndex] * F2[binIndex] -
                            ppY[binIndex] * F2[binIndex]) /
                          (P2[binIndex] - P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]);
          // Error propagation is not implemented in the algorithm.
          pmE[binIndex] = 0.;
        }
      }
    } else {
      mpWS = pmWS->clone();
      for (size_t wsIndex = 0; wsIndex != mpWS->getNumberHistograms(); ++wsIndex) {
        const auto &ppY = ppWS->y(wsIndex);
        const auto &pmY = pmWS->y(wsIndex);
        auto &mpY = mpWS->mutableY(wsIndex);
        auto &mpE = mpWS->mutableE(wsIndex);
        const auto &mmY = mmWS->y(wsIndex);
        for (size_t binIndex = 0; binIndex != mpY.size(); ++binIndex) {
          mpY[binIndex] =
              (-ppY[binIndex] * P2[binIndex] + P2[binIndex] * pmY[binIndex] - P2[binIndex] * mmY[binIndex] +
               2 * ppY[binIndex] * F2[binIndex] * P2[binIndex] - pmY[binIndex] * P1[binIndex] +
               P1[binIndex] * mmY[binIndex] + ppY[binIndex] * P1[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] + 2 * pmY[binIndex] * F1[binIndex] * P1[binIndex] +
               ppY[binIndex] * F1[binIndex] - ppY[binIndex] * F2[binIndex] - pmY[binIndex] * F1[binIndex]) /
              (-P2[binIndex] + 2 * F2[binIndex] * P2[binIndex] + P1[binIndex] - F2[binIndex]);
          // Error propagation is not implemented in the algorithm.
          mpE[binIndex] = 0.;
        }
      }
    }
  }

  void solveMissingIntensities(const Mantid::API::MatrixWorkspace_sptr &ppWS, Mantid::API::MatrixWorkspace_sptr &pmWS,
                               Mantid::API::MatrixWorkspace_sptr &mpWS, const Mantid::API::MatrixWorkspace_sptr &mmWS,
                               const Mantid::API::MatrixWorkspace_sptr &effWS) {
    const auto &F1 = effWS->y(0);
    const auto &F1E = effWS->e(0);
    const auto &F2 = effWS->y(1);
    const auto &F2E = effWS->e(1);
    const auto &P1 = effWS->y(2);
    const auto &P1E = effWS->e(2);
    const auto &P2 = effWS->y(3);
    const auto &P2E = effWS->e(3);
    pmWS = ppWS->clone();
    mpWS = ppWS->clone();
    for (size_t wsIndex = 0; wsIndex != ppWS->getNumberHistograms(); ++wsIndex) {
      const auto &ppY = ppWS->y(wsIndex);
      const auto &ppE = ppWS->e(wsIndex);
      auto &pmY = pmWS->mutableY(wsIndex);
      auto &pmE = pmWS->mutableE(wsIndex);
      auto &mpY = mpWS->mutableY(wsIndex);
      auto &mpE = mpWS->mutableE(wsIndex);
      const auto &mmY = mmWS->y(wsIndex);
      const auto &mmE = mmWS->e(wsIndex);
      for (size_t binIndex = 0; binIndex != mpY.size(); ++binIndex) {
        const double P12 = P1[binIndex] * P1[binIndex];
        const double P13 = P1[binIndex] * P12;
        const double P14 = P1[binIndex] * P13;
        const double P22 = P2[binIndex] * P2[binIndex];
        const double P23 = P2[binIndex] * P22;
        const double F12 = F1[binIndex] * F1[binIndex];
        {
          mpY[binIndex] =
              -(-mmY[binIndex] * P22 * F1[binIndex] + 2 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P22 -
                2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] -
                8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
                2 * ppY[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
                8 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P2[binIndex] +
                2 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] -
                8 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] * P1[binIndex] -
                2 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
                2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
                8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
                mmY[binIndex] * P2[binIndex] * F1[binIndex] + ppY[binIndex] * F1[binIndex] * F2[binIndex] -
                ppY[binIndex] * F2[binIndex] * P12 + 4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 +
                4 * ppY[binIndex] * F12 * F2[binIndex] * P1[binIndex] -
                4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] +
                ppY[binIndex] * F2[binIndex] * P1[binIndex] - 4 * ppY[binIndex] * F12 * F2[binIndex] * P12 -
                ppY[binIndex] * F12 * F2[binIndex]) /
              (-F1[binIndex] * F2[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * P22 * F1[binIndex] * P1[binIndex] + 2 * P2[binIndex] * F1[binIndex] * P1[binIndex] +
               3 * F1[binIndex] * F2[binIndex] * P2[binIndex] - P2[binIndex] * F1[binIndex] + P22 * F1[binIndex] +
               F2[binIndex] * P12 - 2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 -
               F2[binIndex] * P1[binIndex] - 8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dI00 =
              -F2[binIndex] *
              (-2 * P2[binIndex] * F1[binIndex] + 2 * P12 * P2[binIndex] +
               8 * P2[binIndex] * F1[binIndex] * P1[binIndex] - 2 * P1[binIndex] * P2[binIndex] +
               2 * P2[binIndex] * F12 - 8 * P2[binIndex] * F12 * P1[binIndex] - 8 * P2[binIndex] * F1[binIndex] * P12 +
               8 * P2[binIndex] * F12 * P12 - 4 * F1[binIndex] * P1[binIndex] - F12 + 4 * F12 * P1[binIndex] +
               P1[binIndex] + F1[binIndex] - P12 + 4 * F1[binIndex] * P12 - 4 * F12 * P12) /
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dI11 =
              -P2[binIndex] * F1[binIndex] * (1 - 2 * P1[binIndex] - P2[binIndex] + 2 * P1[binIndex] * P2[binIndex]) /
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double divisor1 =
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dF1 =
              -F2[binIndex] *
              (-P1[binIndex] * mmY[binIndex] * P2[binIndex] + 4 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P22 -
               ppY[binIndex] * F2[binIndex] * P12 * P2[binIndex] -
               10 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 - 8 * ppY[binIndex] * F2[binIndex] * P12 * P22 +
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] -
               ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               32 * ppY[binIndex] * F12 * F2[binIndex] * P14 * P2[binIndex] +
               32 * ppY[binIndex] * F2[binIndex] * P14 * P2[binIndex] * F1[binIndex] -
               32 * ppY[binIndex] * F2[binIndex] * P14 * P22 * F1[binIndex] +
               32 * ppY[binIndex] * F12 * F2[binIndex] * P14 * P22 +
               32 * ppY[binIndex] * F12 * F2[binIndex] * P13 * P23 + 2 * ppY[binIndex] * F2[binIndex] * P14 +
               4 * ppY[binIndex] * P13 * P23 - 4 * P13 * mmY[binIndex] * P23 -
               8 * ppY[binIndex] * F2[binIndex] * P13 * P23 - 16 * ppY[binIndex] * P23 * F12 * P13 +
               8 * ppY[binIndex] * F12 * F2[binIndex] * P14 - 8 * ppY[binIndex] * F2[binIndex] * P14 * P2[binIndex] +
               8 * ppY[binIndex] * F2[binIndex] * P14 * P22 - 8 * ppY[binIndex] * F2[binIndex] * P14 * F1[binIndex] +
               10 * ppY[binIndex] * F2[binIndex] * P13 * P2[binIndex] - 4 * ppY[binIndex] * F2[binIndex] * P13 * P22 +
               16 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 -
               4 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P23 + 12 * ppY[binIndex] * F2[binIndex] * P12 * P23 +
               18 * ppY[binIndex] * P22 * F12 * P1[binIndex] - 20 * ppY[binIndex] * F12 * F2[binIndex] * P13 -
               36 * ppY[binIndex] * P22 * F12 * P12 + 24 * ppY[binIndex] * P22 * F12 * P13 -
               6 * ppY[binIndex] * P2[binIndex] * F12 * P1[binIndex] -
               5 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] + 8 * ppY[binIndex] * F12 * F2[binIndex] * P22 -
               8 * ppY[binIndex] * P2[binIndex] * F12 * P13 + 12 * ppY[binIndex] * P2[binIndex] * F12 * P12 +
               18 * ppY[binIndex] * F12 * F2[binIndex] * P12 - 7 * ppY[binIndex] * F12 * F2[binIndex] * P1[binIndex] -
               12 * ppY[binIndex] * P23 * F12 * P1[binIndex] + 24 * ppY[binIndex] * P23 * F12 * P12 -
               4 * ppY[binIndex] * F12 * F2[binIndex] * P23 - 3 * ppY[binIndex] * P1[binIndex] * P22 +
               ppY[binIndex] * F2[binIndex] * P12 - 3 * ppY[binIndex] * P12 * P2[binIndex] +
               3 * P12 * mmY[binIndex] * P2[binIndex] - 9 * P12 * mmY[binIndex] * P22 + 9 * ppY[binIndex] * P12 * P22 +
               ppY[binIndex] * P1[binIndex] * P2[binIndex] + 3 * P1[binIndex] * mmY[binIndex] * P22 -
               8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               40 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex] -
               40 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P22 -
               64 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 * P2[binIndex] +
               64 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 * P22 +
               34 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] * P1[binIndex] -
               52 * ppY[binIndex] * F12 * F2[binIndex] * P22 * P1[binIndex] -
               84 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P2[binIndex] +
               120 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P22 +
               88 * ppY[binIndex] * F12 * F2[binIndex] * P13 * P2[binIndex] -
               112 * ppY[binIndex] * F12 * F2[binIndex] * P13 * P22 +
               24 * ppY[binIndex] * F12 * F2[binIndex] * P23 * P1[binIndex] -
               48 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P23 + 2 * ppY[binIndex] * P13 * P2[binIndex] -
               6 * ppY[binIndex] * P13 * P22 - 3 * ppY[binIndex] * F2[binIndex] * P13 +
               2 * ppY[binIndex] * P1[binIndex] * P23 - 6 * ppY[binIndex] * P12 * P23 +
               ppY[binIndex] * P2[binIndex] * F12 - 3 * ppY[binIndex] * P22 * F12 + ppY[binIndex] * F12 * F2[binIndex] +
               2 * ppY[binIndex] * P23 * F12 - 2 * P13 * mmY[binIndex] * P2[binIndex] + 6 * P13 * mmY[binIndex] * P22 +
               6 * P12 * mmY[binIndex] * P23 - 2 * P1[binIndex] * mmY[binIndex] * P23) /
              (divisor1 * divisor1);
          const double divisor2 =
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dF2 =
              P2[binIndex] * F1[binIndex] *
              (3 * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
               12 * ppY[binIndex] * P22 * F1[binIndex] * P1[binIndex] -
               36 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P12 + 24 * ppY[binIndex] * P22 * F1[binIndex] * P12 +
               18 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] +
               12 * ppY[binIndex] * F1[binIndex] * P12 + 24 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P13 -
               16 * ppY[binIndex] * P22 * F1[binIndex] * P13 + 12 * ppY[binIndex] * P22 * F12 * P1[binIndex] -
               24 * ppY[binIndex] * P22 * F12 * P12 + 16 * ppY[binIndex] * P22 * F12 * P13 -
               18 * ppY[binIndex] * P2[binIndex] * F12 * P1[binIndex] - 24 * ppY[binIndex] * P2[binIndex] * F12 * P13 +
               36 * ppY[binIndex] * P2[binIndex] * F12 * P12 -
               19 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P2[binIndex] +
               28 * F1[binIndex] * P12 * mmY[binIndex] * P2[binIndex] -
               12 * F1[binIndex] * P13 * mmY[binIndex] * P2[binIndex] +
               22 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P22 - 28 * F1[binIndex] * P12 * mmY[binIndex] * P22 +
               8 * F1[binIndex] * P13 * mmY[binIndex] * P22 - 8 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P23 +
               8 * F1[binIndex] * P12 * mmY[binIndex] * P23 - ppY[binIndex] * F12 + 2 * ppY[binIndex] * P13 -
               2 * P13 * mmY[binIndex] - mmY[binIndex] * F1[binIndex] + 2 * ppY[binIndex] * P1[binIndex] * P22 +
               9 * ppY[binIndex] * P12 * P2[binIndex] - 9 * P12 * mmY[binIndex] * P2[binIndex] +
               6 * P12 * mmY[binIndex] * P22 - 6 * ppY[binIndex] * P12 * P22 -
               3 * ppY[binIndex] * P1[binIndex] * P2[binIndex] - 2 * P1[binIndex] * mmY[binIndex] * P22 -
               6 * ppY[binIndex] * F1[binIndex] * P1[binIndex] + 2 * ppY[binIndex] * P22 * F1[binIndex] -
               3 * ppY[binIndex] * P2[binIndex] * F1[binIndex] - P1[binIndex] * mmY[binIndex] +
               ppY[binIndex] * P1[binIndex] - 3 * ppY[binIndex] * P12 + ppY[binIndex] * F1[binIndex] +
               3 * P12 * mmY[binIndex] - 6 * ppY[binIndex] * P13 * P2[binIndex] + 4 * ppY[binIndex] * P13 * P22 +
               3 * ppY[binIndex] * P2[binIndex] * F12 - 2 * ppY[binIndex] * P22 * F12 +
               5 * F1[binIndex] * P1[binIndex] * mmY[binIndex] + 6 * ppY[binIndex] * F12 * P1[binIndex] -
               8 * F1[binIndex] * P12 * mmY[binIndex] - 12 * F12 * P12 * ppY[binIndex] -
               8 * ppY[binIndex] * F1[binIndex] * P13 + 6 * P13 * mmY[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * P13 * mmY[binIndex] + 8 * F12 * P13 * ppY[binIndex] - 4 * P13 * mmY[binIndex] * P22 -
               5 * mmY[binIndex] * P22 * F1[binIndex] + 2 * mmY[binIndex] * P23 * F1[binIndex] +
               4 * mmY[binIndex] * P2[binIndex] * F1[binIndex]) /
              (divisor2 * divisor2);
          const double divisor3 =
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dP1 =
              -F1[binIndex] * F2[binIndex] *
              (-2 * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] +
               8 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               24 * ppY[binIndex] * P22 * F1[binIndex] * P1[binIndex] +
               8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P22 +
               8 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P12 +
               6 * ppY[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 - 24 * ppY[binIndex] * P22 * F1[binIndex] * P12 -
               12 * ppY[binIndex] * F2[binIndex] * P12 * P22 -
               8 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] -
               2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               ppY[binIndex] * F2[binIndex] * P2[binIndex] - 4 * ppY[binIndex] * F2[binIndex] * P22 -
               8 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P23 -
               16 * ppY[binIndex] * P23 * F1[binIndex] * P1[binIndex] -
               8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P23 + 16 * ppY[binIndex] * P23 * F1[binIndex] * P12 +
               8 * ppY[binIndex] * F2[binIndex] * P12 * P23 - 24 * ppY[binIndex] * P22 * F12 * P1[binIndex] +
               24 * ppY[binIndex] * P22 * F12 * P12 + 8 * ppY[binIndex] * P2[binIndex] * F12 * P1[binIndex] +
               6 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] - 12 * ppY[binIndex] * F12 * F2[binIndex] * P22 -
               8 * ppY[binIndex] * P2[binIndex] * F12 * P12 - 4 * ppY[binIndex] * F12 * F2[binIndex] * P12 +
               4 * ppY[binIndex] * F12 * F2[binIndex] * P1[binIndex] + 16 * ppY[binIndex] * P23 * F12 * P1[binIndex] -
               16 * ppY[binIndex] * P23 * F12 * P12 + 8 * ppY[binIndex] * F12 * F2[binIndex] * P23 +
               4 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
               4 * F1[binIndex] * P12 * mmY[binIndex] * P2[binIndex] -
               12 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P22 + 12 * F1[binIndex] * P12 * mmY[binIndex] * P22 +
               8 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P23 - 8 * F1[binIndex] * P12 * mmY[binIndex] * P23 +
               2 * mmY[binIndex] * P23 - 2 * ppY[binIndex] * P23 + 4 * ppY[binIndex] * F2[binIndex] * P23 -
               6 * ppY[binIndex] * P1[binIndex] * P22 - ppY[binIndex] * F2[binIndex] * P12 -
               2 * ppY[binIndex] * P12 * P2[binIndex] + 2 * P12 * mmY[binIndex] * P2[binIndex] -
               6 * P12 * mmY[binIndex] * P22 + 6 * ppY[binIndex] * P12 * P22 +
               2 * ppY[binIndex] * P1[binIndex] * P2[binIndex] - ppY[binIndex] * P2[binIndex] +
               6 * P1[binIndex] * mmY[binIndex] * P22 - 6 * ppY[binIndex] * P22 * F1[binIndex] +
               2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] + 3 * ppY[binIndex] * P22 +
               16 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               40 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 -
               24 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
               48 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P22 + mmY[binIndex] * P2[binIndex] -
               3 * mmY[binIndex] * P22 + 32 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P23 -
               32 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P23 -
               24 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] * P1[binIndex] +
               48 * ppY[binIndex] * F12 * F2[binIndex] * P22 * P1[binIndex] +
               24 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P2[binIndex] -
               48 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P22 -
               32 * ppY[binIndex] * F12 * F2[binIndex] * P23 * P1[binIndex] +
               32 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P23 + 4 * ppY[binIndex] * P1[binIndex] * P23 +
               4 * ppY[binIndex] * P23 * F1[binIndex] - 4 * ppY[binIndex] * P12 * P23 -
               2 * ppY[binIndex] * P2[binIndex] * F12 + 6 * ppY[binIndex] * P22 * F12 -
               ppY[binIndex] * F12 * F2[binIndex] - 4 * ppY[binIndex] * P23 * F12 + 4 * P12 * mmY[binIndex] * P23 -
               4 * P1[binIndex] * mmY[binIndex] * P23 + 3 * mmY[binIndex] * P22 * F1[binIndex] -
               2 * mmY[binIndex] * P23 * F1[binIndex] - mmY[binIndex] * P2[binIndex] * F1[binIndex]) /
              (divisor3 * divisor3);
          const double divisor4 =
              (-P2[binIndex] * F1[binIndex] + 3 * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * P22 * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P22 -
               2 * F2[binIndex] * P12 * P2[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P12 +
               2 * P2[binIndex] * F1[binIndex] * P1[binIndex] + P22 * F1[binIndex] + F2[binIndex] * P12 +
               3 * F1[binIndex] * F2[binIndex] * P1[binIndex] + 2 * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               F1[binIndex] * F2[binIndex] - F2[binIndex] * P1[binIndex] -
               8 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               4 * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex]);
          const double dP2 =
              F1[binIndex] * F2[binIndex] *
              (-2 * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] +
               4 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P22 +
               12 * ppY[binIndex] * P22 * F1[binIndex] * P1[binIndex] +
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P22 +
               24 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P12 +
               12 * ppY[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
               12 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 - 24 * ppY[binIndex] * P22 * F1[binIndex] * P12 -
               12 * ppY[binIndex] * F2[binIndex] * P12 * P22 -
               12 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] -
               6 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] -
               4 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               12 * ppY[binIndex] * F1[binIndex] * P12 - 16 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P13 +
               16 * ppY[binIndex] * P22 * F1[binIndex] * P13 - 8 * ppY[binIndex] * F2[binIndex] * P13 * P2[binIndex] +
               8 * ppY[binIndex] * F2[binIndex] * P13 * P22 - 8 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 -
               12 * ppY[binIndex] * P22 * F12 * P1[binIndex] + 8 * ppY[binIndex] * F12 * F2[binIndex] * P13 +
               24 * ppY[binIndex] * P22 * F12 * P12 - 16 * ppY[binIndex] * P22 * F12 * P13 +
               12 * ppY[binIndex] * P2[binIndex] * F12 * P1[binIndex] +
               4 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] - 4 * ppY[binIndex] * F12 * F2[binIndex] * P22 +
               16 * ppY[binIndex] * P2[binIndex] * F12 * P13 - 24 * ppY[binIndex] * P2[binIndex] * F12 * P12 -
               12 * ppY[binIndex] * F12 * F2[binIndex] * P12 + 6 * ppY[binIndex] * F12 * F2[binIndex] * P1[binIndex] +
               10 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P2[binIndex] -
               16 * F1[binIndex] * P12 * mmY[binIndex] * P2[binIndex] +
               8 * F1[binIndex] * P13 * mmY[binIndex] * P2[binIndex] -
               6 * F1[binIndex] * P1[binIndex] * mmY[binIndex] * P22 + 12 * F1[binIndex] * P12 * mmY[binIndex] * P22 -
               8 * F1[binIndex] * P13 * mmY[binIndex] * P22 + ppY[binIndex] * F12 - 2 * ppY[binIndex] * P13 +
               2 * P13 * mmY[binIndex] + mmY[binIndex] * F1[binIndex] - 2 * ppY[binIndex] * P1[binIndex] * P22 +
               ppY[binIndex] * F2[binIndex] * P1[binIndex] - 3 * ppY[binIndex] * F2[binIndex] * P12 -
               6 * ppY[binIndex] * P12 * P2[binIndex] + 6 * P12 * mmY[binIndex] * P2[binIndex] -
               6 * P12 * mmY[binIndex] * P22 + 6 * ppY[binIndex] * P12 * P22 +
               2 * ppY[binIndex] * P1[binIndex] * P2[binIndex] + ppY[binIndex] * F1[binIndex] * F2[binIndex] +
               2 * P1[binIndex] * mmY[binIndex] * P22 + 6 * ppY[binIndex] * F1[binIndex] * P1[binIndex] -
               2 * ppY[binIndex] * P22 * F1[binIndex] + 2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] +
               24 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               24 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P22 -
               48 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P2[binIndex] +
               48 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P12 * P22 + P1[binIndex] * mmY[binIndex] -
               ppY[binIndex] * P1[binIndex] + 3 * ppY[binIndex] * P12 - ppY[binIndex] * F1[binIndex] -
               3 * P12 * mmY[binIndex] + 32 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 * P2[binIndex] -
               32 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P13 * P22 -
               24 * ppY[binIndex] * F12 * F2[binIndex] * P2[binIndex] * P1[binIndex] +
               24 * ppY[binIndex] * F12 * F2[binIndex] * P22 * P1[binIndex] +
               48 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P2[binIndex] -
               48 * ppY[binIndex] * F12 * F2[binIndex] * P12 * P22 -
               32 * ppY[binIndex] * F12 * F2[binIndex] * P13 * P2[binIndex] +
               32 * ppY[binIndex] * F12 * F2[binIndex] * P13 * P22 + 4 * ppY[binIndex] * P13 * P2[binIndex] -
               4 * ppY[binIndex] * P13 * P22 + 2 * ppY[binIndex] * F2[binIndex] * P13 -
               2 * ppY[binIndex] * P2[binIndex] * F12 + 2 * ppY[binIndex] * P22 * F12 -
               ppY[binIndex] * F12 * F2[binIndex] - 5 * F1[binIndex] * P1[binIndex] * mmY[binIndex] -
               6 * ppY[binIndex] * F12 * P1[binIndex] + 8 * F1[binIndex] * P12 * mmY[binIndex] +
               12 * F12 * P12 * ppY[binIndex] + 8 * ppY[binIndex] * F1[binIndex] * P13 -
               4 * P13 * mmY[binIndex] * P2[binIndex] - 4 * F1[binIndex] * P13 * mmY[binIndex] -
               8 * F12 * P13 * ppY[binIndex] + 4 * P13 * mmY[binIndex] * P22 + mmY[binIndex] * P22 * F1[binIndex] -
               2 * mmY[binIndex] * P2[binIndex] * F1[binIndex]) /
              (divisor4 * divisor4);
          const double e1 = dI00 * ppE[binIndex];
          const double e2 = dI11 * mmE[binIndex];
          const double e3 = dF1 * F1E[binIndex];
          const double e4 = dF2 * F2E[binIndex];
          const double e5 = dP1 * P1E[binIndex];
          const double e6 = dP2 * P2E[binIndex];
          mpE[binIndex] = std::sqrt(e1 * e1 + e2 * e2 + e3 * e3 + e4 * e4 + e5 * e5 + e6 * e6);
        }
        {
          pmY[binIndex] = -(ppY[binIndex] * P2[binIndex] * F1[binIndex] -
                            2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
                            2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] -
                            2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] +
                            2 * P1[binIndex] * mpY[binIndex] * F2[binIndex] * P2[binIndex] +
                            ppY[binIndex] * P1[binIndex] * P2[binIndex] - P1[binIndex] * mpY[binIndex] * P2[binIndex] +
                            4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
                            P1[binIndex] * mmY[binIndex] * P2[binIndex] - ppY[binIndex] * F1[binIndex] +
                            2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] - P1[binIndex] * mmY[binIndex] -
                            P1[binIndex] * mpY[binIndex] * F2[binIndex] + ppY[binIndex] * F2[binIndex] * P1[binIndex] +
                            ppY[binIndex] * F1[binIndex] * F2[binIndex] -
                            2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] +
                            P1[binIndex] * mpY[binIndex] - ppY[binIndex] * P1[binIndex]) /
                          ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex]));
          const double dI00 =
              -(-P1[binIndex] + P1[binIndex] * P2[binIndex] + F2[binIndex] * P1[binIndex] -
                2 * F2[binIndex] * P1[binIndex] * P2[binIndex] + 2 * F1[binIndex] * P1[binIndex] -
                2 * P2[binIndex] * F1[binIndex] * P1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P1[binIndex] +
                4 * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] + F1[binIndex] * F2[binIndex] -
                F1[binIndex] + P2[binIndex] * F1[binIndex] - 2 * F1[binIndex] * F2[binIndex] * P2[binIndex]) /
              ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex]));
          const double dI11 = -(P1[binIndex] * P2[binIndex] - P1[binIndex]) /
                              ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex]));
          const double dI10 = -(P1[binIndex] - P1[binIndex] * P2[binIndex] - F2[binIndex] * P1[binIndex] +
                                2 * F2[binIndex] * P1[binIndex] * P2[binIndex]) /
                              ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex]));
          const double factor1 = (-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]);
          const double dF1 =
              -(ppY[binIndex] * P2[binIndex] - 2 * ppY[binIndex] * F2[binIndex] * P2[binIndex] -
                2 * ppY[binIndex] * P1[binIndex] * P2[binIndex] +
                4 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] - ppY[binIndex] +
                2 * ppY[binIndex] * P1[binIndex] + ppY[binIndex] * F2[binIndex] -
                2 * ppY[binIndex] * F2[binIndex] * P1[binIndex]) /
                  ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex])) +
              (ppY[binIndex] * P2[binIndex] * F1[binIndex] -
               2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] +
               2 * P1[binIndex] * mpY[binIndex] * F2[binIndex] * P2[binIndex] +
               ppY[binIndex] * P1[binIndex] * P2[binIndex] - P1[binIndex] * mpY[binIndex] * P2[binIndex] +
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               P1[binIndex] * mmY[binIndex] * P2[binIndex] - ppY[binIndex] * F1[binIndex] +
               2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] - P1[binIndex] * mmY[binIndex] -
               P1[binIndex] * mpY[binIndex] * F2[binIndex] + ppY[binIndex] * F2[binIndex] * P1[binIndex] +
               ppY[binIndex] * F1[binIndex] * F2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] + P1[binIndex] * mpY[binIndex] -
               ppY[binIndex] * P1[binIndex]) *
                  (-1 + 2 * P1[binIndex]) / ((factor1 * factor1) * (-1 + P2[binIndex]));
          const double dF2 =
              -(-2 * ppY[binIndex] * P1[binIndex] * P2[binIndex] - 2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] +
                2 * P1[binIndex] * mpY[binIndex] * P2[binIndex] +
                4 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] - P1[binIndex] * mpY[binIndex] +
                ppY[binIndex] * P1[binIndex] + ppY[binIndex] * F1[binIndex] -
                2 * ppY[binIndex] * F1[binIndex] * P1[binIndex]) /
              ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex]));
          const double factor2 = (-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]);
          const double dP1 =
              -(-2 * ppY[binIndex] * F2[binIndex] * P2[binIndex] - 2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] +
                2 * mpY[binIndex] * F2[binIndex] * P2[binIndex] + ppY[binIndex] * P2[binIndex] -
                mpY[binIndex] * P2[binIndex] + 4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] +
                mmY[binIndex] * P2[binIndex] + 2 * ppY[binIndex] * F1[binIndex] - mmY[binIndex] -
                mpY[binIndex] * F2[binIndex] + ppY[binIndex] * F2[binIndex] -
                2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] + mpY[binIndex] - ppY[binIndex]) /
                  ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex])) +
              (ppY[binIndex] * P2[binIndex] * F1[binIndex] -
               2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] +
               2 * P1[binIndex] * mpY[binIndex] * F2[binIndex] * P2[binIndex] +
               ppY[binIndex] * P1[binIndex] * P2[binIndex] - P1[binIndex] * mpY[binIndex] * P2[binIndex] +
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               P1[binIndex] * mmY[binIndex] * P2[binIndex] - ppY[binIndex] * F1[binIndex] +
               2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] - P1[binIndex] * mmY[binIndex] -
               P1[binIndex] * mpY[binIndex] * F2[binIndex] + ppY[binIndex] * F2[binIndex] * P1[binIndex] +
               ppY[binIndex] * F1[binIndex] * F2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] + P1[binIndex] * mpY[binIndex] -
               ppY[binIndex] * P1[binIndex]) *
                  (-1 + 2 * F1[binIndex]) / ((factor2 * factor2) * (-1 + P2[binIndex]));
          const double factor3 = (-1 + P2[binIndex]);
          const double dP2 =
              -(ppY[binIndex] * F1[binIndex] - 2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] -
                2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] - 2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] +
                2 * P1[binIndex] * mpY[binIndex] * F2[binIndex] + ppY[binIndex] * P1[binIndex] -
                P1[binIndex] * mpY[binIndex] + 4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] +
                P1[binIndex] * mmY[binIndex]) /
                  ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (-1 + P2[binIndex])) +
              (ppY[binIndex] * P2[binIndex] * F1[binIndex] -
               2 * ppY[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P2[binIndex] -
               2 * ppY[binIndex] * P2[binIndex] * F1[binIndex] * P1[binIndex] +
               2 * P1[binIndex] * mpY[binIndex] * F2[binIndex] * P2[binIndex] +
               ppY[binIndex] * P1[binIndex] * P2[binIndex] - P1[binIndex] * mpY[binIndex] * P2[binIndex] +
               4 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] * P2[binIndex] +
               P1[binIndex] * mmY[binIndex] * P2[binIndex] - ppY[binIndex] * F1[binIndex] +
               2 * ppY[binIndex] * F1[binIndex] * P1[binIndex] - P1[binIndex] * mmY[binIndex] -
               P1[binIndex] * mpY[binIndex] * F2[binIndex] + ppY[binIndex] * F2[binIndex] * P1[binIndex] +
               ppY[binIndex] * F1[binIndex] * F2[binIndex] -
               2 * ppY[binIndex] * F1[binIndex] * F2[binIndex] * P1[binIndex] + P1[binIndex] * mpY[binIndex] -
               ppY[binIndex] * P1[binIndex]) /
                  ((-P1[binIndex] + 2 * F1[binIndex] * P1[binIndex] - F1[binIndex]) * (factor3 * factor3));
          const double e1 = dI00 * ppE[binIndex];
          const double e2 = dI11 * mmE[binIndex];
          const double e3 = dI10 * mpE[binIndex];
          const double e4 = dF1 * F1E[binIndex];
          const double e5 = dF2 * F2E[binIndex];
          const double e6 = dP1 * P1E[binIndex];
          const double e7 = dP2 * P2E[binIndex];
          pmE[binIndex] = std::sqrt(e1 * e1 + e2 * e2 + e3 * e3 + e4 * e4 + e5 * e5 + e6 * e6 + e7 * e7);
        }
      }
    }
  }

  void runIdealCaseFullCorrections(const std::string &flipperConfig, const std::array<std::string, 4> &outputOrder,
                                   const bool useSpinStates = false) {
    auto effWS = idealEfficiencies(edges);
    std::string spinStates = "";
    if (useSpinStates) {
      spinStates = outputOrder[0] + "," + outputOrder[1] + "," + outputOrder[2] + "," + outputOrder[3];
    }
    idealCaseFullCorrectionsTest(edges, effWS, outputOrder, flipperConfig, spinStates);
  }

  size_t getPolIndex(const std::string &pol) {
    static const std::unordered_map<std::string, size_t> polMap = {{"_++", 0}, {"_+-", 1}, {"_-+", 2}, {"_--", 3}};
    const auto it = polMap.find(pol);
    if (it != polMap.end()) {
      return it->second;
    }
    throw std::invalid_argument("Unknown polarization string: " + pol);
  }

  std::pair<MatrixWorkspace_sptr, MatrixWorkspace_sptr> setupFailureWorkspaceAndEfficiencies() {
    Counts counts{0., 0., 0.};

    auto const ws00 = createWorkspace(1, counts);
    const std::string wsName{"ws00"};
    addWorkspaceToService(wsName, ws00);

    auto effWS = idealEfficiencies(edges);

    return {ws00, effWS};
  }

  void prepareWorkspacesAndRunCorrectionWithSampleState(const Counts &counts, const BinEdges &edges, size_t numClones,
                                                        bool addSpinStateToLog,
                                                        const std::vector<std::string> &expectedLogValues,
                                                        const std::string &grouping = "") {
    constexpr size_t nHist{2};
    auto wsList = createWorkspaceList(numClones, counts);
    auto wsNames = generateWorkspaceNames(numClones);
    setWorkspacesTestData(wsNames, wsList, nHist);
    addWorkspacesToService(wsNames, wsList);

    const auto effWS = efficiencies(edges);
    auto alg = createWildesAlg(wsNames, effWS, grouping);
    alg->setProperty("AddSpinStateToLog", addSpinStateToLog);

    const auto outputWS = runAlg(std::move(alg));
    validateNumberOfEntries(outputWS, numClones);

    if (addSpinStateToLog) {
      validateSpinStateLogs(outputWS, expectedLogValues);
    } else {
      validateNoSpinStateLogs(outputWS, numClones);
    }
  }

  MatrixWorkspace_sptr createWorkspace(size_t nHist = 2, std::optional<Counts> counts = std::nullopt) {
    if (!counts) {
      counts = Counts{yVal, 4.2 * (yVal), yVal};
    }

    return create<Workspace2D>(nHist, Histogram(edges, *counts));
  }

  std::vector<MatrixWorkspace_sptr> cloneWorkspaces(const MatrixWorkspace_sptr &sourceWorkspace,
                                                    const size_t numClones) {
    std::vector<MatrixWorkspace_sptr> clonedWorkspaces;

    clonedWorkspaces.push_back(sourceWorkspace);
    for (size_t i = 1; i < numClones; ++i) {
      clonedWorkspaces.push_back(sourceWorkspace->clone());
    }

    return clonedWorkspaces;
  }

  std::vector<std::string> generateWorkspaceNames(const size_t numClones) {
    std::vector<std::string> wsNames;

    for (size_t i = 0; i < numClones; ++i) {
      wsNames.push_back("ws" + std::to_string(i));
    }

    return wsNames;
  }
  void addWorkspacesToService(const std::vector<std::string> &wsNames,
                              const std::vector<MatrixWorkspace_sptr> &wsList) {
    for (size_t i = 0; i < wsNames.size(); ++i) {
      AnalysisDataService::Instance().addOrReplace(wsNames[i], wsList[i]);
    }
  }

  void addWorkspaceToService(const std::string &wsName, const MatrixWorkspace_sptr &ws) {
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
  }

  void setWorkspacesTestData(std::vector<std::string> &wsNames,
                             const std::vector<Mantid::API::MatrixWorkspace_sptr> &wsList, const size_t nHist) {

    for (size_t i = 0; i != wsNames.size(); ++i) {
      for (size_t j = 0; j != nHist; ++j) {
        wsList[i]->mutableY(j) *= static_cast<double>(i + 1);
        wsList[i]->mutableE(j) *= static_cast<double>(i + 1);
      }
    }
  }

  std::vector<MatrixWorkspace_sptr> createWorkspaceList(const size_t numClones,
                                                        const std::optional<Counts> &counts = std::nullopt) {
    auto baseWorkspace = counts.has_value() ? createWorkspace(2, *counts) : createWorkspace();

    return cloneWorkspaces(baseWorkspace, numClones);
  }

  void setupWorkspacesForIdealCasesTwoInput(const std::vector<std::string> &wsNames,
                                            const std::vector<Mantid::API::MatrixWorkspace_sptr> &wsList,
                                            const size_t nHist) {

    for (size_t i = 0; i != nHist; ++i) {
      wsList[1]->mutableY(i) *= 2.;
      wsList[1]->mutableE(i) *= 2.;
    }

    addWorkspaceToService(wsNames.front(), wsList.front());
    addWorkspaceToService(wsNames.back(), wsList.back());
  }

  void validateNumberOfEntries(const WorkspaceGroup_sptr &workspaceGroup, const size_t expectedEntries) {
    TS_ASSERT_EQUALS(workspaceGroup->getNumberOfEntries(), expectedEntries);
  }

  void validateSpinStateLogs(const WorkspaceGroup_sptr &outputWS, const std::vector<std::string> &expectedLogValues) {
    const auto logName = SpinStatesORSO::LOG_NAME;
    for (size_t i = 0; i < expectedLogValues.size(); ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws);
      const auto &run = ws->run();
      TS_ASSERT(run.hasProperty(logName));
      TS_ASSERT_EQUALS(run.getPropertyValueAsType<std::string>(logName), expectedLogValues[i]);
    }
  }

  void validateNoSpinStateLogs(const WorkspaceGroup_sptr &outputWS, size_t numClones) {
    for (size_t i = 0; i < numClones; ++i) {
      MatrixWorkspace_sptr ws = std::dynamic_pointer_cast<MatrixWorkspace>(outputWS->getItem(i));
      TS_ASSERT(ws);
      TS_ASSERT(!ws->run().hasProperty(SpinStatesORSO::LOG_NAME));
    }
  }
};

class PolarizationCorrectionWildesTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    auto loadWS = AlgorithmManager::Instance().createUnmanaged("LoadILLReflectometry");
    loadWS->setChild(true);
    loadWS->initialize();
    loadWS->setProperty("Filename", "ILL/D17/317370.nxs");
    loadWS->setProperty("OutputWorkspace", "output");
    loadWS->setProperty("XUnit", "TimeOfFlight");
    loadWS->execute();
    m_ws00 = loadWS->getProperty("OutputWorkspace");
    auto groupDetectors = AlgorithmManager::Instance().createUnmanaged("GroupDetectors");
    groupDetectors->setChild(true);
    groupDetectors->initialize();
    groupDetectors->setProperty("InputWorkspace", m_ws00);
    groupDetectors->setProperty("OutputWorkspace", "output");
    groupDetectors->setPropertyValue("WorkspaceIndexList", "201, 202, 203");
    groupDetectors->execute();
    m_ws00 = groupDetectors->getProperty("OutputWorkspace");
    auto convertUnits = AlgorithmManager::Instance().createUnmanaged("ConvertUnits");
    convertUnits->setChild(true);
    convertUnits->initialize();
    convertUnits->setProperty("InputWorkspace", m_ws00);
    convertUnits->setProperty("OutputWorkspace", "output");
    convertUnits->setProperty("Target", "Wavelength");
    convertUnits->execute();
    m_ws00 = convertUnits->getProperty("OutputWorkspace");
    auto crop = AlgorithmManager::Instance().createUnmanaged("CropWorkspace");
    crop->setChild(true);
    crop->initialize();
    crop->setProperty("InputWorkspace", m_ws00);
    crop->setProperty("OutputWorkspace", "output");
    crop->setProperty("XMin", 0.);
    crop->execute();
    m_ws00 = crop->getProperty("OutputWorkspace");
    AnalysisDataService::Instance().addOrReplace("00", m_ws00);
    m_ws01 = m_ws00->clone();
    AnalysisDataService::Instance().addOrReplace("01", m_ws01);
    m_ws10 = m_ws00->clone();
    AnalysisDataService::Instance().addOrReplace("10", m_ws10);
    m_ws11 = m_ws00->clone();
    AnalysisDataService::Instance().addOrReplace("11", m_ws11);
    auto loadEff = AlgorithmManager::Instance().createUnmanaged("LoadILLPolarizationFactors");
    loadEff->setChild(true);
    loadEff->initialize();
    loadEff->setProperty("Filename", "ILL/D17/PolarizationFactors.txt");
    loadEff->setProperty("OutputWorkspace", "output");
    loadEff->setProperty("WavelengthReference", m_ws00);
    loadEff->execute();
    m_effWS = loadEff->getProperty("OutputWorkspace");
  }

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_DirectBeamPerformance() { runPerformanceTest("00", "0"); }

  void test_ThreeInputsPerformanceMissing01() { runPerformanceTest("00, 10, 11", "00, 10, 11"); }

  void test_ThreeInputsPerformanceMissing10() { runPerformanceTest("00, 01, 11", "00, 01, 11"); }

  void test_TwoInputsNoAnalyzerPerformance() { runPerformanceTest("00, 11", "0, 1"); }

  void test_TwoInputsPerformance() { runPerformanceTest("00, 11", "00, 11"); }

private:
  Mantid::API::MatrixWorkspace_sptr m_effWS;
  Mantid::API::MatrixWorkspace_sptr m_ws00;
  Mantid::API::MatrixWorkspace_sptr m_ws01;
  Mantid::API::MatrixWorkspace_sptr m_ws10;
  Mantid::API::MatrixWorkspace_sptr m_ws11;

  void runPerformanceTest(const std::string &inputWorkspaces, const std::string &flippers) {
    for (int i = 0; i < 3000; ++i) {
      PolarizationCorrectionWildes correction;
      correction.setChild(true);
      correction.setRethrows(true);
      correction.initialize();
      correction.setProperty("InputWorkspaces", inputWorkspaces);
      correction.setProperty("OutputWorkspace", "output");
      correction.setProperty("Flippers", flippers);
      correction.setProperty("Efficiencies", m_effWS);
      TS_ASSERT_THROWS_NOTHING(correction.execute())
    }
  }
};
