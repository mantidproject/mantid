#ifndef MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_
#define MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/FuncMinimizers/FABADAMinimizer.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidAPI/AlgorithmManager.h"

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

class FABADAMinimizerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FABADAMinimizerTest *createSuite() {
    return new FABADAMinimizerTest();
  }
  static void destroySuite(FABADAMinimizerTest *suite) { delete suite; }

  void test_expDecay() {
    auto ws2 = createExpDecayWorkspace();

    API::IFunction_sptr fun(new ExpDecay);
    fun->setParameter("Height", 8.);
    fun->setParameter("Lifetime", 1.0);

    Algorithms::Fit fit;
    fit.initialize();
    fit.setChild(true);
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

    size_t nParams = fun->nParams();

    // Test PDF workspace
    MatrixWorkspace_sptr pdf = fit.getProperty("PDF");
    TS_ASSERT(pdf);
    TS_ASSERT_EQUALS(pdf->getNumberHistograms(), nParams + 1);
    TS_ASSERT_EQUALS(pdf->x(0).size(), 21);
    TS_ASSERT_EQUALS(pdf->y(0).size(), 20);
    TS_ASSERT_DELTA(pdf->y(0)[7], 0.41, 0.01);
    TS_ASSERT_DELTA(pdf->y(1)[8], 5.24, 0.01);
    TS_ASSERT_DELTA(pdf->y(2)[0], 0.44, 0.01);

    //  Test CostFunction table
    ITableWorkspace_sptr costFunctTable = fit.getProperty("CostFunctionTable");
    TS_ASSERT(costFunctTable);
    TS_ASSERT_EQUALS(costFunctTable->columnCount(), 4);
    TS_ASSERT_EQUALS(costFunctTable->rowCount(), 1);
    TS_ASSERT_EQUALS(costFunctTable->getColumn(0)->type(), "double");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(0)->name(), "Chi2min");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(1)->name(), "Chi2MP");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(2)->name(), "Chi2min_red");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(3)->type(), "double");
    TS_ASSERT_EQUALS(costFunctTable->getColumn(3)->name(), "Chi2MP_red");
    TS_ASSERT_LESS_THAN_EQUALS(costFunctTable->Double(0, 0),
                               costFunctTable->Double(0, 1));
    TS_ASSERT_LESS_THAN_EQUALS(costFunctTable->Double(0, 2),
                               costFunctTable->Double(0, 3));
    TS_ASSERT_DELTA(costFunctTable->Double(0, 0), costFunctTable->Double(0, 1),
                    1.5);
    TS_ASSERT_DELTA(costFunctTable->Double(0, 0), 0.0, 1.0);

    // Test ConvergedChain workspace
    MatrixWorkspace_sptr convChain = fit.getProperty("ConvergedChain");
    TS_ASSERT(convChain);
    TS_ASSERT_EQUALS(convChain->getNumberHistograms(), nParams + 1);
    TS_ASSERT_EQUALS(convChain->x(0).size(), 500);
    TS_ASSERT_EQUALS(convChain->x(0)[437], 437);
    TS_ASSERT_DELTA(convChain->y(0)[0], 9.4, 0.1);
    TS_ASSERT_DELTA(convChain->y(0)[249], 9.4, 0.1);
    TS_ASSERT_DELTA(convChain->y(0)[499], 10.4, 0.1);
    TS_ASSERT_DELTA(convChain->y(1)[0], 0.5, 0.1);
    TS_ASSERT_DELTA(convChain->y(1)[249], 0.5, 0.1);
    TS_ASSERT_DELTA(convChain->y(1)[499], 0.5, 0.1);
    TS_ASSERT_DELTA(convChain->y(2)[0], 0.3, 0.1);
    TS_ASSERT_DELTA(convChain->y(2)[249], 0.2, 0.1);
    TS_ASSERT_DELTA(convChain->y(2)[499], 0.6, 0.1);

    // Test Chain workspace
    MatrixWorkspace_sptr chain = fit.getProperty("Chains");
    TS_ASSERT(chain);
    TS_ASSERT_EQUALS(chain->getNumberHistograms(), nParams + 1);
    TS_ASSERT_EQUALS(chain->x(0)[5000], 5000);
    TS_ASSERT(convChain->x(0).size() < chain->x(0).size());

    // Parameters workspace
    ITableWorkspace_sptr param = fit.getProperty("Parameters");
    TS_ASSERT(param);
    TS_ASSERT_EQUALS(param->columnCount(), 4);
    TS_ASSERT_EQUALS(param->rowCount(), nParams);
    TS_ASSERT_EQUALS(param->getColumn(0)->type(), "str");
    TS_ASSERT_EQUALS(param->getColumn(0)->name(), "Name");
    TS_ASSERT_EQUALS(param->getColumn(1)->type(), "double");
    TS_ASSERT_EQUALS(param->getColumn(1)->name(), "Value");
    TS_ASSERT_EQUALS(param->getColumn(2)->type(), "double");
    TS_ASSERT_EQUALS(param->getColumn(2)->name(), "Left's error");
    TS_ASSERT_EQUALS(param->getColumn(3)->type(), "double");
    TS_ASSERT_EQUALS(param->getColumn(3)->name(), "Rigth's error");
    TS_ASSERT(param->Double(0, 1) == fun->getParameter("Height"));
    TS_ASSERT(param->Double(1, 1) == fun->getParameter("Lifetime"));
  }

  void test_low_MaxIterations() {
    auto ws2 = createExpDecayWorkspace();

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

  void test_cosineWithConstraint() {

    auto ws2 = createCosineWorkspace();

    Algorithms::Fit fit;
    fit.initialize();
    fit.setChild(true);
    fit.setPropertyValue("Function", "name=UserFunction, Formula=a*cos(b*x), "
                                     "a=2, b=-1, constraints=(0<b<1)");
    fit.setProperty("InputWorkspace", ws2);
    fit.setProperty("WorkspaceIndex", 0);
    fit.setProperty("CreateOutput", true);
    fit.setProperty("MaxIterations", 100000);
    fit.setProperty("Minimizer", "FABADA,ChainLength=5000,StepsBetweenValues="
                                 "10,ConvergenceCriteria=0.1,CostFunctionTable="
                                 "CostFunction,Chains=Chain,ConvergedChain"
                                 "=ConvergedChain,Parameters=Parameters");
    TS_ASSERT_THROWS_NOTHING(fit.execute());
	TS_ASSERT_EQUALS(fit.getPropertyValue("OutputStatus"), "success");
	IFunction_sptr fun = fit.getProperty("Function");
    TS_ASSERT_DELTA(fun->getParameter("a"), 0.9, 0.1);
    TS_ASSERT_DELTA(fun->getParameter("b"), 0.9, 0.1);

    // Test PDF workspace
    MatrixWorkspace_sptr pdf = fit.getProperty("PDF");
    TS_ASSERT(pdf);
    TS_ASSERT_DELTA(pdf->y(0)[11], 0.55, 0.01);
    TS_ASSERT_DELTA(pdf->y(1)[19], 4.88, 0.01);
    TS_ASSERT_DELTA(pdf->y(2)[0], 0.34, 0.01);

    //  Test CostFunction table
    ITableWorkspace_sptr costFunc = fit.getProperty("CostFunctionTable");
    TS_ASSERT(costFunc);
    TS_ASSERT_EQUALS(costFunc->columnCount(), 4);
    TS_ASSERT_EQUALS(costFunc->rowCount(), 1);
    TS_ASSERT_LESS_THAN_EQUALS(costFunc->Double(0, 0), costFunc->Double(0, 1));
    TS_ASSERT_LESS_THAN_EQUALS(costFunc->Double(0, 2), costFunc->Double(0, 3));
    TS_ASSERT_DELTA(costFunc->Double(0, 0), costFunc->Double(0, 1), 0.5);
    TS_ASSERT_DELTA(costFunc->Double(0, 0), 0.0, 1.0);

    // Parameters workspace
    ITableWorkspace_sptr param = fit.getProperty("Parameters");
    TS_ASSERT(param);
    TS_ASSERT_EQUALS(param->columnCount(), 4);
    TS_ASSERT_EQUALS(param->rowCount(), 2);
    TS_ASSERT_DELTA(param->Double(0, 2), -0.50, 0.01);
    TS_ASSERT_DELTA(param->Double(0, 3), 1.10, 0.01);
    TS_ASSERT_DELTA(param->Double(1, 2), -0.07, 0.01);
    TS_ASSERT_DELTA(param->Double(1, 3), 0.01, 0.1);
  }

private:
  API::MatrixWorkspace_sptr createExpDecayWorkspace() {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(1, 20, 20);

    Mantid::MantidVec &x = ws2->dataX(0);
    Mantid::MantidVec &y = ws2->dataY(0);
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      x[i] = 0.1 * double(i);
      y[i] = 10.0 * exp(-(x[i]) / 0.5);
    }

    return ws2;
  }

  API::MatrixWorkspace_sptr createCosineWorkspace() {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    ws2->initialize(1, 20, 20);

    Mantid::MantidVec &x = ws2->dataX(0);
    Mantid::MantidVec &y = ws2->dataY(0);
    for (size_t i = 0; i < ws2->blocksize(); ++i) {
      double xx = 2 * M_PI * i / 20;
      x[i] = xx;
      y[i] = cos(xx);
    }

    return ws2;
  }
};

#endif /* MANTID_CURVEFITTING_FABADAMINIMIZERTEST_H_ */
