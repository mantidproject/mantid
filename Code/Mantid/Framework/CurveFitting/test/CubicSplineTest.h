#ifndef CubicSplineTEST_H_
#define CubicSplineTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CubicSpline.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;

class CubicSplineTest : public CxxTest::TestSuite
{
public:

  void testNormal() {

  }

};

#endif /*CubicSplineTEST_H_*/
