#ifndef CubicSplineTEST_H_
#define CubicSplineTEST_H_

#include <boost/scoped_array.hpp>
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

class CubicSplineTest : public CxxTest::TestSuite {
public:
  void testSetNAttribute() {
    CubicSpline cspline;

    // cspline created with 3 attributes
    TS_ASSERT(cspline.getAttribute("n").asInt() == 3);

    // resize to have 10 attributes
    cspline.setAttributeValue("n", 10);

    TS_ASSERT(cspline.getAttribute("n").asInt() == 10);

    // Check that resizing the spline has initialised the attributes/parameters
    for (int i = 0; i < 10; ++i) {
      std::string index = boost::lexical_cast<std::string>(i);

      std::string xAttrName = "x" + index;
      std::string yAttrName = "y" + index;

      TS_ASSERT_EQUALS(cspline.getAttribute(xAttrName).asDouble(), i);
      TS_ASSERT_EQUALS(cspline.getParameter(yAttrName), 0);
    }
  }

  void testSetNAttributeBoundary() {
    CubicSpline cspline;

    // cubic splines must have at least 3 points
    TS_ASSERT_THROWS(cspline.setAttributeValue("n", 2), std::invalid_argument);

    // set the number of points to something sensible
    cspline.setAttributeValue("n", 5);

    // attempt to make it smaller than it already is
    TS_ASSERT_THROWS(cspline.setAttributeValue("n", 4), std::invalid_argument);

    size_t oldAttrN = cspline.nAttributes();

    // attempt to set the attribute to the same value doesn't
    // change anything
    TS_ASSERT_THROWS_NOTHING(cspline.setAttributeValue("n", 5));
    TS_ASSERT_EQUALS(oldAttrN, cspline.nAttributes());
  }

  void testSetXAttributeValue() {
    CubicSpline cspline;

    // setup x and y  values
    cspline.setAttributeValue("n", 10);

    for (int i = 0; i < 10; ++i) {
      // set the x values to be some arbitary value
      cspline.setXAttribute(i, i * 2);

      std::string index = boost::lexical_cast<std::string>(i);
      std::string xAttrName = "x" + index;

      // check x value is equal to what we set
      TS_ASSERT_EQUALS(cspline.getAttribute(xAttrName).asDouble(), i * 2);
    }
  }

  void testSetXAttributeValueBoundary() {
    CubicSpline cspline;

    // check that an invalid index throws errors
    TS_ASSERT_THROWS(cspline.setXAttribute(4, 0), std::range_error);

    TS_ASSERT_THROWS(cspline.setXAttribute(-1, 4), std::range_error);
  }

  void testKnownInterpolationValues() {
    CubicSpline cspline;

    int nData = 10; // number of data points to fit too

    boost::scoped_array<double> x(new double[nData]);
    boost::scoped_array<double> referenceSet(new double[nData]);

    // setup spline with n data points separated by 1
    setupCubicSpline(cspline, nData, 1);

    // generate a set of test points
    generateTestData(nData, referenceSet, x, 1);

    FunctionDomain1DView view(x.get(), nData);
    FunctionValues testDataValues(view);

    cspline.function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < nData; ++i) {
      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }
  }

  void testOutOfOrderInterploationPoints() {
    CubicSpline cspline;

    setupCubicSpline(cspline, 10, 1);

    // swap the values of some points
    cspline.setXAttribute(3, 1);
    cspline.setXAttribute(1, 3);

    int testDataSize = 5;

    boost::scoped_array<double> x(new double[testDataSize]);
    boost::scoped_array<double> refSet(new double[testDataSize]);

    generateTestData(testDataSize, refSet, x, 1);

    FunctionDomain1DView view(x.get(), testDataSize);
    FunctionValues testDataValues(view);

    cspline.function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(refSet[i], testDataValues[i], 1e-4);
    }
  }

  void testUnknowntInterpolationValues() {
    CubicSpline cspline;

    int nData = 20;
    int testDataSize = 30;

    boost::scoped_array<double> x(new double[testDataSize]);
    boost::scoped_array<double> referenceSet(new double[testDataSize]);

    // init spline with 10 data points
    setupCubicSpline(cspline, nData, 1);

    // generate three test points for every data point
    generateTestData(testDataSize, referenceSet, x, 0.3);

    FunctionDomain1DView view(x.get(), testDataSize);
    FunctionValues testDataValues(view);

    cspline.function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }
  }

  void testCalculateDerivative() {
    CubicSpline cspline;

    int nData = 10;
    int testDataSize = 10;

    boost::scoped_array<double> x(new double[nData]);
    boost::scoped_array<double> refSet(new double[testDataSize]);
    boost::scoped_array<double> testDataValues(new double[testDataSize]);

    setupCubicSpline(cspline, nData, 1);
    generateDerviTestData(testDataSize, refSet, x, 1, 1);

    cspline.derivative1D(testDataValues.get(), x.get(), nData, 1);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(refSet[i], testDataValues[i], 1e-2);
    }
  }

private:
  // generate a set of uniform points to test the spline
  void generateTestData(int numTests, boost::scoped_array<double> &refSet,
                        boost::scoped_array<double> &xValues, double xModify) {
    for (int i = 0; i < numTests; ++i) {
      xValues[i] = (i * xModify);
      refSet[i] = splineYFunction(xValues[i]);
    }
  }

  void generateDerviTestData(int numTests, boost::scoped_array<double> &refSet,
                             boost::scoped_array<double> &xValues,
                             double xModify, double h) {
    for (int i = 0; i < numTests; ++i) {
      xValues[i] = (i * xModify);
      refSet[i] =
          (splineYFunction(xValues[i] + h) - splineYFunction(xValues[i] - h)) /
          2 * h;
    }
  }

  // function which we wish to use to generate our corresponding y data
  double splineYFunction(double x) { return x * 2; }

  // setup a CubicSpline class for testing
  void setupCubicSpline(CubicSpline &cspline, int nData, double xModify) {
    cspline.setAttributeValue("n", nData);

    // calculate a reference set to check against
    for (int i = 0; i < nData; ++i) {
      cspline.setXAttribute(i, i * xModify);
      cspline.setParameter(static_cast<size_t>(i),
                           splineYFunction(i * xModify));
    }
  }
};

#endif /*CubicSplineTEST_H_*/
