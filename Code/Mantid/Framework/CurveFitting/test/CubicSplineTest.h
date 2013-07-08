#ifndef CubicSplineTEST_H_
#define CubicSplineTEST_H_

#include <cmath>
#include <cxxtest/TestSuite.h>
#include <iostream>

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

class CubicSplineTest: public CxxTest::TestSuite
{
public:

  void testKnownInterpolationValues()
  {
    CubicSpline cspline;

    int nData = 10;
    int testDataSize = 10;

    double x[testDataSize];
    double referenceSet[testDataSize];

    setupCubicSpline(cspline, nData);

    //generate a set of test points
    for (int i = 0; i < testDataSize; ++i)
    {
      x[i] = i;
      referenceSet[i] = splineYFunction(x[i]);
    }

    FunctionDomain1DView view(x, nData);
    FunctionValues testDataValues(view);

    cspline.function(view, testDataValues);

    //compare reference data with output data
    for (int i = 0; i < testDataSize; ++i)
    {
      TS_ASSERT_EQUALS(referenceSet[i], testDataValues[i]);
    }

  }

  void testUnknowntInterpolationValues()
  {
    CubicSpline cspline;

    int nData = 10;
    int testDataSize = 30;

    double x[testDataSize];
    double referenceSet[testDataSize];

    setupCubicSpline(cspline, nData);

    //generate a set of test points
    for (int i = 0; i < testDataSize; ++i)
    {
      x[i] = (i * 0.3);
      referenceSet[i] = splineYFunction(x[i]);
    }

    FunctionDomain1DView view(x, testDataSize);
    FunctionValues testDataValues(view);

    cspline.function(view, testDataValues);

    //compare reference data with output data
    for (int i = 0; i < testDataSize; ++i)
    {
      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }
  }

private:

  //function which we wish to use to generate our corresponding y data
  double splineYFunction(double x)
  {
    return sin((2 * M_PI / 18) * x);
  }

  //setup a CubicSpline class for testing
  void setupCubicSpline(CubicSpline& cspline, int nData)
  {
    cspline.setAttributeValue("n", nData);

    //calculate a reference set to check against
    for (int i = 0; i < nData; ++i)
    {
      cspline.setXAttribute(i, i);
      cspline.setParameter(static_cast<size_t>(i), splineYFunction(i));
    }
  }

};

#endif /*CubicSplineTEST_H_*/
