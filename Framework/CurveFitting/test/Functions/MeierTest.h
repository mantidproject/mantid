#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Functions/Meier.h"

#include <array>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Algorithms;

class MeierTest : public CxxTest::TestSuite {
public:
  void testFunctionName() {
    auto meier = createFunction();
    TS_ASSERT_EQUALS(meier->name(), "MeierV2");
  }

  void testFunctionCategory() {
    auto meier = createFunction();
    TS_ASSERT_EQUALS(meier->category(), "Muon\\MuonSpecific");
  }

  void testFunctionRegisteredInFactory() { FunctionFactory::Instance().createInitialized("name=MeierV2"); }

  void testFunctionHasExpectedOrderedParameters() {
    auto meier = createFunction();

    const size_t expectedNParams = 5;
    TS_ASSERT_EQUALS(meier->nParams(), expectedNParams);

    if (meier->nParams() == expectedNParams) {
      const char *expectedParams[expectedNParams] = {"A0", "FreqD", "FreqQ", "Sigma", "Lambda"};
      auto actualParamsNames = meier->getParameterNames();

      for (size_t i = 0; i < expectedNParams; ++i) {
        TS_ASSERT_EQUALS(actualParamsNames[i], expectedParams[i]);
      }
    }
  }

  void testFunctionHasExpectedOrderedAttributes() {
    auto meier = createFunction();

    const size_t expectedNAttrs = 1;
    TS_ASSERT_EQUALS(meier->nAttributes(), expectedNAttrs);

    if (meier->nAttributes() == expectedNAttrs) {
      const char *expectedAttrs[expectedNAttrs] = {"Spin"};
      auto actualAttrsNames = meier->getAttributeNames();

      for (size_t i = 0; i < expectedNAttrs; ++i) {
        TS_ASSERT_EQUALS(actualAttrsNames[i], expectedAttrs[i]);
      }
    }
  }

  void testFunctionGivesExpectedValueForGivenInput() {
    MeierV2 meier;
    meier.initialize();

    meier.setParameter("A0", 0.5);
    meier.setParameter("FreqD", 0.01);
    meier.setParameter("FreqQ", 0.05);
    meier.setParameter("Lambda", 0.1);
    meier.setParameter("Sigma", 0.2);

    std::array<double, 4> xValues = {0.0, 4.0, 8.0, 12.0};
    std::array<double, 4> expected = {0.5, 0.0920992725837422, 0.0023798684614228663, 0.0007490849206591537};
    std::array<double, 4> yValues;

    meier.function1D(yValues.data(), xValues.data(), 4);

    for (size_t i = 0; i < 4; i++) {
      TS_ASSERT_DELTA(yValues[i], expected[i], 1e-5);
    }
  }

  void testFunctionFit() {
    auto targetFun = createFunction();
    targetFun->setParameter("A0", 0.5);
    targetFun->setParameter("FreqD", 0.01);
    targetFun->setParameter("FreqQ", 0.05);
    targetFun->setParameter("Lambda", 0.1);
    targetFun->setParameter("Sigma", 0.2);

    auto guessFun = createFunction();
    guessFun->setParameter("A0", 0.55);
    guessFun->setParameter("FreqD", 0.015);
    guessFun->setParameter("FreqQ", 0.055);
    guessFun->setParameter("Lambda", 0.15);
    guessFun->setParameter("Sigma", 0.25);

    auto ws = createWorkspace(*targetFun);
    auto fit = AlgorithmManager::Instance().create("Fit");
    fit->setProperty("Function", guessFun);
    fit->setProperty("InputWorkspace", ws);
    fit->setProperty("MaxIterations", 2000);
    fit->execute();

    IFunction_sptr outputFunction = fit->getProperty("Function");
    const double atol = 0.01;
    TS_ASSERT_DELTA(outputFunction->getParameter("A0"), 0.5, atol);
    TS_ASSERT_DELTA(outputFunction->getParameter("FreqD"), 0.01, atol);
    TS_ASSERT_DELTA(outputFunction->getParameter("FreqQ"), 0.05, atol);
    TS_ASSERT_DELTA(outputFunction->getParameter("Lambda"), 0.1, atol);
    TS_ASSERT_DELTA(outputFunction->getParameter("Sigma"), 0.2, atol);
    TS_ASSERT_EQUALS(outputFunction->getAttribute("Spin").asDouble(), 3.5);
  }

private:
  MatrixWorkspace_sptr createWorkspace(const IFunction &fun) const {
    int n = 80;
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, n, n);

    FunctionDomain1DVector x({0.1,  0.3,  0.5,  0.7,  0.9,  1.1,  1.3,  1.5,  1.7,  1.9,  2.1,  2.3,  2.5,  2.7,
                              2.9,  3.1,  3.3,  3.5,  3.7,  3.9,  4.1,  4.3,  4.5,  4.7,  4.9,  5.1,  5.3,  5.5,
                              5.7,  5.9,  6.1,  6.3,  6.5,  6.7,  6.9,  7.1,  7.3,  7.5,  7.7,  7.9,  8.1,  8.3,
                              8.5,  8.7,  8.9,  9.1,  9.3,  9.5,  9.7,  9.9,  10.1, 10.3, 10.5, 10.7, 10.9, 11.1,
                              11.3, 11.5, 11.7, 11.9, 12.1, 12.3, 12.5, 12.7, 12.9, 13.1, 13.3, 13.5, 13.7, 13.9,
                              14.1, 14.3, 14.5, 14.7, 14.9, 15.1, 15.3, 15.5, 15.7, 15.9});
    FunctionValues y(x);
    std::vector<double> e(n, 1.0);

    fun.function(x, y);
    ws->setPoints(0, x.toVector());
    ws->dataY(0) = y.toVector();
    ws->dataE(0) = e;

    return ws;
  }

  IFunction_sptr createFunction() {
    auto fun = std::make_shared<MeierV2>();
    fun->initialize();
    return fun;
  }
};
