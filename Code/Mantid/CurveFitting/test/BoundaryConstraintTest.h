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
    gaus->setWidth(2.2);

    BoundaryConstraint* bc = new BoundaryConstraint("Sigma");

    TS_ASSERT(!bc->hasLower());
    TS_ASSERT(!bc->hasUpper());

    bc->setLower(1.0);
    bc->setUpper(2.0);

    TS_ASSERT(bc->hasLower());
    TS_ASSERT(bc->hasUpper());

  }

};

#endif /*BOUNDARYCONSTRAINTTEST_H_*/
