// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#pragma once
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"
#include "PolarizationCorrectionsTestUtils.h"
#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using namespace PolCorrTestUtils;

struct PolarizationTestParameters {
  PolarizationTestParameters() = default;
  PolarizationTestParameters(const double tauIni, const double polIni, const double pxdIni)
      : Tau(tauIni), polInitial(polIni), pxd(pxdIni) {}
  double Tau{DEFAULT_LIFETIME};
  double polInitial{DEFAULT_INI_POL};
  double pxd{DEFAULT_PXD};
  double pxdError{0};
  std::vector<double> outPolarizations{};
  std::vector<MatrixWorkspace_sptr> outEfficiencies{};
};

class HeliumAnalyserEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_parameters = TestWorkspaceParameters();
    m_parameters.updateSpectra(1, 1.75, 8, .5);
    m_polParameters = PolarizationTestParameters();
  }
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void test_name() {
    HeliumAnalyserEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), HE_EFF_ALG);
  }

  void test_init() {
    HeliumAnalyserEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void test_input_workspace_not_a_group_throws() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    MatrixWorkspace_sptr test = generateFunctionDefinedWorkspace(m_parameters);
    const auto alg = prepareHeEffAlgorithm({m_parameters.testName});
    TSM_ASSERT_THROWS("InputWorkspaces has to be a group", alg->execute(), const std::runtime_error &);
  }

  void test_input_workspace_wrong_size_group_throws() {
    // The units of the input workspace should be wavelength
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    AnalysisDataService::Instance().remove("T0_11");
    const auto alg = prepareHeEffAlgorithm(groupNames);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: \n InputWorkspaces: Error in workspace T0 : The "
                            "number of periods within the input workspace is not an allowed value.");
  }

  void test_invalid_spin_throws() {
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create(HE_EFF_ALG);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "10,01"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
  }

  void test_tau_is_not_fit_for_one_input_workspace() {
    // Fitting with one input workspace is equivalent to calculating helium efficiency at t=0
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    const auto alg = prepareHeEffAlgorithm(groupNames);

    alg->execute();
    TS_ASSERT(alg->isExecuted());
    const auto groupOut = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_NAME);
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), 1);
    TS_ASSERT_DELTA(m_polParameters.polInitial, m_polParameters.outPolarizations.at(0), 1e-4);
  }

  void test_child_algorithm_exec() {
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    const auto alg = prepareHeEffAlgorithm(groupNames);
    alg->setChild(true);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const WorkspaceGroup_sptr groupOut = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), 1);
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(groupOut->getItem(0));
    TS_ASSERT_EQUALS(wsOut->getNumberHistograms(), 1);
  }

  void test_order_of_time_differences_does_not_matter() {
    const std::vector<std::vector<double>> delays = {{0, 10, 20}, {10, 20, 0}, {20, 0, 10}};
    for (const auto &delayVec : delays) {
      const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization(delayVec);
      const auto alg =
          prepareHeEffAlgorithm(groupNames, OUTPUT_NAME, SPIN_STATE, OUTPUT_TABLE_NAME, OUTPUT_CURVES_NAME);

      alg->execute();
      TS_ASSERT(alg->isExecuted());
      assertOutputNames(groupNames.size());
      assertOutputs(groupNames.size());
    }
  }

  void test_fit_with_different_lifetimes_and_delays() {
    const std::vector<double> delays = {0, 10, 20};
    const std::vector<double> lifetimes = {1, 100, 500};
    for (const auto &tau : lifetimes) {
      m_polParameters.Tau = tau;
      const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization(delays);
      const auto alg =
          prepareHeEffAlgorithm(groupNames, OUTPUT_NAME, SPIN_STATE, OUTPUT_TABLE_NAME, OUTPUT_CURVES_NAME);

      alg->execute();
      TS_ASSERT(alg->isExecuted());
      assertOutputNames(groupNames.size());
      assertOutputs(groupNames.size());
    }
  }

private:
  IAlgorithm_sptr prepareHeEffAlgorithm(const std::vector<std::string> &inputWorkspaces,
                                        const std::string &outputName = OUTPUT_NAME,
                                        const std::string &spinState = SPIN_STATE,
                                        const std::string &outputFitParameters = "",
                                        const std::string &outputFitCurves = "") {
    const auto heAlgorithm = AlgorithmManager::Instance().create(HE_EFF_ALG);
    heAlgorithm->initialize();
    heAlgorithm->setProperty("InputWorkspaces", inputWorkspaces);
    heAlgorithm->setProperty("SpinStates", spinState);
    heAlgorithm->setProperty("OutputWorkspace", outputName);

    heAlgorithm->setProperty("OutputFitParameters", outputFitParameters);
    heAlgorithm->setProperty("OutputFitCurves", outputFitCurves);
    return heAlgorithm;
  }

  std::vector<std::string> generateEfficienciesFromLifetimeAndInitialPolarization(const std::vector<double> &delays = {
                                                                                      0}) {
    std::vector<double> polarizations;
    std::transform(delays.cbegin(), delays.cend(), std::back_inserter(polarizations), [&](const double delay) {
      return m_polParameters.polInitial * std::exp(-delay / m_polParameters.Tau);
    });
    m_polParameters.outPolarizations = polarizations;
    m_polParameters.outEfficiencies = std::vector<MatrixWorkspace_sptr>(delays.size());
    std::vector<std::string> names(delays.size(), "T");
    const double mu = LAMBDA_CONVERSION_FACTOR * m_polParameters.pxd;

    for (size_t index = 0; index < delays.size(); index++) {
      const auto phe = polarizations.at(index);
      const auto effFunc = fillFuncStr({mu * phe}, EFFICIENCY_FUNC_STR);
      const auto yNsfFunc = fillFuncStr({mu * (1 - phe)}, SPIN_TEST_FUNC_STR);
      const auto ySfFunc = fillFuncStr({mu * (1 + phe)}, SPIN_TEST_FUNC_STR);
      m_parameters.delay = delays.at(index);
      const std::string groupIndexStr = std::to_string(index);
      names.at(index) += groupIndexStr;
      (void)createPolarizedTestGroup(names.at(index), m_parameters,
                                     std::vector({yNsfFunc, ySfFunc, ySfFunc, yNsfFunc}));
      m_parameters.updateNameAndFunc("eff" + groupIndexStr, effFunc);
      m_polParameters.outEfficiencies.at(index) = generateFunctionDefinedWorkspace(m_parameters);
    }
    return names;
  }

  void assertOutputNames(const size_t HeFitSize = 1) {
    const auto adsNames = AnalysisDataService::Instance().getObjectNames();
    std::vector<std::string> expectedNamesCurves = {OUTPUT_CURVES_NAME + "_decay_curves_0"};
    auto basenames = std::vector<std::string>(HeFitSize, OUTPUT_CURVES_NAME + "_He3_polarization_curves_");
    std::for_each(basenames.begin(), basenames.end(), [n = 0](auto &name) mutable { name += std::to_string(n++); });
    expectedNamesCurves.insert(expectedNamesCurves.cend(), basenames.cbegin(), basenames.cend());
    TS_ASSERT(AnalysisDataService::Instance().doesExist(OUTPUT_CURVES_NAME));
    TS_ASSERT(std::ranges::includes(adsNames, expectedNamesCurves));
  }

  void assertOutputs(const size_t HeFitSize = 1) {

    // Assert Efficiencies
    const auto groupOut = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_NAME);
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), HeFitSize);
    for (size_t index = 0; index < HeFitSize; index++) {
      const auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(groupOut->getItem(index));
      const auto &y = ws->dataY(0);
      TS_ASSERT_DELTA(y, m_polParameters.outEfficiencies.at(index)->dataY(0), DELTA);
    }

    // Assert Polarization parameters
    const auto polTable = AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(OUTPUT_TABLE_NAME);
    const auto values = polTable->getColumn("Value")->numeric_fill();
    TS_ASSERT_DELTA(std::vector<double>(values.cbegin(), values.cbegin() + HeFitSize), m_polParameters.outPolarizations,
                    DELTA);

    // Assert Decay Parameters
    if (HeFitSize > 1) {
      TS_ASSERT_DELTA(values.at(HeFitSize + 1), m_polParameters.polInitial, DELTA);
      TS_ASSERT_DELTA(values.at(HeFitSize + 2), m_polParameters.Tau, DELTA);
    }
  }

  TestWorkspaceParameters m_parameters{};
  PolarizationTestParameters m_polParameters{};
};
