// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef BSPLINETEST_H_
#define BSPLINETEST_H_

#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/BSpline.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using Mantid::CurveFitting::Functions::BSpline;

class BSplineTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BSplineTest *createSuite() { return new BSplineTest(); }
  static void destroySuite(BSplineTest *suite) { delete suite; }

  void test_category() {
    BSpline cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void test_defaults() {
    BSpline bsp;
    int order = bsp.getAttribute("Order").asInt();
    int nbreak = bsp.getAttribute("NBreak").asInt();
    size_t nparams = bsp.nParams();

    TS_ASSERT_EQUALS(order, 3);
    TS_ASSERT_EQUALS(nbreak, 10);
    TS_ASSERT_EQUALS(nparams, 11);
    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), 0.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), 1.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("Uniform").asBool(), true);
  }

  void test_nonpositive_order() {
    BSpline bsp;
    TS_ASSERT_THROWS(bsp.setAttributeValue("Order", -3), const std::invalid_argument &);
    TS_ASSERT_THROWS(bsp.setAttributeValue("Order", 0), const std::invalid_argument &);
  }

  void test_nbreak_too_small() {
    BSpline bsp;
    TS_ASSERT_THROWS(bsp.setAttributeValue("NBreak", 1), const std::invalid_argument &);
    TS_ASSERT_THROWS(bsp.setAttributeValue("NBreak", 0), const std::invalid_argument &);
    TS_ASSERT_THROWS(bsp.setAttributeValue("NBreak", -3),
                     const std::invalid_argument &);
  }

  void test_set_uniform_break_points() {
    BSpline bsp;
    TS_ASSERT_EQUALS(bsp.getAttribute("Uniform").asBool(), true);
    TS_ASSERT_EQUALS(bsp.getAttribute("NBreak").asInt(), 10);
    bsp.setAttributeValue("StartX", -10.0);
    bsp.setAttributeValue("EndX", 10.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), -10.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), 10.0);

    std::vector<double> breaks = bsp.getAttribute("BreakPoints").asVector();
    TS_ASSERT_EQUALS(breaks.size(), 10);
    TS_ASSERT_EQUALS(bsp.nParams(), 11);

    const double dx = 20.0 / 9;
    for (size_t i = 0; i < 10; ++i) {
      TS_ASSERT_DELTA(-10.0 + static_cast<double>(i) * dx, breaks[i], 1e-14);
      TS_ASSERT_EQUALS(bsp.parameterName(i), "A" + std::to_string(i));
    }
    TS_ASSERT_EQUALS(bsp.parameterName(10), "A10");
  }

  void test_set_nonuniform_break_points() {
    BSpline bsp;
    bsp.setAttributeValue("Uniform", false);
    std::vector<double> inputBreaks(8);
    inputBreaks[0] = 3.0;
    inputBreaks[1] = 4.0;
    inputBreaks[2] = 7.0;
    inputBreaks[3] = 8.0;
    inputBreaks[4] = 15.0;
    inputBreaks[5] = 17.0;
    inputBreaks[6] = 18.0;
    inputBreaks[7] = 30.0;
    bsp.setAttributeValue("BreakPoints", inputBreaks);

    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), 3.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), 30.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("NBreak").asInt(), 8);

    std::vector<double> breaks = bsp.getAttribute("BreakPoints").asVector();
    TS_ASSERT_EQUALS(breaks.size(), 8);
    TS_ASSERT_EQUALS(bsp.nParams(), 9);
    for (size_t i = 0; i < 8; ++i) {
      TS_ASSERT_DELTA(inputBreaks[i], breaks[i], 1e-14);
      TS_ASSERT_EQUALS(bsp.parameterName(i), "A" + std::to_string(i));
    }
    TS_ASSERT_EQUALS(bsp.parameterName(8), "A8");
  }

  void test_try_set_nonuniform_break_points_with_wrong_order() {
    BSpline bsp;
    bsp.setAttributeValue("Uniform", false);
    std::vector<double> inputBreaks(8);
    inputBreaks[0] = 3.0;
    inputBreaks[1] = 4.0;
    inputBreaks[2] = 7.0;
    inputBreaks[3] = 8.0;
    inputBreaks[4] = 15.0;
    inputBreaks[5] = 7.0;
    inputBreaks[6] = 18.0;
    inputBreaks[7] = 30.0;
    TS_ASSERT_THROWS(bsp.setAttributeValue("BreakPoints", inputBreaks),
                     const std::invalid_argument &);
  }

  void test_set_wrong_startx_endx() {
    BSpline bsp;
    TS_ASSERT_EQUALS(bsp.getAttribute("Uniform").asBool(), true);
    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), 0.0);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), 1.0);

    double startx = 10.0;
    double endx = -10.0;

    bsp.setAttributeValue("StartX", startx);
    bsp.setAttributeValue("EndX", endx);

    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), startx);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), endx);

    FunctionDomain1DVector x(startx, endx, 100);
    FunctionValues y(x);

    TS_ASSERT_THROWS(bsp.function(x, y), const std::invalid_argument &);

    startx = 10.0;
    endx = startx;

    bsp.setAttributeValue("StartX", startx);
    bsp.setAttributeValue("EndX", endx);

    TS_ASSERT_EQUALS(bsp.getAttribute("StartX").asDouble(), startx);
    TS_ASSERT_EQUALS(bsp.getAttribute("EndX").asDouble(), endx);

    FunctionDomain1DVector x1(startx, endx, 100);
    FunctionValues y1(x1);

    TS_ASSERT_THROWS(bsp.function(x1, y1), const std::invalid_argument &);
  }

  void test_create_with_function_factory_uniform() {
    auto bsp = FunctionFactory::Instance().createInitialized(
        "name=BSpline,Uniform=true,Order=3,NBreak=3,StartX=0.05,EndX=66.6,"
        "BreakPoints=(0.005,0.5,6.0)");
    TS_ASSERT_EQUALS(bsp->getAttribute("StartX").asDouble(), 0.05);
    TS_ASSERT_EQUALS(bsp->getAttribute("EndX").asDouble(), 66.6);
    TS_ASSERT_EQUALS(bsp->getAttribute("Uniform").asBool(), true);
    TS_ASSERT_EQUALS(bsp->getAttribute("NBreak").asInt(), 3);
    std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
    TS_ASSERT_EQUALS(breaks.size(), 3);
    TS_ASSERT_EQUALS(breaks[0], 0.05);
    TS_ASSERT_DELTA(breaks[1], 33.325, 1e-14);
    TS_ASSERT_EQUALS(breaks[2], 66.6);
  }

  void test_create_with_function_factory_nonuniform() {
    auto bsp = FunctionFactory::Instance().createInitialized(
        "name=BSpline,Uniform=false,Order=3,NBreak=3,StartX=0.05,EndX=66.6,"
        "BreakPoints=(0.005,0.5,6.0)");
    TS_ASSERT_EQUALS(bsp->getAttribute("StartX").asDouble(), 0.005);
    TS_ASSERT_EQUALS(bsp->getAttribute("EndX").asDouble(), 6.0);
    TS_ASSERT_EQUALS(bsp->getAttribute("Uniform").asBool(), false);
    TS_ASSERT_EQUALS(bsp->getAttribute("NBreak").asInt(), 3);
    std::vector<double> breaks = bsp->getAttribute("BreakPoints").asVector();
    TS_ASSERT_EQUALS(breaks.size(), 3);
    TS_ASSERT_EQUALS(breaks[0], 0.005);
    TS_ASSERT_EQUALS(breaks[1], 0.5);
    TS_ASSERT_EQUALS(breaks[2], 6.0);
  }
};

#endif /*BSPLINETEST_H_*/
