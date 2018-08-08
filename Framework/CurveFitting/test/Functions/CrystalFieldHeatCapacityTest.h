#ifndef CRYSTALFIELDHEATCAPACITYTEST_H_
#define CRYSTALFIELDHEATCAPACITYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Algorithms/EvaluateFunction.h"
#include "MantidCurveFitting/Functions/CrystalFieldHeatCapacity.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldHeatCapacityTest : public CxxTest::TestSuite {
public:
  void test_evaluate() {
    CrystalFieldHeatCapacity fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    FunctionDomain1DVector x(1.0, 300.0, 100);
    FunctionValues y(x);
    fun.function(x, y);

    // Test function values obtained from McPhase, interpolated by two cubics
    auto testFun1 = FunctionFactory::Instance().createInitialized(
        "name=UserFunction,Formula=a*x*x*x+b*x*x+c*x+d,"
        "a=6.1504e-6,b=2.4075e-5,c=-7.9692e-3,d=5.9915e-2");
    FunctionValues t1(x);
    testFun1->function(x, t1);
    auto testFun2 = FunctionFactory::Instance().createInitialized(
        "name=UserFunction,Formula=a*x*x*x+b*x*x+c*x+d,"
        "a=1.6632e-6,b=-1.1572e-3,c=0.24439,d=-10.351");
    FunctionValues t2(x);
    testFun2->function(x, t2);

    for (size_t i = 0; i < x.size(); ++i) {
      // Below 80K use polynomial 1, above use polynomial 2
      TS_ASSERT_DELTA(y[i], (x[i] < 80) ? t1[i] : t2[i], 0.2);
    }
  }

  void test_factory() {
    std::string funDef =
        "name=CrystalFieldHeatCapacity,Ion=Ce,Symmetry=C2v,"
        "B20=0.37,B22=3.9, B40=-0.03,B42=-0.1,B44=-0.12, "
        "ties=(BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=BextX)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "CrystalFieldHeatCapacity");
    TS_ASSERT_EQUALS(fun->getAttribute("Ion").asString(), "Ce");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetry").asString(), "C2v");
    TS_ASSERT_EQUALS(fun->getParameter("B20"), 0.37);
    TS_ASSERT_EQUALS(fun->getParameter("B42"), -0.1);

    auto i = fun->parameterIndex("BextZ");
    auto tie = fun->getTie(i);
    TS_ASSERT(tie);
    if (tie) {
      TS_ASSERT_EQUALS(tie->asString(), "BextZ=BextX");
    }

    size_t nTies = 0;
    for (size_t i = 0; i < fun->nParams(); ++i) {
      auto tie = fun->getTie(i);
      if (tie) {
        ++nTies;
      }
    }
    TS_ASSERT_EQUALS(nTies, 1); // Fixed values not ties.
  }

  void test_evaluate_function() {
    auto ws =
        WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100, 1.0, 3.0);
    std::string fun =
        "name=CrystalFieldHeatCapacity,Ion=Ce,Symmetry=C2v,"
        "B20=0.37,B22=3.9, B40=-0.03,B42=-0.1,B44=-0.12, "
        "ties=(BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=BextX)";

    Algorithms::EvaluateFunction eval;
    eval.initialize();
    eval.setProperty("Function", fun);
    eval.setProperty("InputWorkspace", ws);
    eval.setPropertyValue("OutputWorkspace", "out");
    eval.execute();

    auto out =
        API::AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("out");

    auto y = out->histogram(1).counts();
    TS_ASSERT_DELTA(y[10], 0.0305, 1e-4);
    TS_ASSERT_DELTA(y[30], 3.7753, 1e-4);
    TS_ASSERT_DELTA(y[70], 5.1547, 1e-4);
    TS_ASSERT_DELTA(y[99], 3.4470, 1e-4);

    API::AnalysisDataService::Instance().clear();
  }
};

#endif /*CRYSTALFIELDHEATCAPACITYTEST_H_*/
