#ifndef MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FuncMinimizers/FABADAMinimizer.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"

#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/FakeObjects.h"
#include "MantidKernel/Exception.h"

using Mantid::CurveFitting::FuncMinimisers::FABADAMinimizer;
using namespace Mantid::API;
using namespace Mantid;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

namespace {

API::MatrixWorkspace_sptr createTestWorkspace(size_t NVectors = 2,
                                              size_t XYLength = 20) {
  MatrixWorkspace_sptr ws2(new WorkspaceTester);
  ws2->initialize(NVectors, XYLength, XYLength);

  for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {

    auto &x = ws2->mutableX(is);
    auto &y = ws2->mutableY(is);
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = (10.0 + double(is)) * exp(-(x[i]) / (0.5 * (1 + double(is))));
    }
  }
  return ws2;
}

void doTestExpDecay(API::MatrixWorkspace_sptr ws2) {

  API::IFunction_sptr fun(new ExpDecay);
  fun->setParameter("Height", 8.);
  fun->setParameter("Lifetime", 1.0);

  Algorithms::Fit fit;
  fit.initialize();

  fit.setRethrows(true);
  fit.setProperty("Function", fun);
  fit.setProperty("InputWorkspace", ws2);
  fit.setProperty("WorkspaceIndex", 0);
  fit.setProperty("CreateOutput", true);
  fit.setProperty("MaxIterations", 100000);
  fit.setProperty("Minimizer", "FABADA,ChainLength=5000,StepsBetweenValues="
                               "10,ConvergenceCriteria=0.1,CostFunctionTable="
                               "CostFunction,Chains=Chain,ConvergedChain"
                               "=ConvergedChain,Parameters=Parameters");

  TS_ASSERT_THROWS_NOTHING(fit.execute());

  TS_ASSERT(fit.isExecuted());

  TS_ASSERT_DELTA(fun->getParameter("Height"), 10.0, 0.7);
  TS_ASSERT_DELTA(fun->getParameter("Lifetime"), 0.5, 0.1);
  TS_ASSERT_DELTA(fun->getError(0), 0.7, 1e-1);
  TS_ASSERT_DELTA(fun->getError(1), 0.06, 1e-2);

  TS_ASSERT_EQUALS(fit.getPropertyValue("OutputStatus"), "success");

  size_t n = fun->nParams();

  TS_ASSERT(AnalysisDataService::Instance().doesExist("PDF"));
  MatrixWorkspace_sptr wsPDF = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("PDF"));
  TS_ASSERT(wsPDF);
  TS_ASSERT_EQUALS(wsPDF->getNumberHistograms(), n + 1);

  const auto &X = wsPDF->mutableX(0);
  const auto &Y = wsPDF->mutableY(0);
  TS_ASSERT_EQUALS(X.size(), 21);
  TS_ASSERT_EQUALS(Y.size(), 20);

  TS_ASSERT(AnalysisDataService::Instance().doesExist("CostFunction"));
  ITableWorkspace_sptr CostFunctionTable =
      boost::dynamic_pointer_cast<ITableWorkspace>(
          API::AnalysisDataService::Instance().retrieve("CostFunction"));

  TS_ASSERT(CostFunctionTable);
  TS_ASSERT_EQUALS(CostFunctionTable->columnCount(), 4);
  TS_ASSERT_EQUALS(CostFunctionTable->rowCount(), 1);
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(0)->type(), "double");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(0)->name(), "Chi2min");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(1)->type(), "double");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(1)->name(), "Chi2MP");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(2)->type(), "double");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(2)->name(), "Chi2min_red");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(3)->type(), "double");
  TS_ASSERT_EQUALS(CostFunctionTable->getColumn(3)->name(), "Chi2MP_red");
  TS_ASSERT(CostFunctionTable->Double(0, 0) <= CostFunctionTable->Double(0, 1));
  TS_ASSERT(CostFunctionTable->Double(0, 2) <= CostFunctionTable->Double(0, 3));
  // TS_ASSERT_DELTA(CostFunctionTable->Double(0, 0),
  //                CostFunctionTable->Double(0, 1), 1.5);
  TS_ASSERT_DELTA(CostFunctionTable->Double(0, 0), 0.0, 1.0);

  TS_ASSERT(AnalysisDataService::Instance().doesExist("ConvergedChain"));
  MatrixWorkspace_sptr wsConv = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("ConvergedChain"));
  TS_ASSERT(wsConv);
  TS_ASSERT_EQUALS(wsConv->getNumberHistograms(), n + 1);

  const auto &Xconv = wsConv->x(0);
  TS_ASSERT_EQUALS(Xconv.size(), 500);
  TS_ASSERT_EQUALS(Xconv[437], 437);

  TS_ASSERT(AnalysisDataService::Instance().doesExist("Chain"));
  MatrixWorkspace_sptr wsChain = boost::dynamic_pointer_cast<MatrixWorkspace>(
      API::AnalysisDataService::Instance().retrieve("Chain"));
  TS_ASSERT(wsChain);
  TS_ASSERT_EQUALS(wsChain->getNumberHistograms(), n + 1);

  const auto &Xchain = wsChain->x(0);
  TS_ASSERT_EQUALS(Xchain[5000], 5000);

  TS_ASSERT(Xconv.size() < Xchain.size());

  TS_ASSERT(AnalysisDataService::Instance().doesExist("Parameters"));
  ITableWorkspace_sptr Ptable = boost::dynamic_pointer_cast<ITableWorkspace>(
      API::AnalysisDataService::Instance().retrieve("Parameters"));

  TS_ASSERT(Ptable);
  TS_ASSERT_EQUALS(Ptable->columnCount(), 4);
  TS_ASSERT_EQUALS(Ptable->rowCount(), n);
  TS_ASSERT_EQUALS(Ptable->getColumn(0)->type(), "str");
  TS_ASSERT_EQUALS(Ptable->getColumn(0)->name(), "Name");
  TS_ASSERT_EQUALS(Ptable->getColumn(1)->type(), "double");
  TS_ASSERT_EQUALS(Ptable->getColumn(1)->name(), "Value");
  TS_ASSERT_EQUALS(Ptable->getColumn(2)->type(), "double");
  TS_ASSERT_EQUALS(Ptable->getColumn(2)->name(), "Left's error");
  TS_ASSERT_EQUALS(Ptable->getColumn(3)->type(), "double");
  TS_ASSERT_EQUALS(Ptable->getColumn(3)->name(), "Rigth's error");
  TS_ASSERT(Ptable->Double(0, 1) == fun->getParameter("Height"));
  TS_ASSERT(Ptable->Double(1, 1) == fun->getParameter("Lifetime"));
}
}
class FABADAMinimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FABADAMinimizerTest *createSuite() {
    return new FABADAMinimizerTest();
  }
  static void destroySuite(FABADAMinimizerTest *suite) { delete suite; }

  void test_expDecay() {
    auto ws2 = createTestWorkspace();
    doTestExpDecay(ws2);
  }

  void test_low_MaxIterations() {
    auto ws2 = createTestWorkspace();

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 1.);
    fun->setParameter("Lifetime", 1.0);

    Algorithms::Fit fit;
    fit.initialize();

    fit.setRethrows(true);
    fit.setProperty("Function", fun);
    fit.setProperty("InputWorkspace", ws2);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("CreateOutput", true);
    fit.setProperty("MaxIterations", 10);
    fit.setProperty("Minimizer", "FABADA,ChainLength=5000,StepsBetweenValues="
                                 "10,ConvergenceCriteria = 0.01");

    TS_ASSERT_THROWS(fit.execute(), std::length_error);

    TS_ASSERT(!fit.isExecuted());
  }
};

class FABADAMinimizerTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FABADAMinimizerTestPerformance *createSuite() {
    return new FABADAMinimizerTestPerformance();
  }
  static void destroySuite(FABADAMinimizerTestPerformance *suite) {
    delete suite;
  }

  void setUp() override { ws = createTestWorkspace(2000, 2000); }
  void test_expDecay_performance() { doTestExpDecay(ws); }

private:
  API::MatrixWorkspace_sptr ws;
};
#endif /* MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_ */
