// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/VectorHelper.h"
#include "MantidMuon/CalculateMuonAsymmetry.h"
#include <cxxtest/TestSuite.h>

#include <utility>

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

using namespace Mantid::API;
using Mantid::MantidVec;
using Mantid::Algorithms::CalculateMuonAsymmetry;

const std::string outputName = "CalculateMuonAsymmetry_Output";

namespace {

struct yData {
  double operator()(const double x, size_t) {

    // Create a fake muon dataset
    double a = 0.20; // Amplitude of the oscillations
    double w = 5.0;  // Frequency of the oscillations
    double phi = 0.1;
    return (3.4 * (1.0 + a * sin(w * x + phi)));
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

MatrixWorkspace_sptr createWorkspace(size_t nspec, size_t maxt) {
  MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspaceFromFunction(
      yData(), static_cast<int>(nspec), 0.0, 10.0, 10.0 * (1.0 / static_cast<double>(maxt)), true, eData());
  ws->setYUnit("Asymmetry");
  return ws;
}

void genData() {
  auto ws1 = createWorkspace(1, 200);
  AnalysisDataService::Instance().addOrReplace("ws1", ws1);

  auto ws2 = createWorkspace(1, 200);
  AnalysisDataService::Instance().addOrReplace("ws2", ws2);

  auto ws3 = createWorkspace(1, 200);
  AnalysisDataService::Instance().addOrReplace("ws3", ws3);

  auto ws4 = createWorkspace(1, 200);
  AnalysisDataService::Instance().addOrReplace("ws4", ws4);
}

ITableWorkspace_sptr genTable() {
  Mantid::API::ITableWorkspace_sptr table = Mantid::API::WorkspaceFactory::Instance().createTable();
  table->addColumn("double", "norm");
  table->addColumn("str", "name");
  table->addColumn("str", "method");

  // populate table
  TableRow row = table->appendRow();
  row << 2.2 << "ws1"
      << "Estimate";
  row = table->appendRow();
  row << 2.2 << "ws2"
      << "Estimate";
  row = table->appendRow();
  row << 2.2 << "ws3"
      << "Estimate";
  row = table->appendRow();
  row << 2.2 << "ws4"
      << "Estimate";
  return table;
}

IAlgorithm_sptr setUpFuncAlg(const std::vector<std::string> &wsNames, const IFunction_sptr &func) {
  auto asymmAlg = AlgorithmManager::Instance().create("ConvertFitFunctionForMuonTFAsymmetry");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("WorkspaceList", wsNames);
  ITableWorkspace_sptr table = genTable();
  asymmAlg->setProperty("NormalizationTable", table);
  asymmAlg->setProperty("InputFunction", func);
  return asymmAlg;
}

IFunction_sptr genSingleFunc(const std::vector<std::string> &wsNames) {
  IFunction_sptr func = FunctionFactory::Instance().createInitialized("name=GausOsc,Frequency=3.0");
  auto alg = setUpFuncAlg(std::move(wsNames), func);
  alg->execute();
  IFunction_sptr funcOut = alg->getProperty("OutputFunction");
  return funcOut;
}

IFunction_sptr genDoubleFunc(const std::vector<std::string> &wsNames) {
  std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
  multiFuncString += "name=GausOsc,$domains=i,Frequency=3.0;";
  multiFuncString += "name=GausOsc,$domains=i,Frequency=3.0;";
  IFunction_sptr func = FunctionFactory::Instance().createInitialized(multiFuncString);
  auto alg = setUpFuncAlg(std::move(wsNames), func);
  alg->execute();
  IFunction_sptr funcOut = alg->getProperty("OutputFunction");
  std::cout << funcOut << std::endl;
  return funcOut;
}

IAlgorithm_sptr setUpAlg(ITableWorkspace_sptr &table, const IFunction_sptr &func,
                         const std::vector<std::string> &wsNamesNorm, const std::vector<std::string> &wsOut) {
  auto asymmAlg = AlgorithmManager::Instance().create("CalculateMuonAsymmetry");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("NormalizationTable", table);
  asymmAlg->setProperty("StartX", 0.1);
  asymmAlg->setProperty("EndX", 0.9);
  asymmAlg->setProperty("InputFunction", func);
  asymmAlg->setProperty("UnNormalizedWorkspaceList", wsNamesNorm);
  asymmAlg->setProperty("ReNormalizedWorkspaceList", wsOut);

  return asymmAlg;
}

void clearADS() { AnalysisDataService::Instance().clear(); }

class CalculateMuonAsymmetryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CalculateMuonAsymmetryTest *createSuite() { return new CalculateMuonAsymmetryTest(); }
  static void destroySuite(CalculateMuonAsymmetryTest *suite) { delete suite; }

  CalculateMuonAsymmetryTest() { FrameworkManager::Instance(); }

  void test_Execute() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    IAlgorithm_sptr alg = setUpAlg(table, func, wsNames, wsOut);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    clearADS();
  }

  void test_singleFit() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    auto alg = setUpAlg(table, func, wsNames, wsOut);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    std::vector<std::string> output = alg->getProperty("ReNormalizedWorkspaceList");

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output[0]);

    double delta = 0.0001;

    TS_ASSERT_DELTA(table->Double(1, 0), 3.4, delta);
    TS_ASSERT_EQUALS(table->String(1, 1), "ws2");
    TS_ASSERT_EQUALS(table->String(1, 2), "Calculated");

    TS_ASSERT_DELTA(outWS->x(0)[10], 0.5, delta);
    TS_ASSERT_DELTA(outWS->x(0)[40], 2.0, delta);
    TS_ASSERT_DELTA(outWS->x(0)[100], 5.0, delta);

    TS_ASSERT_DELTA(outWS->y(0)[10], 0.1031, delta);
    TS_ASSERT_DELTA(outWS->y(0)[40], -0.1250, delta);
    TS_ASSERT_DELTA(outWS->y(0)[100], -0.0065, delta);

    TS_ASSERT_DELTA(outWS->e(0)[10], 0.0015, delta);
    TS_ASSERT_DELTA(outWS->e(0)[40], 0.0015, delta);
    TS_ASSERT_DELTA(outWS->e(0)[100], 0.0015, delta);

    clearADS();
  }
  void test_badFittingRange() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    auto alg = setUpAlg(table, func, wsNames, wsOut);
    alg->setProperty("StartX", 10.);
    alg->setProperty("EndX", 1.);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    clearADS();
  }

  void test_mismatchWSLists() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2", "ws3"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    IAlgorithm_sptr alg = setUpAlg(table, func, wsNames, wsOut);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    clearADS();
  }

  void test_multiFuncSingleWS() {

    genData();
    // need the 2 here to get multi func
    std::vector<std::string> wsNames = {"ws1", "ws3"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genDoubleFunc(wsNames);
    auto table = genTable();
    wsNames = {"ws1"};

    IAlgorithm_sptr alg = setUpAlg(table, func, wsNames, wsOut);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    clearADS();
  }

  void test_yUnitLabel() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    auto alg = setUpAlg(table, func, wsNames, wsOut);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    std::vector<std::string> output = alg->getProperty("ReNormalizedWorkspaceList");

    MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output[0]);

    TS_ASSERT_EQUALS(outWS->YUnitLabel(), "Asymmetry");
  }

  void test_multiFuncWS() {

    genData();
    // need the 2 here to get multi func
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::vector<std::string> wsOut = {"ws3", "ws4"};
    auto func = genDoubleFunc(wsNames);
    auto table = genTable();

    auto alg = setUpAlg(table, func, wsNames, wsOut);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    std::vector<std::string> output = alg->getProperty("ReNormalizedWorkspaceList");

    for (int j = 0; j < 2; j++) {
      MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output[j]);

      double delta = 0.0001;

      TS_ASSERT_DELTA(table->Double(j + 2, 0), 3.4, delta);
      TS_ASSERT_EQUALS(table->String(j + 2, 1), output[j]);
      TS_ASSERT_EQUALS(table->String(j + 2, 2), "Calculated");

      TS_ASSERT_DELTA(outWS->x(0)[10], 0.5, delta);
      TS_ASSERT_DELTA(outWS->x(0)[40], 2.0, delta);
      TS_ASSERT_DELTA(outWS->x(0)[100], 5.0, delta);

      TS_ASSERT_DELTA(outWS->y(0)[10], 0.1031, delta);
      TS_ASSERT_DELTA(outWS->y(0)[40], -0.1250, delta);
      TS_ASSERT_DELTA(outWS->y(0)[100], -0.0065, delta);

      TS_ASSERT_DELTA(outWS->e(0)[10], 0.0015, delta);
      TS_ASSERT_DELTA(outWS->e(0)[40], 0.0015, delta);
      TS_ASSERT_DELTA(outWS->e(0)[100], 0.0015, delta);
    }

    clearADS();
  }

  void test_simultaneous_fit_with_double_pulse_mode_enabled() {
    genData();
    // need the 2 here to get multi func
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::vector<std::string> wsOut = {"ws3", "ws4"};
    auto func = genDoubleFunc(wsNames);
    auto table = genTable();

    auto alg = setUpAlg(table, func, wsNames, wsOut);
    alg->setProperty("EnableDoublePulse", true);
    alg->setProperty("PulseOffset", 0.33);
    alg->setProperty("FirstPulseWeight", 0.5);

    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    std::vector<std::string> output = alg->getProperty("ReNormalizedWorkspaceList");

    for (int j = 0; j < 2; j++) {
      MatrixWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(output[j]);

      double delta = 0.0001;

      TS_ASSERT_DELTA(table->Double(j + 2, 0), 3.4, delta);
      TS_ASSERT_EQUALS(table->String(j + 2, 1), output[j]);
      TS_ASSERT_EQUALS(table->String(j + 2, 2), "Calculated");

      TS_ASSERT_DELTA(outWS->x(0)[10], 0.5, delta);
      TS_ASSERT_DELTA(outWS->x(0)[40], 2.0, delta);
      TS_ASSERT_DELTA(outWS->x(0)[100], 5.0, delta);

      TS_ASSERT_DELTA(outWS->y(0)[10], 0.1031, delta);
      TS_ASSERT_DELTA(outWS->y(0)[40], -0.1250, delta);
      TS_ASSERT_DELTA(outWS->y(0)[100], -0.0065, delta);

      TS_ASSERT_DELTA(outWS->e(0)[10], 0.0015, delta);
      TS_ASSERT_DELTA(outWS->e(0)[40], 0.0015, delta);
      TS_ASSERT_DELTA(outWS->e(0)[100], 0.0015, delta);
    }
    clearADS();
  }
};

class CalculateMuonAsymmetryTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created
  // statically This means the constructor isn't called when running other
  // tests
  static CalculateMuonAsymmetryTestPerformance *createSuite() { return new CalculateMuonAsymmetryTestPerformance(); }
  static void destroySuite(CalculateMuonAsymmetryTestPerformance *suite) {
    AnalysisDataService::Instance().clear();
    delete suite;
  }

  CalculateMuonAsymmetryTestPerformance() { FrameworkManager::Instance(); }

  void setUp() override { input = createWorkspace(1000, 100); }

  void testExec1D() {

    genData();
    std::vector<std::string> wsNames = {"ws1"};
    std::vector<std::string> wsOut = {"ws2"};
    auto func = genSingleFunc(wsNames);
    auto table = genTable();

    IAlgorithm_sptr alg = setUpAlg(table, func, wsNames, wsOut);

    alg->execute();
    clearADS();
  }

  void testExec2D() {

    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::vector<std::string> wsOut = {"ws3", "ws4"};
    auto func = genDoubleFunc(wsNames);
    auto table = genTable();

    IAlgorithm_sptr alg = setUpAlg(table, func, wsNames, wsOut);

    alg->execute();
    clearADS();
  }

private:
  MatrixWorkspace_sptr input;
};
} // namespace
