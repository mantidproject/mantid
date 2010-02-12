#ifndef BOUNDARYCONSTRAINTTEST_H_
#define BOUNDARYCONSTRAINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Gaussian.h"
#include "MantidCurveFitting/Lorentzian.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/Expression.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"

//using namespace Mantid::Kernel;
using namespace Mantid::API;
//using namespace Mantid::DataObjects;
using namespace Mantid::CurveFitting;


class BoundaryConstraintTest : public CxxTest::TestSuite
{
public:



  void test1()
  {
    // set up fitting function
    Gaussian* gaus = new Gaussian();
    gaus->initialize();
    gaus->setCentre(11.2);
    gaus->setHeight(100.7);
    gaus->setParameter("Sigma",1.1);

    BoundaryConstraint* bc = new BoundaryConstraint("Sigma");

    TS_ASSERT(!bc->hasLower());
    TS_ASSERT(!bc->hasUpper());

    bc->setLower(1.0);
    bc->setUpper(2.0);

    TS_ASSERT(bc->hasLower());
    TS_ASSERT(bc->hasUpper());

    BoundaryConstraint* bc2 = new BoundaryConstraint("Sigma",10,20);

    TS_ASSERT_DELTA( bc2->lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc2->upper(), 20 ,0.0001);

    TS_ASSERT_DELTA( gaus->getParameter("Sigma"), 1.1 ,0.0001);
    bc2->setParamToSatisfyConstraint(gaus);
    TS_ASSERT_DELTA( gaus->getParameter("Sigma"), 10.0 ,0.0001);



  }

  void testInitialize1()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(10<Theta<20)");
    bc.initialize(expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Theta" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
  }

  void testInitialize2()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(20>Theta>10)");
    bc.initialize(expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Theta" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
  }

  void testInitialize3()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(10<Theta)");
    bc.initialize(expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Theta" );
    TS_ASSERT_DELTA( bc.lower(), 10 ,0.0001);
    TS_ASSERT( !bc.hasUpper() );
  }

  void testInitialize4()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(Theta<20)");
    bc.initialize(expr);

    TS_ASSERT_EQUALS( bc.getParameterName(), "Theta" );
    TS_ASSERT_DELTA( bc.upper(), 20 ,0.0001);
    TS_ASSERT( !bc.hasLower() );
  }

  void testInitialize5()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(Theta==20)");
    TS_ASSERT_THROWS(bc.initialize(expr),std::invalid_argument);
  }

  void testInitialize6()
  {
    BoundaryConstraint bc;
    Expression expr;
    expr.parse("BoundaryConstraint(a<Theta<b)");
    TS_ASSERT_THROWS(bc.initialize(expr),std::invalid_argument);
  }
};

#endif /*BOUNDARYCONSTRAINTTEST_H_*/
