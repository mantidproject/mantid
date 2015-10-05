#ifndef BOUNDARYCONSTRAINTTEST_H_
#define BOUNDARYCONSTRAINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Lorentzian.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Expression.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class BoundaryConstraintTest : public CxxTest::TestSuite {
public:
  void test1() {
    // set up fitting function
    Gaussian gaus;
    gaus.initialize();
    gaus.setCentre(11.2);
    gaus.setHeight(100.7);
    gaus.setParameter("Sigma", 1.1);

    BoundaryConstraint bc;
    bc.reset(&gaus, 2);

    TS_ASSERT(!bc.hasLower());
    TS_ASSERT(!bc.hasUpper());

    bc.setLower(1.0);
    bc.setUpper(2.0);

    TS_ASSERT(bc.hasLower());
    TS_ASSERT(bc.hasUpper());

    BoundaryConstraint bc2;
    ;
    bc2.reset(&gaus, 2);
    bc2.setBounds(10, 20);

    TS_ASSERT_DELTA(bc2.lower(), 10, 0.0001);
    TS_ASSERT_DELTA(bc2.upper(), 20, 0.0001);

    TS_ASSERT_DELTA(gaus.getParameter("Sigma"), 1.1, 0.0001);

    bc2.setParamToSatisfyConstraint();
    TS_ASSERT_DELTA(gaus.getParameter("Sigma"), 10.0, 0.0001);
  }

  void testInitialize1() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("10<Sigma<20");
    bc.initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc.getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc.lower(), 10, 0.0001);
    TS_ASSERT_DELTA(bc.upper(), 20, 0.0001);
  }

  void testInitialize2() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("20>Sigma>10");
    bc.initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc.getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc.lower(), 10, 0.0001);
    TS_ASSERT_DELTA(bc.upper(), 20, 0.0001);
  }

  void testInitialize3() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("10<Sigma");
    bc.initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc.getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc.lower(), 10, 0.0001);
    TS_ASSERT(!bc.hasUpper());
  }

  void testInitialize4() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("Sigma<20");
    bc.initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc.getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc.upper(), 20, 0.0001);
    TS_ASSERT(!bc.hasLower());
  }

  void testInitialize5() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("Sigma==20");
    TS_ASSERT_THROWS(bc.initialize(&gaus, expr, false), std::invalid_argument);
  }

  void testInitialize6() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("a<Sigma<b");
    TS_ASSERT_THROWS(bc.initialize(&gaus, expr, false), std::invalid_argument);
  }

  // test constructor with lower boundary only
  void testInitialize7() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc(&gaus, "Sigma", 0.0, false);
    TS_ASSERT(bc.hasLower());
    TS_ASSERT(!bc.hasUpper());
    TS_ASSERT_EQUALS(bc.lower(), 0.0);
    TS_ASSERT_EQUALS(bc.getParameterName(), "Sigma");
    TS_ASSERT_EQUALS(bc.getFunction(), &gaus);
  }

  void testAsString() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint *bc = new BoundaryConstraint;
    Expression expr;
    expr.parse("Sigma<20");
    bc->initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc->getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
    TS_ASSERT(!bc->hasLower());

    gaus.addConstraint(bc);
    IFunction_sptr fun =
        FunctionFactory::Instance().createInitialized(gaus.asString());
    TS_ASSERT(fun);

    IConstraint *c = fun->getConstraint(2);
    TS_ASSERT(c);
    bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
    TS_ASSERT(!bc->hasLower());
  }

  void testAsString1() {
    Gaussian gaus;
    gaus.initialize();

    BoundaryConstraint *bcSigma = new BoundaryConstraint;
    Expression exprSigma;
    exprSigma.parse("Sigma<20");
    bcSigma->initialize(&gaus, exprSigma, false);
    gaus.addConstraint(bcSigma);

    BoundaryConstraint *bcHeight = new BoundaryConstraint;
    Expression exprHeight;
    exprHeight.parse("1.3<Height<3.4");
    bcHeight->initialize(&gaus, exprHeight, false);
    gaus.addConstraint(bcHeight);

    IFunction_sptr fun =
        FunctionFactory::Instance().createInitialized(gaus.asString());
    TS_ASSERT(fun);

    IConstraint *c = fun->getConstraint(2);
    TS_ASSERT(c);
    BoundaryConstraint *bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->getParameterName(), "Sigma");
    TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
    TS_ASSERT(!bc->hasLower());

    c = fun->getConstraint(0);
    TS_ASSERT(c);
    bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->getParameterName(), "Height");
    TS_ASSERT_DELTA(bc->lower(), 1.3, 0.0001);
    TS_ASSERT_DELTA(bc->upper(), 3.4, 0.0001);
  }
};

#endif /*BOUNDARYCONSTRAINTTEST_H_*/
