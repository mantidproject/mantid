// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <math.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/TableWorkspace.h"
#include <cxxtest/TestSuite.h>

#include <utility>

using namespace Mantid::DataObjects;
using namespace Mantid::API;

namespace {

const int dead1 = 4;
const int dead2 = 12;
constexpr double DELTA{0.0001};

void populatePhaseTableWithDeadDetectors(const ITableWorkspace_sptr &phaseTable, const MatrixWorkspace_sptr &ws) {
  phaseTable->addColumn("int", "DetectprID");
  phaseTable->addColumn("double", "Asymmetry");
  phaseTable->addColumn("double", "phase");
  double asym(1.);
  for (size_t i = 0; i < ws->getNumberHistograms(); i++) {
    TableRow phaseRow1 = phaseTable->appendRow();
    if (i == dead1 || i == dead2) {
      phaseRow1 << int(i) << 999. << 0.0;
    } else {
      phaseRow1 << int(i) << asym << 2. * M_PI * double(i + 1) / (1. + double(ws->getNumberHistograms()));
    }
  }
}
void populatePhaseTable(const ITableWorkspace_sptr &phaseTable, std::vector<std::string> names, bool swap = false) {
  phaseTable->addColumn("int", names[0]);
  phaseTable->addColumn("double", names[1]);
  phaseTable->addColumn("double", names[2]);
  double asym(1.), phase(2.);
  if (swap) {
    std::swap(asym, phase);
  }
  for (int i = 0; i < 16; i++) {
    TableRow phaseRow1 = phaseTable->appendRow();
    phaseRow1 << i << asym << phase;
    TableRow phaseRow2 = phaseTable->appendRow();
    phaseRow2 << i << asym << phase;
  }
}
void populatePhaseTable(const ITableWorkspace_sptr &phaseTable) {
  populatePhaseTable(std::move(phaseTable), {"DetectorID", "Asymmetry", "Phase"});
}

IAlgorithm_sptr setupAlg(const MatrixWorkspace_sptr &m_loadedData, bool isChildAlg,
                         const ITableWorkspace_sptr &phaseTable) {
  // Set up PhaseQuad
  auto phaseQuad = AlgorithmManager::Instance().create("PhaseQuad");
  phaseQuad->setChild(isChildAlg);
  phaseQuad->initialize();
  phaseQuad->setProperty("InputWorkspace", m_loadedData);
  phaseQuad->setProperty("PhaseTable", phaseTable);
  phaseQuad->setPropertyValue("OutputWorkspace", "outputWs");
  return phaseQuad;
}

IAlgorithm_sptr setupAlg(const MatrixWorkspace_sptr &m_loadedData, bool isChildAlg) {
  // Create and populate a detector table
  std::shared_ptr<ITableWorkspace> phaseTable(new Mantid::DataObjects::TableWorkspace);
  populatePhaseTable(phaseTable);

  return setupAlg(std::move(m_loadedData), isChildAlg, phaseTable);
}

IAlgorithm_sptr setupAlg(const MatrixWorkspace_sptr &m_loadedData, bool isChildAlg, std::vector<std::string> names,
                         bool swap = false) {
  // Create and populate a detector table
  std::shared_ptr<ITableWorkspace> phaseTable(new Mantid::DataObjects::TableWorkspace);
  populatePhaseTable(phaseTable, std::move(names), swap);

  return setupAlg(std::move(m_loadedData), isChildAlg, phaseTable);
}

IAlgorithm_sptr setupAlgDead(const MatrixWorkspace_sptr &m_loadedData) {
  // Create and populate a detector table
  std::shared_ptr<ITableWorkspace> phaseTable(new Mantid::DataObjects::TableWorkspace);
  populatePhaseTableWithDeadDetectors(phaseTable, m_loadedData);

  return setupAlg(m_loadedData, true, phaseTable);
}

MatrixWorkspace_sptr setupWS(const MatrixWorkspace_sptr &m_loadedData) {
  std::shared_ptr<ITableWorkspace> phaseTable(new Mantid::DataObjects::TableWorkspace);
  MatrixWorkspace_sptr ws = m_loadedData->clone();
  // create toy data set
  populatePhaseTableWithDeadDetectors(phaseTable, ws);
  auto xData = ws->points(0);
  for (size_t spec = 0; spec < ws->getNumberHistograms(); spec++) {
    for (size_t j = 0; j < xData.size(); j++) {
      if (spec == dead1 || spec == dead2) {
        ws->mutableY(spec)[j] = 0.0;
        ws->mutableE(spec)[j] = 0.0;
      } else {
        ws->mutableY(spec)[j] = sin(2.3 * xData[j] + phaseTable->Double(spec, 2)) * exp(-xData[j] / 2.19703);
        ws->mutableE(spec)[j] = cos(0.2 * xData[j]);
      }
    }
  }
  return ws;
}

MatrixWorkspace_sptr loadMuonDataset() {
  auto loader = AlgorithmManager::Instance().create("Load");
  loader->setChild(true);
  loader->initialize();
  loader->setProperty("Filename", "emu00006473.nxs");
  loader->setPropertyValue("OutputWorkspace", "outputWs");
  loader->execute();
  Workspace_sptr temp = loader->getProperty("OutputWorkspace");
  MatrixWorkspace_sptr m_loadedData = std::dynamic_pointer_cast<MatrixWorkspace>(temp);
  return m_loadedData;
}
} // namespace

class PhaseQuadMuonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PhaseQuadMuonTest *createSuite() { return new PhaseQuadMuonTest(); }

  static void destroySuite(PhaseQuadMuonTest *suite) { delete suite; }

  void setUp() override {
    if (!m_loadedData) {
      m_loadedData = loadMuonDataset();
    }
  }

  void testTheBasics() {
    IAlgorithm_sptr phaseQuad = AlgorithmManager::Instance().create("PhaseQuad");
    TS_ASSERT_EQUALS(phaseQuad->name(), "PhaseQuad");
    TS_ASSERT_EQUALS(phaseQuad->category(), "Muon");
    TS_ASSERT_THROWS_NOTHING(phaseQuad->initialize());
    TS_ASSERT(phaseQuad->isInitialized());
  }
  void testDead() {
    MatrixWorkspace_sptr ws = setupWS(m_loadedData);
    size_t nspec = ws->getNumberHistograms();
    // check got some dead detectors
    std::vector<bool> emptySpectrum;
    for (size_t h = 0; h < nspec; h++) {
      emptySpectrum.emplace_back(
          std::all_of(ws->y(h).begin(), ws->y(h).end(), [](double value) { return value == 0.; }));
    }
    for (size_t j = 0; j < emptySpectrum.size(); j++) {
      if (j == dead1 || j == dead2) {
        TS_ASSERT(emptySpectrum[j]);
      } else {
        TS_ASSERT(!emptySpectrum[j]);
      }
    }
    // do phase Quad
    auto phaseQuad = setupAlgDead(ws);
    TS_ASSERT_THROWS_NOTHING(phaseQuad->execute());
    TS_ASSERT(phaseQuad->isExecuted());

    // Get the output ws
    MatrixWorkspace_sptr outputWs = phaseQuad->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWs->getSpectrum(0).readX(),
                     m_loadedData->getSpectrum(0).readX()); // Check outputWs X values
    TS_ASSERT_EQUALS(outputWs->getSpectrum(1).readX(), m_loadedData->getSpectrum(1).readX());
    // Check output log is not empty
    TS_ASSERT(outputWs->mutableRun().getLogData().size() > 0);

    const auto specReY = outputWs->getSpectrum(0).y();
    const auto specReE = outputWs->getSpectrum(0).e();
    const auto specImY = outputWs->getSpectrum(1).y();
    const auto specImE = outputWs->getSpectrum(1).e();
    // Check real Y values
    TS_ASSERT_DELTA(specReY[0], -0.6149, DELTA);
    TS_ASSERT_DELTA(specReY[20], 0.2987, DELTA);
    TS_ASSERT_DELTA(specReY[50], 1.2487, DELTA);
    // Check real E values
    TS_ASSERT_DELTA(specReE[0], 0.2927, DELTA);
    TS_ASSERT_DELTA(specReE[20], 0.31489, DELTA);
    TS_ASSERT_DELTA(specReE[50], 0.3512, DELTA);
    // Check imaginary Y values
    TS_ASSERT_DELTA(specImY[0], 1.0823, DELTA);
    TS_ASSERT_DELTA(specImY[20], 1.3149, DELTA);
    TS_ASSERT_DELTA(specImY[50], 0.4965, DELTA);
    // Check imaginary E values
    TS_ASSERT_DELTA(specImE[0], 0.2801, DELTA);
    TS_ASSERT_DELTA(specImE[20], 0.3013, DELTA);
    TS_ASSERT_DELTA(specImE[50], 0.3360, DELTA);
  }
  void testExecPhaseTable() {
    auto phaseQuad = setupAlg(m_loadedData, true);
    TS_ASSERT_THROWS_NOTHING(phaseQuad->execute());
    TS_ASSERT(phaseQuad->isExecuted());

    // Get the output ws
    MatrixWorkspace_sptr outputWs = phaseQuad->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWs->getSpectrum(0).readX(),
                     m_loadedData->getSpectrum(0).readX()); // Check outputWs X values
    TS_ASSERT_EQUALS(outputWs->getSpectrum(1).readX(), m_loadedData->getSpectrum(1).readX());
    // Check output log is not empty
    TS_ASSERT(outputWs->mutableRun().getLogData().size() > 0);

    const auto specReY = outputWs->getSpectrum(0).y();
    const auto specReE = outputWs->getSpectrum(0).e();
    const auto specImY = outputWs->getSpectrum(1).y();
    const auto specImE = outputWs->getSpectrum(1).e();
    // Check real Y values
    TS_ASSERT_DELTA(specReY[0], 2.3212, DELTA);
    TS_ASSERT_DELTA(specReY[20], 0.0510, DELTA);
    TS_ASSERT_DELTA(specReY[50], -0.0578, DELTA);
    // Check real E values
    TS_ASSERT_DELTA(specReE[0], 0.0027, DELTA);
    TS_ASSERT_DELTA(specReE[20], 0.0043, DELTA);
    TS_ASSERT_DELTA(specReE[50], 0.0050, DELTA);
    // Check imaginary Y values
    TS_ASSERT_DELTA(specImY[0], 0.0328, DELTA);
    TS_ASSERT_DELTA(specImY[20], -0.0003, DELTA);
    TS_ASSERT_DELTA(specImY[50], -0.0033, DELTA);
    // Check imaginary E values
    TS_ASSERT_DELTA(specImE[0], 0.0003, DELTA);
    TS_ASSERT_DELTA(specImE[20], 0.0004, DELTA);
    TS_ASSERT_DELTA(specImE[50], 0.0005, DELTA);
  }
  void testNoPhase() {
    std::vector<std::string> names = {"ID", "Asym", "dummy"};
    IAlgorithm_sptr phaseQuad = setupAlg(m_loadedData, true, names);
    TS_ASSERT_THROWS(phaseQuad->execute(), const std::runtime_error &);
  }
  void testNoAsymm() {
    std::vector<std::string> names = {"ID", "AsYMg", "phase"};
    MatrixWorkspace_sptr m_loadedData = loadMuonDataset();
    IAlgorithm_sptr phaseQuad = setupAlg(m_loadedData, true, names);
    TS_ASSERT_THROWS(phaseQuad->execute(), const std::runtime_error &);
  }
  void testTwoPhases() {
    std::vector<std::string> names = {"ID", "Phase", "phi"};
    IAlgorithm_sptr phaseQuad = setupAlg(m_loadedData, true, names);
    TS_ASSERT_THROWS(phaseQuad->execute(), const std::runtime_error &);
  }
  void testTwoAsymm() {
    std::vector<std::string> names = {"ID", "Asym", "Asymm"};
    IAlgorithm_sptr phaseQuad = setupAlg(m_loadedData, true, names);
    TS_ASSERT_THROWS(phaseQuad->execute(), const std::runtime_error &);
  }
  void testSwapOrder() {
    std::vector<std::string> names = {"ID", "phase", "Asymm"};
    auto phaseQuad = setupAlg(m_loadedData, true, names, true);
    TS_ASSERT_THROWS_NOTHING(phaseQuad->execute());
    TS_ASSERT(phaseQuad->isExecuted());

    // Get the output ws
    MatrixWorkspace_sptr outputWs = phaseQuad->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(outputWs->getNumberHistograms(), 2);
    TS_ASSERT_EQUALS(outputWs->getSpectrum(0).readX(),
                     m_loadedData->getSpectrum(0).readX()); // Check outputWs X values
    TS_ASSERT_EQUALS(outputWs->getSpectrum(1).readX(), m_loadedData->getSpectrum(1).readX());

    const auto specReY = outputWs->getSpectrum(0).y();
    const auto specReE = outputWs->getSpectrum(0).e();
    const auto specImY = outputWs->getSpectrum(1).y();
    const auto specImE = outputWs->getSpectrum(1).e();
    // Check real Y values
    TS_ASSERT_DELTA(specReY[0], 2.3212, DELTA);
    TS_ASSERT_DELTA(specReY[20], 0.0510, DELTA);
    TS_ASSERT_DELTA(specReY[50], -0.0578, DELTA);
    // Check real E values
    TS_ASSERT_DELTA(specReE[0], 0.0027, DELTA);
    TS_ASSERT_DELTA(specReE[20], 0.0043, DELTA);
    TS_ASSERT_DELTA(specReE[50], 0.0050, DELTA);
    // Check imaginary Y values
    TS_ASSERT_DELTA(specImY[0], 0.0328, DELTA);
    TS_ASSERT_DELTA(specImY[20], -0.0003, DELTA);
    TS_ASSERT_DELTA(specImY[50], -0.0033, DELTA);
    // Check imaginary E values
    TS_ASSERT_DELTA(specImE[0], 0.0003, DELTA);
    TS_ASSERT_DELTA(specImE[20], 0.0004, DELTA);
    TS_ASSERT_DELTA(specImE[50], 0.0005, DELTA);
  }
  // add test for different order

private:
  MatrixWorkspace_sptr m_loadedData;
};

class PhaseQuadMuonTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PhaseQuadMuonTestPerformance *createSuite() { return new PhaseQuadMuonTestPerformance(); }

  static void destroySuite(PhaseQuadMuonTestPerformance *suite) { delete suite; }

  void setUp() override {
    m_loadedData = loadMuonDataset();
    phaseQuad = setupAlg(m_loadedData, false);
  }

  void tearDown() override { Mantid::API::AnalysisDataService::Instance().remove("outputWs"); }

  void testPerformanceWs() { phaseQuad->execute(); }

private:
  MatrixWorkspace_sptr m_loadedData;
  IAlgorithm_sptr phaseQuad;
};
