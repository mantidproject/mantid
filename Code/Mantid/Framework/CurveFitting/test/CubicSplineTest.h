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

//  void testKnownInterpolationValues()
//  {
//    CubicSpline cspline;
//    int nData = 10;
//    double ref[nData];
//    double xVals[nData];
//
//    setupCubicSpline(cspline, nData);
//
//    //calculate a reference set to check against
//    for (int i = 0; i < nData; ++i)
//    {
//      double x_val = i;
//      xVals[i] = x_val;
//      ref[i] = splineYFunction(x_val);
//    }
//
//    FunctionDomain1DView view(xVals, nData);
//    FunctionValues testDataValues(view);
//
//    cspline.function(view, testDataValues);
//
//    //compare reference data with output data
//    for (int i = 0; i < nData; ++i)
//    {
//      TS_ASSERT_DELTA(ref[i], testDataValues[i], 1e-7);
//    }
//
//  }

  void testUnknowntInterpolationValues()
  {

    CubicSpline cspline;

    int nData = 10;
    int testDataSize = 30;

    double x[testDataSize];
    double referenceSet[testDataSize];

    //set up class for testing
    cspline.setAttributeValue("n", nData);

    //calculate a reference set to check against
    for (int i = 0; i < nData; ++i)
    {
      cspline.setXAttribute(i, i);
      cspline.setParameter(static_cast<size_t>(i), splineYFunction(i));
    }

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
      std::cout << "Ref: " << referenceSet[i] << std::endl;
      std::cout << "Out: " << testDataValues[i] << std::endl;

      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }

    std::cout << cspline.asString() << std::endl;
  }

private:

  //function which we wish to use to generate our corresponding y data
  double splineYFunction(double x)
  {
    return sin((2*M_PI/18) * x);
  }

  void setupCubicSpline(CubicSpline& cspline, int nData)
  {

  }

};

#endif /*CubicSplineTEST_H_*/
