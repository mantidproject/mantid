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

#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/PolarizationCorrections/HeliumAnalyserEfficiency.h"
#include "MantidKernel/ArrayProperty.h"

#include <cmath>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

namespace HePolDecayTest {

static const std::string GROUP_NAME = "group";
static const std::string OUTPUT_NAME = "groupOut";
static const std::string OUTPUT_TABLE_NAME = "tableOut";
static const std::string OUTPUT_CURVES_NAME = "curvesOut";
static const std::string TEST_SPIN_STATE = "00,01,10,11";
static const std::string TEST_ALGORITHM = "HeliumAnalyserEfficiency";
static const auto TIME_ORIGIN = DateAndTime("2025-07-01T08:00:00");
constexpr double DELTA = 0.01;

struct PolarizationTestParameters {
  PolarizationTestParameters() = default;
  PolarizationTestParameters(const double tauIni, const double polIni, const double pxdIni)
      : Tau(tauIni), polInitial(polIni), pxd(pxdIni), pxdError(), outPolarizations(), outEfficiencies() {}
  double Tau{45};
  double polInitial{0.6};
  double pxd{12};
  double pxdError{0};
  std::vector<double> outPolarizations{};
  std::vector<std::vector<double>> outEfficiencies{};
};

struct InputTestParameters {
  InputTestParameters() = default;
  InputTestParameters(const int spec, const int bins, const std::string &group, const std::string &wsName,
                      const std::string &units)
      : nSpec(spec), nBins(bins), groupName(group), testName(wsName), xUnit(units) {}
  int nSpec{1};
  int nBins{5};
  std::string groupName{GROUP_NAME};
  std::string testName{OUTPUT_NAME};
  std::string xUnit{"Wavelength"};
};

IAlgorithm_sptr prepareAlgorithm(const std::vector<std::string> &inputWorkspaces,
                                 const std::string &outputName = OUTPUT_NAME,
                                 const std::string &spinState = TEST_SPIN_STATE,
                                 const std::string &outputFitParameters = "", const std::string &outputFitCurves = "") {
  const auto heAlgorithm = AlgorithmManager::Instance().create(TEST_ALGORITHM);
  heAlgorithm->initialize();
  heAlgorithm->setProperty("InputWorkspaces", inputWorkspaces);
  heAlgorithm->setProperty("SpinStates", spinState);
  heAlgorithm->setProperty("OutputWorkspace", outputName);

  heAlgorithm->setProperty("OutputFitParameters", outputFitParameters);
  heAlgorithm->setProperty("OutputFitCurves", outputFitCurves);
  return heAlgorithm;
}

MatrixWorkspace_sptr generateWorkspace(const std::string &name, const std::vector<double> &x,
                                       const std::vector<double> &y, const std::string &xUnit = "Wavelength",
                                       const int nSpec = 1, const double delay = 1) {
  const auto createWorkspace = AlgorithmManager::Instance().create("CreateWorkspace");
  createWorkspace->initialize();
  createWorkspace->setProperty("DataX", x);
  createWorkspace->setProperty("DataY", y);
  createWorkspace->setProperty("UnitX", xUnit);
  createWorkspace->setProperty("NSpec", nSpec);
  createWorkspace->setProperty("OutputWorkspace", name);
  createWorkspace->execute();

  const auto convertToHistogram = AlgorithmManager::Instance().create("ConvertToHistogram");
  convertToHistogram->initialize();
  convertToHistogram->setProperty("InputWorkspace", name);
  convertToHistogram->setProperty("OutputWorkspace", name);
  convertToHistogram->execute();

  MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(name);
  // We are working with delays in hours but DateAndTime overloads + operator with seconds for type double, thus factor
  // 3600
  const auto start = TIME_ORIGIN + 3600 * delay;
  ws->mutableRun().setStartAndEndTime(start, start + 1.0);
  return ws;
}

void groupWorkspaces(const std::string &name, const std::vector<MatrixWorkspace_sptr> &wsToGroup) {
  const auto groupWorkspace = AlgorithmManager::Instance().create("GroupWorkspaces");
  groupWorkspace->initialize();
  std::vector<std::string> wsToGroupNames(wsToGroup.size());
  std::transform(wsToGroup.cbegin(), wsToGroup.cend(), wsToGroupNames.begin(),
                 [](const MatrixWorkspace_sptr &w) { return w->getName(); });
  groupWorkspace->setProperty("InputWorkspaces", wsToGroupNames);
  groupWorkspace->setProperty("OutputWorkspace", name);
  groupWorkspace->execute();
}

// TimeDifference is a python algorithm. This is a simple mock for running the tests.
class TimeDifference final : public Algorithm {
public:
  const std::string name() const override { return "TimeDifference"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "TimeDifference Mock Algorithm"; }

private:
  void init() override {
    declareProperty(std::make_unique<Kernel::ArrayProperty<std::string>>("InputWorkspaces"));
    declareProperty(std::make_unique<WorkspaceProperty<ITableWorkspace>>("OutputWorkspace", "out",
                                                                         Mantid::Kernel::Direction::Output),
                    "");
    declareProperty("ReferenceWorkspace", "");
  };

  void exec() override {
    const ITableWorkspace_sptr outputTable = WorkspaceFactory::Instance().createTable();
    const std::vector<std::string> workspaces = getProperty("InputWorkspaces");

    outputTable->addColumn("str", "ws_name");
    outputTable->addColumn("str", "midtime_stamp");
    outputTable->addColumn("float", "seconds");
    outputTable->addColumn("float", "seconds_error");
    outputTable->addColumn("float", "hours");
    outputTable->addColumn("float", "hours_error");

    for (const auto &wsName : workspaces) {
      const WorkspaceGroup_sptr group = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(wsName);
      const auto ws = std::dynamic_pointer_cast<MatrixWorkspace>(group->getItem(0));
      const auto timeStart = ws->mutableRun().getPropertyValueAsType<std::string>("start_time");
      const auto delay = static_cast<float>(DateAndTime::secondsFromDuration(DateAndTime(timeStart) - TIME_ORIGIN));

      TableRow newRow = outputTable->appendRow();
      // Set error to 2 seconds (this will be a duration of 1 second).
      constexpr float sError = 2.0;
      constexpr float hError = sError / 3600;
      newRow << wsName << timeStart << delay << sError << delay / 3600 << hError;
    }
    setProperty("OutputWorkspace", outputTable);
  }
};

DECLARE_ALGORITHM(TimeDifference)

} // namespace HePolDecayTest

using namespace HePolDecayTest;

class HeliumAnalyserEfficiencyTest : public CxxTest::TestSuite {
public:
  void setUp() override {
    m_inputParameters = InputTestParameters();
    m_polParameters = PolarizationTestParameters();
  }
  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testName() {
    HeliumAnalyserEfficiency alg;
    TS_ASSERT_EQUALS(alg.name(), "HeliumAnalyserEfficiency");
  }

  void testInit() {
    HeliumAnalyserEfficiency alg;
    alg.initialize();
    TS_ASSERT(alg.isInitialized());
  }

  void testInputWorkspaceNotAGroupThrows() {
    // Should accept a group workspace containing four workspaces, corresponding to the four spin configurations
    std::vector<double> x{1};
    std::vector<double> y{1};
    MatrixWorkspace_sptr test = generateWorkspace("test", x, y);
    const auto alg = prepareAlgorithm({"test"});
    TSM_ASSERT_THROWS("InputWorkspaces has to be a group", alg->execute(), const std::runtime_error &);
  }

  void testInputWorkspaceWithWrongSizedGroupThrows() {
    // The units of the input workspace should be wavelength
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    AnalysisDataService::Instance().remove("T00_0");
    const auto alg = prepareAlgorithm(groupNames);
    TS_ASSERT_THROWS_EQUALS(alg->execute(), const std::runtime_error &e, std::string(e.what()),
                            "Some invalid Properties found: \n InputWorkspaces: Error in workspace group_0 : The "
                            "number of periods within the input workspace is not an allowed value.");
  }

  void testInvalidSpinStateFormatThrowsError() {
    auto heliumAnalyserEfficiency = AlgorithmManager::Instance().create("HeliumAnalyserEfficiency");
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "bad"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "10,01"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "00,00,11,11"), std::invalid_argument &);
    TS_ASSERT_THROWS(heliumAnalyserEfficiency->setProperty("SpinStates", "02,20,22,00"), std::invalid_argument &);
  }

  void testInputWorkspaceDoesntFitTauIfOnlyOneInput() {
    // Fitting with one input workspace is equivalent to calculating helium efficiency at t=0
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    const auto alg = prepareAlgorithm(groupNames);

    alg->execute();
    TS_ASSERT(alg->isExecuted());
    const auto groupOut = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_NAME);
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), 1);
    TS_ASSERT_DELTA(m_polParameters.polInitial, m_polParameters.outPolarizations.at(0), 1e-4);
  }

  void testSmallNumberOfBins() {
    // With less than 3 bins it's not possible to perform the error calculation correctly, because the
    // number of parameters exceeds the number of data points.
    m_inputParameters.nBins = 2;
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    const auto alg = prepareAlgorithm(groupNames);

    alg->execute();
    TS_ASSERT(alg->isExecuted());
    const auto groupOut = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(OUTPUT_NAME);
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), 1);
  }

  void testChildAlgorithmExecutesSuccessfully() {
    const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization();
    const auto alg = prepareAlgorithm(groupNames);
    alg->setChild(true);

    alg->execute();

    TS_ASSERT(alg->isExecuted());
    const WorkspaceGroup_sptr groupOut = alg->getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(groupOut->getNumberOfEntries(), 1);
    const auto wsOut = std::dynamic_pointer_cast<MatrixWorkspace>(groupOut->getItem(0));
    TS_ASSERT_EQUALS(wsOut->getNumberHistograms(), 1);
  }

  void testAssertTimeDifferencesOrderDoesNotMatter() {
    const std::vector<std::vector<double>> delays = {{0, 10, 20}, {10, 20, 0}, {20, 0, 10}};
    for (const auto &delayVec : delays) {
      const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization(delayVec);
      const auto alg =
          prepareAlgorithm(groupNames, OUTPUT_NAME, TEST_SPIN_STATE, OUTPUT_TABLE_NAME, OUTPUT_CURVES_NAME);

      alg->execute();
      TS_ASSERT(alg->isExecuted());
      assertOutputNames(groupNames.size());
      assertOutputs(groupNames.size());
    }
  }

  void testFitWithDifferentMagnitudesOfTau() {
    const std::vector<double> delays = {0, 10, 20};
    const std::vector<double> lifetimes = {1, 100, 1000};
    for (const auto &tau : lifetimes) {
      m_polParameters.Tau = tau;
      const auto groupNames = generateEfficienciesFromLifetimeAndInitialPolarization(delays);
      const auto alg =
          prepareAlgorithm(groupNames, OUTPUT_NAME, TEST_SPIN_STATE, OUTPUT_TABLE_NAME, OUTPUT_CURVES_NAME);

      alg->execute();
      TS_ASSERT(alg->isExecuted());
      assertOutputNames(groupNames.size());
      assertOutputs(groupNames.size());
    }
  }

private:
  std::vector<std::string> generateEfficienciesFromLifetimeAndInitialPolarization(const std::vector<double> &delays = {
                                                                                      0}) {
    std::vector<double> polarizations;
    std::transform(delays.cbegin(), delays.cend(), std::back_inserter(polarizations), [&](const double delay) {
      return m_polParameters.polInitial * std::exp(-delay / m_polParameters.Tau);
    });

    const auto numBins = m_inputParameters.nBins;
    std::vector<double> x(numBins), tEfficiency(numBins), yNsf(numBins), ySf(numBins), otherEfficiency(numBins);
    std::vector<std::string> names(delays.size(), m_inputParameters.groupName);
    m_polParameters.outEfficiencies = std::vector<std::vector<double>>(delays.size());

    for (size_t groupIndex = 0; groupIndex < delays.size(); groupIndex++) {
      const auto phe = polarizations.at(groupIndex);
      // Wavelength fixed between 1.75 and 8 (algorithm default)
      for (auto wsIndex = 0; wsIndex < numBins; wsIndex++) {
        x[wsIndex] = 1.75 + static_cast<double>(wsIndex) * 8.0 / static_cast<double>(numBins);
        const auto mu = 0.0733 * m_polParameters.pxd * x[wsIndex];
        tEfficiency[wsIndex] = (1 + std::tanh(mu * phe)) / 2;
        yNsf[wsIndex] = 0.9 * std::exp(-mu * (1 - phe));
        ySf[wsIndex] = 0.9 * std::exp(-mu * (1 + phe));
      }

      std::vector<MatrixWorkspace_sptr> wsVec(4);
      const auto xUnit = m_inputParameters.xUnit;
      const auto nSpec = m_inputParameters.nSpec;
      const std::string groupIndexStr = "_" + std::to_string(groupIndex);
      wsVec[0] = generateWorkspace("T00" + groupIndexStr, x, yNsf, xUnit, nSpec, delays.at(groupIndex));
      wsVec[1] = generateWorkspace("T01" + groupIndexStr, x, ySf, xUnit, nSpec, delays.at(groupIndex));
      wsVec[2] = generateWorkspace("T10" + groupIndexStr, x, ySf, xUnit, nSpec, delays.at(groupIndex));
      wsVec[3] = generateWorkspace("T11" + groupIndexStr, x, yNsf, xUnit, nSpec, delays.at(groupIndex));
      names.at(groupIndex) += groupIndexStr;
      groupWorkspaces(names.at(groupIndex), wsVec);
      m_polParameters.outEfficiencies.at(groupIndex) = tEfficiency;
    }
    m_polParameters.outPolarizations = polarizations;
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
      TS_ASSERT_DELTA(y, m_polParameters.outEfficiencies.at(index), DELTA);
      TS_ASSERT_EQUALS(ws->getNumberBins(0), m_inputParameters.nBins);
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

  InputTestParameters m_inputParameters{};
  PolarizationTestParameters m_polParameters{};
};
