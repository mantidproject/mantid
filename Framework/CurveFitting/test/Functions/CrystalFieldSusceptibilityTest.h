#ifndef CRYSTALFIELDSUSCEPTIBILITYTEST_H_
#define CRYSTALFIELDSUSCEPTIBILITYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/ParameterTie.h"
#include "MantidCurveFitting/Functions/CrystalFieldSusceptibility.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;

class CrystalFieldSusceptibilityTest : public CxxTest::TestSuite {
public:
  void test_evaluate() {
    CrystalFieldSusceptibility fun;
    fun.setParameter("B20", 0.37737);
    fun.setParameter("B22", 3.9770);
    fun.setParameter("B40", -0.031787);
    fun.setParameter("B42", -0.11611);
    fun.setParameter("B44", -0.12544);
    fun.setAttributeValue("Ion", "Ce");
    fun.setAttributeValue("Unit", "bohr");
    fun.setAttributeValue("Hdir", std::vector<double>{1., 1., 1.});
    fun.setAttributeValue("inverse", true);
    FunctionDomain1DVector x(10.0, 300.0, 100);
    FunctionValues y(x);
    fun.function(x, y);

    // Test values obtained from McPhase, interpolated by a polynomial
    auto testFun1 = FunctionFactory::Instance().createInitialized(
        "name=UserFunction,Formula=a*x*x*x+b*x*x+c*x+d,"
        "a=2.22169e-6,b=-1.310952e-3,c=0.90995,d=1.61086");
    FunctionValues t(x);
    testFun1->function(x, t);

    for (size_t i = 0; i < x.size(); ++i) {
      TS_ASSERT_DELTA(y[i] / t[i], 1, 0.1);
    }
  }

  void test_factory() {
    std::string funDef =
        "name=CrystalFieldSusceptibility,Ion=Pr,Symmetry=C2v,"
        "Unit=SI,Hdir=(1,2,0),inverse=1,"
        "B20=0.37,B22=3.9, B40=-0.03,B42=-0.1,B44=-0.12,Lambda=-0.2, "
        "ties=(BmolX=0,BmolY=0,BmolZ=0,BextX=0,BextY=0,BextZ=0)";
    auto fun = FunctionFactory::Instance().createInitialized(funDef);
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "CrystalFieldSusceptibility");
    TS_ASSERT_EQUALS(fun->getAttribute("Ion").asString(), "Pr");
    TS_ASSERT_EQUALS(fun->getAttribute("Symmetry").asString(), "C2v");
    TS_ASSERT_EQUALS(fun->getAttribute("Unit").asString(), "SI");
    auto Hdir = fun->getAttribute("Hdir").asVector();
    TS_ASSERT_EQUALS(Hdir[0], 1);
    TS_ASSERT_EQUALS(Hdir[1], 2);
    TS_ASSERT_EQUALS(Hdir[2], 0);
    TS_ASSERT_EQUALS(fun->getAttribute("inverse").asBool(), true);
    TS_ASSERT_EQUALS(fun->getParameter("B20"), 0.37);
    TS_ASSERT_EQUALS(fun->getParameter("Lambda"), -0.2);

    size_t nTies = 0;
    for (size_t i = 0; i < fun->nParams(); ++i) {
      auto tie = fun->getTie(i);
      if (tie) {
        ++nTies;
      }
    }
    TS_ASSERT_EQUALS(nTies, 0);
  }
};

#endif /*CRYSTALFIELDSUSCEPTIBILITYTEST_H_*/
