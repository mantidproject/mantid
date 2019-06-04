// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BOUNDARYCONSTRAINTTEST_H_
#define BOUNDARYCONSTRAINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Expression.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidKernel/UnitFactory.h"


using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Constraints;

class BoundaryConstraintTest : public CxxTest::TestSuite {
public:
  void testInitialize1() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("10<Sigma<20");
    bc.initialize(&gaus, expr, false);

    TS_ASSERT_EQUALS(bc.parameterName(), "Sigma");
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

    TS_ASSERT_EQUALS(bc.parameterName(), "Sigma");
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

    TS_ASSERT_EQUALS(bc.parameterName(), "Sigma");
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

    TS_ASSERT_EQUALS(bc.parameterName(), "Sigma");
    TS_ASSERT_DELTA(bc.upper(), 20, 0.0001);
    TS_ASSERT(!bc.hasLower());
  }

  void testInitialize5() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("Sigma==20");
    TS_ASSERT_THROWS(bc.initialize(&gaus, expr, false),
                     const std::invalid_argument &);
  }

  void testInitialize6() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("a<Sigma<b");
    TS_ASSERT_THROWS(bc.initialize(&gaus, expr, false),
                     const std::invalid_argument &);
  }

  // test constructor with lower boundary only
  void testInitialize7() {
    Gaussian gaus;
    gaus.initialize();
    BoundaryConstraint bc(&gaus, "Sigma", 0.0, false);
    TS_ASSERT(bc.hasLower());
    TS_ASSERT(!bc.hasUpper());
    TS_ASSERT_EQUALS(bc.lower(), 0.0);
    TS_ASSERT_EQUALS(bc.parameterName(), "Sigma");
    TS_ASSERT_EQUALS(bc.getLocalFunction(), &gaus);
  }

  void testAsString() {
    Gaussian gaus;
    gaus.initialize();
    {
      auto bc = std::make_unique<BoundaryConstraint>();
      Expression expr;
      expr.parse("Sigma<20");
      bc->initialize(&gaus, expr, false);

      TS_ASSERT_EQUALS(bc->parameterName(), "Sigma");
      TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
      TS_ASSERT(!bc->hasLower());
      gaus.addConstraint(std::move(bc));
    }

    IFunction_sptr fun =
        FunctionFactory::Instance().createInitialized(gaus.asString());
    TS_ASSERT(fun);

    IConstraint *c = fun->getConstraint(2);
    TS_ASSERT(c);
    auto bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->parameterName(), "Sigma");
    TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
    TS_ASSERT(!bc->hasLower());
  }

  void testAsString1() {
    Gaussian gaus;
    gaus.initialize();

    auto bcSigma = std::make_unique<BoundaryConstraint>();
    Expression exprSigma;
    exprSigma.parse("Sigma<20");
    bcSigma->initialize(&gaus, exprSigma, false);
    gaus.addConstraint(std::move(bcSigma));

    auto bcHeight = std::make_unique<BoundaryConstraint>();
    Expression exprHeight;
    exprHeight.parse("1.3<Height<3.4");
    bcHeight->initialize(&gaus, exprHeight, false);
    gaus.addConstraint(std::move(bcHeight));

    IFunction_sptr fun =
        FunctionFactory::Instance().createInitialized(gaus.asString());
    TS_ASSERT(fun);

    IConstraint *c = fun->getConstraint(2);
    TS_ASSERT(c);
    BoundaryConstraint *bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->parameterName(), "Sigma");
    TS_ASSERT_DELTA(bc->upper(), 20, 0.0001);
    TS_ASSERT(!bc->hasLower());

    c = fun->getConstraint(0);
    TS_ASSERT(c);
    bc = dynamic_cast<BoundaryConstraint *>(c);
    TS_ASSERT(bc);

    TS_ASSERT_EQUALS(bc->parameterName(), "Height");
    TS_ASSERT_DELTA(bc->lower(), 1.3, 0.0001);
    TS_ASSERT_DELTA(bc->upper(), 3.4, 0.0001);
  }
};

#endif /*BOUNDARYCONSTRAINTTEST_H_*/
