// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidMuon/EstimateMuonAsymmetryFromCounts.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::EstimateMuonAsymmetryFromCounts;

const std::string outputName = "EstimateMuonAsymmetryFromCounts_Output";

namespace {
struct yData {
  double operator()(const double x, size_t) {
    // Create a fake muon dataset
    double a = 0.1;                                             // Amplitude of the oscillations
    double w = 25.;                                             // Frequency of the oscillations
    double tau = Mantid::PhysicalConstants::MuonLifetime * 1e6; // Muon life time in microseconds
    double phi = 0.05;
    double e = exp(-x / tau);
    return (20. * (1.0 + a * cos(w * x + phi)) * e);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {
  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
      yData(), static_cast<int>(nspec), 0.0, 1.0, (1.0 / static_cast<double>(maxt)), true, eData());
  // Add  number of good frames
  ws->mutableRun().addProperty("goodfrm", 10);
  // AnalysisDataService::Instance().addOrReplace("ws",ws);
  return ws;
}

ITableWorkspace_sptr genTable() {
  Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable();
  table->addColumn("double", "norm");
  table->addColumn("str", "name");
  table->addColumn("str", "method");
  return table;
}

IAlgorithm_sptr setUpAlg(ITableWorkspace_sptr &table) {
  auto asymmAlg = AlgorithmManager::Instance().create("EstimateMuonAsymmetryFromCounts");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("NormalizationTable", table);
  asymmAlg->setProperty("WorkspaceName", "ws");
  asymmAlg->setProperty("StartX", 0.1);
  asymmAlg->setProperty("EndX", 0.9);
  return asymmAlg;
}
} // namespace

class EstimateMuonAsymmetryFromCountsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateMuonAsymmetryFromCountsTest *createSuite() { return new EstimateMuonAsymmetryFromCountsTest(); }
  static void destroySuite(EstimateMuonAsymmetryFromCountsTest *suite) { delete suite; }

  EstimateMuonAsymmetryFromCountsTest() { FrameworkManager::Instance(); }

  void testInit() {
    auto table = genTable();
    auto alg = setUpAlg(table);
    TS_ASSERT(alg->isInitialized())
  }

  void test_Execute() {

    auto ws = createWorkspace(1, 50);
    auto table = genTable();
    auto alg = setUpAlg(table);

    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
  }
  void test_EmptySpectrumList() {

    auto ws = createWorkspace(2, 50);
    auto table = genTable();
    auto alg = setUpAlg(table);

    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");

    double Delta = 0.0001;
    for (int j = 0; j < 2; j++) {
      // Test some X values
      TS_ASSERT_DELTA(outWS->x(j)[10], 0.2000, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[19], 0.3800, Delta);
      TS_ASSERT_DELTA(outWS->x(j)[49], 0.9800, Delta);
      // Test some Y values
      TS_ASSERT_DELTA(outWS->y(j)[10], 0.0176, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[19], -0.1128, Delta);
      TS_ASSERT_DELTA(outWS->y(j)[49], 0.0672, Delta);
      // Test some E values
      TS_ASSERT_DELTA(outWS->e(j)[10], 0.0002, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[19], 0.0003, Delta);
      TS_ASSERT_DELTA(outWS->e(j)[49], 0.0004, Delta);
    }
  }
  void test_SpectrumList() {

    std::vector<MatrixWorkspace_sptr> workspaces;
    workspaces.emplace_back(createWorkspace(2, 50));

    // First, run the algorithm without specifying any spectrum
    auto table = genTable();
    auto alg1 = setUpAlg(table);

    alg1->setProperty("InputWorkspace", workspaces[0]);
    alg1->setPropertyValue("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg1->execute());
    TS_ASSERT(alg1->isExecuted());

    workspaces.emplace_back(alg1->getProperty("OutputWorkspace"));

    // Then run the algorithm on the second spectrum only
    auto alg2 = setUpAlg(table);

    alg2->setProperty("InputWorkspace", workspaces[0]);
    alg2->setPropertyValue("OutputWorkspace", outputName);
    alg2->setPropertyValue("Spectra", "1");
    TS_ASSERT_THROWS_NOTHING(alg2->execute());
    TS_ASSERT(alg2->isExecuted());
    workspaces.emplace_back(alg2->getProperty("OutputWorkspace"));

    for (int j = 0; j < 3; j++) {
      if (j != 0) { // check we have 2 spectra
        TS_ASSERT_EQUALS(workspaces[j]->getNumberHistograms(), workspaces[0]->getNumberHistograms());
      }
      if (j != 2) { // check results match
        TS_ASSERT_EQUALS(workspaces[j]->x(j).rawData(), workspaces[2]->x(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->y(j).rawData(), workspaces[2]->y(j).rawData());
        TS_ASSERT_EQUALS(workspaces[j]->e(j).rawData(), workspaces[2]->e(j).rawData());
      }
    }
  }
  void test_yUnitLabel() {

    auto ws = createWorkspace(1, 50);

    auto table = genTable();
    auto alg = setUpAlg(table);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted())
    MatrixWorkspace_sptr result = alg->getProperty("OutputWorkspace");
    TS_ASSERT(result);
    TS_ASSERT_EQUALS(result->YUnitLabel(), "Asymmetry");
  }
  void test_NoRange() {
    auto ws = createWorkspace(1, 50);

    auto table = genTable();
    auto alg = setUpAlg(table);
    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("StartX", 0.1);
    alg->setProperty("EndX", 0.1);
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }
  void test_BackwardsRange() {
    auto ws = createWorkspace(1, 50);

    auto table = genTable();
    auto alg = setUpAlg(table);

    alg->setProperty("InputWorkspace", ws);
    alg->setProperty("StartX", 0.9);
    alg->setProperty("EndX", 0.1);
    alg->setProperty("OutputWorkspace", outputName);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }
  void test_NumberOfDataPoints() {

    double dx = (1.0 / 300.0);
    auto fineWS = WorkspaceCreationHelper::create2DWorkspaceFromFunction(yData(), 1, 0.0, 1.0, dx, true, eData());
    fineWS->mutableRun().addProperty("goodfrm", 10);
    auto coarseWS =
        WorkspaceCreationHelper::create2DWorkspaceFromFunction(yData(), 1, dx, 1.0 + dx, 3.0 * dx, true, eData());

    coarseWS->mutableRun().addProperty("goodfrm", 10);

    auto table = genTable();
    auto fineAlg = setUpAlg(table);

    fineAlg->setProperty("WorkspaceName", "fine");
    fineAlg->setProperty("InputWorkspace", fineWS);
    fineAlg->setPropertyValue("OutputWorkspace", "fineOutWS");
    TS_ASSERT_THROWS_NOTHING(fineAlg->execute());
    TS_ASSERT(fineAlg->isExecuted());
    MatrixWorkspace_sptr fineOutWS = fineAlg->getProperty("OutputWorkspace");

    auto coarseAlg = setUpAlg(table);
    coarseAlg->setProperty("InputWorkspace", coarseWS);
    coarseAlg->setProperty("WorkspaceName", "coarse");
    coarseAlg->setPropertyValue("OutputWorkspace", "coarseOutWS");
    TS_ASSERT_THROWS_NOTHING(coarseAlg->execute());
    TS_ASSERT(coarseAlg->isExecuted());
    MatrixWorkspace_sptr coarseOutWS = coarseAlg->getProperty("OutputWorkspace");

    // check names in table

    TS_ASSERT_EQUALS(table->String(0, 1), "fine");
    TS_ASSERT_EQUALS(table->String(1, 1), "coarse");

    double Delta = 0.05; // only expect numbers to be similar
    for (int j = 0; j < 28; j++) {
      // Test some X values
      TS_ASSERT_DELTA(fineOutWS->x(0)[1 + j * 3], coarseOutWS->x(0)[j], Delta);
      // Test some Y values
      TS_ASSERT_DELTA(fineOutWS->y(0)[1 + j * 3], coarseOutWS->y(0)[j], Delta);
      // Test some E values
      TS_ASSERT_DELTA(fineOutWS->e(0)[1 + j * 3], coarseOutWS->e(0)[j], Delta);
    }
  }
  void test_UserDefinedNorm() {

    auto ws = createWorkspace(1, 50);
    double userNorm = 10.2;

    auto table = genTable();
    auto alg = setUpAlg(table);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("NormalizationIn", userNorm);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputWorkspace");
    double normFromAlg = table->Double(0, 0);

    double Delta = 0.0001;
    TS_ASSERT_DELTA(normFromAlg, userNorm, Delta);
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 0.2000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 0.3800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 0.9800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], -0.7965, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], -0.8226, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], -0.7866, Delta);
  }
  void test_unNorm() {

    auto ws = createWorkspace(1, 50);

    auto table = genTable();
    auto alg = setUpAlg(table);
    alg->setProperty("InputWorkspace", ws);
    alg->setPropertyValue("OutputWorkspace", outputName);
    alg->setProperty("OutputUnNormData", true);
    alg->setProperty("OutputUnNormWorkspace", "out");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    MatrixWorkspace_sptr outWS = alg->getProperty("OutputUnNormWorkspace");

    double Delta = 0.0001;
    // Test some X values
    TS_ASSERT_DELTA(outWS->x(0)[10], 0.2000, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[19], 0.3800, Delta);
    TS_ASSERT_DELTA(outWS->x(0)[49], 0.9800, Delta);
    // Test some Y values
    TS_ASSERT_DELTA(outWS->y(0)[10], 2.0757, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[19], 1.8098, Delta);
    TS_ASSERT_DELTA(outWS->y(0)[49], 2.1769, Delta);
  }
};
// turn clang off, otherwise this does not compile
// clang-format off
class EstimateMuonAsymmetryFromCountsTestPerformance : public CxxTest::TestSuite {
  // clang-format on
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EstimateMuonAsymmetryFromCountsTestPerformance *createSuite() {
    return new EstimateMuonAsymmetryFromCountsTestPerformance();
  }
  // clang-format off
  static void  destroySuite(EstimateMuonAsymmetryFromCountsTestPerformance *suite) {
    // clang-format on
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  EstimateMuonAsymmetryFromCountsTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override { input = createWorkspace(1000, 100); }

  void testExec2D() {
    EstimateMuonAsymmetryFromCounts alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", input);
    alg.setPropertyValue("OutputWorkspace", "output");
    alg.setProperty("StartX", 0.1);
    alg.setProperty("EndX", 0.9);

    Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable();
    table->addColumn("double", "norm");
    table->addColumn("str", "name");
    table->addColumn("str", "method");
    alg.setProperty("NormalizationTable", table);

    alg.execute();
  }

private:
  MatrixWorkspace_sptr input;
};
