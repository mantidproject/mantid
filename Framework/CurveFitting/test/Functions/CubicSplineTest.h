#ifndef CubicSplineTEST_H_
#define CubicSplineTEST_H_

#include "MantidCurveFitting/Functions/CubicSpline.h"
#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::CurveFitting::Functions;

class CubicSplineTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CubicSplineTest *createSuite() { return new CubicSplineTest(); }
  static void destroySuite(CubicSplineTest *suite) { delete suite; }

  void test_category_and_name() {

    CubicSpline_sptr cfn = boost::make_shared<CubicSpline>();

    TS_ASSERT(cfn->category() == "Background");
    TS_ASSERT(cfn->name() == "CubicSpline");
  }

  void testSetNAttribute() {
    // call constructor - does not initialise
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();

    // initialise "n"!
    cspline->initialize();
    // cspline created with 3 attributes
    TS_ASSERT_EQUALS(cspline->getAttribute("n").asInt(), 3);

    // resize to have 10 attributes
    cspline->setAttributeValue("n", 10);

    TS_ASSERT_EQUALS(cspline->getAttribute("n").asInt(), 10);

    // Check that resizing the spline has initialised the attributes/parameters
    for (int i = 0; i < 10; ++i) {
      auto index = std::to_string(i);

      std::string xAttrName = "x" + index;
      std::string yAttrName = "y" + index;

      TS_ASSERT_EQUALS(cspline->getAttribute(xAttrName).asDouble(), i);
      TS_ASSERT_EQUALS(cspline->getParameter(yAttrName), 0);
    }
  }

  void testSetNAttributeBoundary() {
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();

    cspline->initialize();

    // cubic splines must have at least 3 points
    TS_ASSERT_THROWS(cspline->setAttributeValue("n", 2), std::invalid_argument);

    // set the number of points to something sensible
    cspline->setAttributeValue("n", 5);

    // attempt to make it smaller than it already is
    TS_ASSERT_THROWS(cspline->setAttributeValue("n", 4), std::invalid_argument);

    size_t oldAttrN = cspline->nAttributes();

    // attempt to set the attribute to the same value doesn't
    // change anything
    TS_ASSERT_THROWS_NOTHING(cspline->setAttributeValue("n", 5));
    TS_ASSERT_EQUALS(oldAttrN, cspline->nAttributes());
  }

  void testKnownInterpolationValues() {
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();
    cspline->initialize();
    int nData = 10; // number of data points to fit too

    boost::scoped_array<double> x(new double[nData]);
    boost::scoped_array<double> referenceSet(new double[nData]);

    // setup spline with n data points separated by 1
    setupCubicSpline(cspline, nData, 1);

    // generate a set of test points
    generateTestData(nData, referenceSet, x, 1);

    FunctionDomain1DView view(x.get(), nData);
    FunctionValues testDataValues(view);

    cspline->function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < nData; ++i) {
      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }
  }

  void testUnknowntInterpolationValues() {
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();
    cspline->initialize();
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

    cspline->function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(referenceSet[i], testDataValues[i], 1e-4);
    }
  }

  void testCalculateDerivative() {
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();
    cspline->initialize();
    int nData = 10;
    int testDataSize = 10;

    boost::scoped_array<double> x(new double[nData]);
    boost::scoped_array<double> refSet(new double[testDataSize]);
    boost::scoped_array<double> testDataValues(new double[testDataSize]);

    setupCubicSpline(cspline, nData, 1);
    generateDerivTestData(testDataSize, refSet, x, 1, 1);

    cspline->derivative1D(testDataValues.get(), x.get(), nData, 1);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(refSet[i], testDataValues[i], 1e-2);
    }
  }

  void testUnorderedX() {
    CubicSpline_sptr cspline = boost::make_shared<CubicSpline>();
    cspline->initialize();
    int nData = 5;
    int testDataSize = 5;
    boost::scoped_array<double> x(new double[nData]);
    boost::scoped_array<double> refSet(new double[testDataSize]);

    setupCubicSpline(cspline, nData, -0.5);

    // generate descending data with negative xModify
    generateTestData(testDataSize, refSet, x, -0.5);

    FunctionDomain1DView view(x.get(), nData);
    FunctionValues testDataValues(view);

    cspline->function(view, testDataValues);

    // compare reference data with output data
    for (int i = 0; i < testDataSize; ++i) {
      TS_ASSERT_DELTA(refSet[i], testDataValues[i], 1e-4);
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

  void generateDerivTestData(int numTests, boost::scoped_array<double> &refSet,
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
  void setupCubicSpline(CubicSpline_sptr &cspline, int nData, double xModify) {
    cspline->setAttributeValue("n", nData);
    // calculate a reference set to check against
    for (int i = 0; i < nData; ++i) {
      cspline->setAttributeValue("x" + std::to_string(i), i * xModify);
      cspline->setParameter(static_cast<size_t>(i),
                            splineYFunction(i * xModify));
    }
  }
};

#endif /*CubicSplineTEST_H_*/
