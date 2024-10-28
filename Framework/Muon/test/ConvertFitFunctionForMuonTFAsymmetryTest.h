// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidMuon/ConvertFitFunctionForMuonTFAsymmetry.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::Muon::ConvertFitFunctionForMuonTFAsymmetry;

const std::string outputName = "EstimateMuonAsymmetryFromCounts_Output";

namespace {

const std::string NORM_PARAM{"f0.f0.A0"};
const std::string OFFSET_PARAM{"f0.f1.f0.A0"};
const std::string USER_FUNC{"f0.f1.f1."};
const std::string EXP_PARAM{"f1.A"};

struct yData {
  double operator()(const double x, size_t) {
    // Create a fake muon dataset
    return (x);
  }
};

struct eData {
  double operator()(const double, size_t) { return 0.005; }
};

MatrixWorkspace_sptr createWorkspace() {
  MatrixWorkspace_sptr ws =
      WorkspaceCreationHelper::create2DWorkspaceFromFunction(yData(), 1, 0.0, 1.0, (1.0 / 50.), true, eData());
  return ws;
}

ITableWorkspace_sptr genTable() {
  ITableWorkspace_sptr table = WorkspaceFactory::Instance().createTable();
  table->addColumn("double", "norm");
  table->addColumn("str", "name");
  table->addColumn("str", "method");
  std::vector<std::string> names = {"ws1", "ws2", "ws3"};
  for (size_t j = 0; j < names.size(); j++) {
    TableRow row = table->appendRow();
    row << double(j + 1) << names[j] << "test";
  }
  return table;
}

IAlgorithm_sptr setUpAlg(const std::vector<std::string> &wsNames, const IFunction_sptr &func, bool copyTies = true) {
  auto asymmAlg = AlgorithmManager::Instance().create("ConvertFitFunctionForMuonTFAsymmetry");
  asymmAlg->initialize();
  asymmAlg->setChild(true);
  asymmAlg->setProperty("WorkspaceList", wsNames);
  ITableWorkspace_sptr table = genTable();
  asymmAlg->setProperty("NormalizationTable", table);
  asymmAlg->setProperty("InputFunction", func);
  asymmAlg->setProperty("CopyTies", copyTies);
  return asymmAlg;
}

void genData() {
  auto ws1 = createWorkspace();
  AnalysisDataService::Instance().addOrReplace("ws1", ws1);
  AnalysisDataService::Instance().addOrReplace("ws2", ws1);
  AnalysisDataService::Instance().addOrReplace("ws3", ws1);
}

IFunction_sptr doFit(const IFunction_sptr &func, int iterations, std::vector<std::string> wsNames) {

  auto fit = AlgorithmManager::Instance().create("Fit");
  fit->initialize();
  fit->setProperty("Function", func);
  fit->setProperty("InputWorkspace", wsNames[0]);
  if (wsNames.size() > 1) {

    fit->setProperty("InputWorkspace_1", wsNames[1]);
  }
  fit->setProperty("Output", "fit");
  fit->setProperty("MaxIterations", iterations);
  fit->execute();
  std::string funcString = fit->getPropertyValue("Function");
  return FunctionFactory::Instance().createInitialized(funcString);
}
} // namespace

class ConvertFitFunctionForMuonTFAsymmetryTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertFitFunctionForMuonTFAsymmetryTest *createSuite() {
    return new ConvertFitFunctionForMuonTFAsymmetryTest();
  }
  static void destroySuite(ConvertFitFunctionForMuonTFAsymmetryTest *suite) { delete suite; }

  ConvertFitFunctionForMuonTFAsymmetryTest() { FrameworkManager::Instance(); }

  void testInit() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func =
        FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;ties =(f0.A1=2)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
  }

  void test_Execute() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func =
        FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;ties =(f0.A1=2)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_1D() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    auto outFunc = doFit(normFunc, 0, wsNames);
    TS_ASSERT_DELTA(outFunc->getParameter(OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter(EXP_PARAM), 0.0, 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter(USER_FUNC + "A1"), 2.0, 0.0001);
  }

  void test_1DFix() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func =
        FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;ties =(f0.A1=2)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    auto outFunc = doFit(normFunc, 200, wsNames);
    TS_ASSERT_DELTA(outFunc->getParameter(OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter(EXP_PARAM), 0.0, 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter(USER_FUNC + "A1"), 2.0, 0.0001);
    TS_ASSERT_DIFFERS(outFunc->getParameter(USER_FUNC + "A0"), 0.0);
  }
  void test_1DTie() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground,A0=0,A1=2;name=LinearBackground,A0=0,A1=4;ties "
        "=(f0.A1=f1.A1)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    auto outFunc = doFit(normFunc, 200, wsNames);
    TS_ASSERT_DELTA(outFunc->getParameter(OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter(EXP_PARAM), 0.0, 0.0001);
    TS_ASSERT_EQUALS(outFunc->getParameter(USER_FUNC + "f0.A1"), outFunc->getParameter(USER_FUNC + "f1.A1"));
    TS_ASSERT_DIFFERS(outFunc->getParameter(USER_FUNC + "f0.A0"), func->getParameter("f0.A0"));
    TS_ASSERT_DIFFERS(outFunc->getParameter(USER_FUNC + "f1.A0"), func->getParameter("f1.A0"));
  }
  void test_1DTieWithoutCopyTies() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground,A0=0,A1=2;name=LinearBackground,A0=0,A1=4;ties "
        "=(f0.A1=f1.A1)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");
    TS_ASSERT_EQUALS(normFunc->getTie(0), nullptr);
  }
  // tests for multi domain
  void test_2D() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);

    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 0, wsNames);

    TS_ASSERT_DELTA(normFunc->getParameter("f0." + NORM_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DELTA(normFunc->getParameter("f0." + USER_FUNC + "A0"), 0.0, 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + USER_FUNC + "A1"), 1.0, 0.0001);

    TS_ASSERT_DELTA(normFunc->getParameter("f1." + NORM_PARAM), 2., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DELTA(normFunc->getParameter("f1." + USER_FUNC + "A0"), 2.0, 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + USER_FUNC + "A1"), 3.0, 0.0001);
  }
  void test_2DFix() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1.5;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);
    multiFunc->addTies("f0.A1=1.5");
    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 200, wsNames);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f0." + NORM_PARAM), 1.);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f0." + USER_FUNC + "A0"), 0.0);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + USER_FUNC + "A1"), 1.5, 0.0001);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f1." + NORM_PARAM), 2.);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f1." + USER_FUNC + "A0"), 2.0);
    TS_ASSERT_DIFFERS(normFunc->getParameter("f1." + USER_FUNC + "A1"), 3.0);
  }

  void test_2DTie() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1.5;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);
    multiFunc->addTies("f0.A1=f1.A1");
    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 200, wsNames);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f0." + NORM_PARAM), 1.);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f0." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f0." + USER_FUNC + "A0"), 0.0);
    TS_ASSERT_EQUALS(normFunc->getParameter("f0." + USER_FUNC + "A1"),
                     normFunc->getParameter("f1." + USER_FUNC + "A1"));

    TS_ASSERT_DIFFERS(normFunc->getParameter("f1." + NORM_PARAM), 2.);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + OFFSET_PARAM), 1., 0.0001);
    TS_ASSERT_DELTA(normFunc->getParameter("f1." + EXP_PARAM), 0.0, 0.0001);

    TS_ASSERT_DIFFERS(normFunc->getParameter("f1." + USER_FUNC + "A0"), 2.0);
  }

  // test extract
  void test_1DExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    alg = setUpAlg(wsNames, normFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr outFunc = alg->getProperty("OutputFunction");

    // auto outFunc = doFit(normFunc,0,wsNames);
    TS_ASSERT_DELTA(outFunc->getParameter("A0"), 0.0, 0.0001);
    TS_ASSERT_DELTA(outFunc->getParameter("A1"), 2.0, 0.0001);
  }

  void test_1DFixExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func =
        FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=2;ties =(f0.A1=2)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    auto outFunc = doFit(normFunc, 200, wsNames);

    alg = setUpAlg(wsNames, outFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr extractFunc = alg->getProperty("OutputFunction");

    TS_ASSERT_DELTA(extractFunc->getParameter("A1"), 2.0, 0.0001);
    TS_ASSERT_EQUALS(outFunc->getParameter(USER_FUNC + "A0"), extractFunc->getParameter("A0"));
  }
  void test_1DTieExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1"};
    IFunction_sptr func = FunctionFactory::Instance().createInitialized(
        "name=LinearBackground,A0=0,A1=2;name=LinearBackground,A0=0,A1=4;ties "
        "=(f0.A1=f1.A1)");
    auto alg = setUpAlg(wsNames, func);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = alg->getProperty("OutputFunction");

    auto outFunc = doFit(normFunc, 200, wsNames);

    alg = setUpAlg(wsNames, outFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr extractFunc = alg->getProperty("OutputFunction");

    TS_ASSERT_EQUALS(extractFunc->getParameter("f0.A1"), extractFunc->getParameter("f1.A1"));
    TS_ASSERT_DIFFERS(extractFunc->getParameter("f0.A0"), func->getParameter("f0.A0"));
    TS_ASSERT_DIFFERS(extractFunc->getParameter("f1.A0"), func->getParameter("f1.A0"));
  }

  // tests for multi domain
  void test_2DExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);

    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 0, wsNames);

    alg = setUpAlg(wsNames, normFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr extractFunc = alg->getProperty("OutputFunction");

    TS_ASSERT_DELTA(extractFunc->getParameter("f0.A0"), 0.0, 0.0001);
    TS_ASSERT_DELTA(extractFunc->getParameter("f0.A1"), 1.0, 0.0001);

    TS_ASSERT_DELTA(extractFunc->getParameter("f1.A0"), 2.0, 0.0001);
    TS_ASSERT_DELTA(extractFunc->getParameter("f1.A1"), 3.0, 0.0001);
  }
  void test_2DFixExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1.5;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);
    multiFunc->addTies("f0.A1=1.5");
    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 200, wsNames);

    alg = setUpAlg(wsNames, normFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr extractFunc = alg->getProperty("OutputFunction");

    TS_ASSERT_DIFFERS(extractFunc->getParameter("f0.A0"), 0.0);
    TS_ASSERT_DELTA(extractFunc->getParameter("f0.A1"), 1.5, 0.0001);

    TS_ASSERT_DIFFERS(extractFunc->getParameter("f1.A0"), 2.0);
    TS_ASSERT_DIFFERS(extractFunc->getParameter("f1.A1"), 3.0);
  }

  void test_2DTieExtract() {
    genData();
    std::vector<std::string> wsNames = {"ws1", "ws2"};
    std::string multiFuncString = "composite=MultiDomainFunction,NumDeriv=1;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=0,A1=1.5;";
    multiFuncString += "name=LinearBackground, $domains=i,A0=2,A1=3;";

    IFunction_sptr multiFunc = FunctionFactory::Instance().createInitialized(multiFuncString);
    multiFunc->addTies("f0.A1=f1.A1");
    auto alg = setUpAlg(wsNames, multiFunc);
    TS_ASSERT(alg->isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr normFunc = doFit(alg->getProperty("OutputFunction"), 200, wsNames);

    alg = setUpAlg(wsNames, normFunc);
    alg->setProperty("Mode", "Extract");
    TS_ASSERT(alg->isInitialized());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    IFunction_sptr extractFunc = alg->getProperty("OutputFunction");

    TS_ASSERT_DIFFERS(extractFunc->getParameter("f0.A0"), 0.0);
    TS_ASSERT_EQUALS(extractFunc->getParameter("f0.A1"), extractFunc->getParameter("f1.A1"));

    TS_ASSERT_DIFFERS(extractFunc->getParameter("f1.A0"), 2.0);
  }

private:
  MatrixWorkspace_sptr input;
};
