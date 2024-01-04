// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Convolution.h"
#include "MantidCurveFitting/Functions/DeltaFunction.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::CurveFitting::Functions;

// same class as ConvolutionTest_Gauss in ConvolutionTest.h
class DeltaFunctionTest_Gauss : public IPeakFunction {
public:
  DeltaFunctionTest_Gauss() {
    declareParameter("c");     // center of the peak
    declareParameter("h", 1.); // height of the peak
    declareParameter("s", 1.); // 1/(2*sigma^2)
  }

  std::string name() const override { return "DeltaFunctionTest_Gauss"; }

  void functionLocal(double *out, const double *xValues, const size_t nData) const override {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues, const size_t nData) override {
    // throw Mantid::Kernel::Exception::NotImplementedError("");
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      double e = h * exp(-x * x * w);
      out->set(i, 0, x * h * e * w);
      out->set(i, 1, e);
      out->set(i, 2, -x * x * h * e);
    }
  }

  double centre() const override { return getParameter(0); }

  double height() const override { return getParameter(1); }

  double fwhm() const override { return getParameter(2); }

  void setCentre(const double c) override { setParameter(0, c); }
  void setHeight(const double h) override { setParameter(1, h); }

  void setFwhm(const double w) override { setParameter(2, w); }

}; // end of DeltaFunctionTest_Gauss

// A derived class from DeltaFunction
class DeltaFunctionTest_Delta : public DeltaFunction {
public:
  DeltaFunctionTest_Delta() {
    declareParameter("p1");
    declareParameter("p2");
  }

  double HeightPrefactor() const override {
    const double &p1 = getParameter("p1");
    const double &p2 = getParameter("p2");
    return p1 * p2; // simple operation
  }

  std::string name() const override { return "DeltaFunctionTest_Delta"; }

  // Expose protected function so we can test it.
  void callFunctionDeriv1D() { functionDeriv1D(nullptr, nullptr, 0); }

}; // end of DeltaFunctionTest_Delta

class DeltaFunctionTest : public CxxTest::TestSuite {
public:
  void test_category() {
    DeltaFunction fn;
    TS_ASSERT_EQUALS(fn.category(), "Peak");
  }

  void testDeltaFunction() {
    Convolution conv;
    // set the resolution function
    double h = 3.0; // height
    double a = 1.3; // 1/(2*sigma^2)
    auto res = IFunction_sptr(new DeltaFunctionTest_Gauss());
    res->setParameter("c", 0);
    res->setParameter("h", h);
    res->setParameter("s", a);
    conv.addFunction(res);

    // set the "structure factor"
    double H = 1.5, p1 = 2.6, p2 = 0.7;
    auto eds = IFunction_sptr(new DeltaFunctionTest_Delta());
    eds->setParameter("Height", H);
    eds->setParameter("p1", p1);
    eds->setParameter("p2", p2);
    conv.addFunction(eds);

    // set up some frequency values centered around zero
    const int N = 117;
    double w[N], w0, dw = 0.13;
    w0 = -dw * int(N / 2);
    for (int i = 0; i < N; i++)
      w[i] = w0 + i * dw;

    FunctionDomain1DView wView(&w[0], N);
    FunctionValues out(wView);
    // convolve. The result must be the resolution multiplied by factor
    // H*p1*p2);
    conv.function(wView, out);
    for (int i = 0; i < N; i++) {
      TS_ASSERT_DELTA(out.getCalculated(i), H * p1 * p2 * h * exp(-w[i] * w[i] * a), 1e-10);
    }
  }

  void test_delta_with_shift() {
    auto res = IPeakFunction_sptr(new DeltaFunctionTest_Gauss());
    double a = 0.13;
    double ha = 1.0 / sqrt(M_PI * a);
    res->setParameter("c", 0);
    res->setParameter("h", ha);
    res->setParameter("s", 1. / a);

    auto gauss = IPeakFunction_sptr(new DeltaFunctionTest_Gauss());
    double h = 3.0;
    double b = 3.0;
    gauss->setParameter("c", 0);
    gauss->setParameter("h", h);
    gauss->setParameter("s", 1. / b);

    auto delta = IPeakFunction_sptr(new DeltaFunction());
    double shift = 0.1;
    double scale = 0.3;
    delta->setParameter("Centre", shift);
    delta->setParameter("Height", scale);

    Convolution conv;
    conv.addFunction(res);
    conv.addFunction(gauss);
    conv.addFunction(delta);

    FunctionDomain1DVector x(-6, 6, 100);
    FunctionValues y(x);
    conv.function(x, y);

    double hh = h * sqrt(b / (a + b));
    double bb = a + b;
    for (size_t i = 10; i < x.size() - 10; ++i) {
      auto xx = x[i];
      auto xxx = xx - shift;
      auto d = y[i] - hh * exp(-xx * xx / bb) - scale * ha * exp(-xxx * xxx / a);
      TS_ASSERT_DELTA(d, 0.0, 1e-11);
    }
  }

  void test_two_deltas_with_shifts() {
    auto res = IPeakFunction_sptr(new DeltaFunctionTest_Gauss());
    double a = 0.13;
    double ha = 1.0 / sqrt(M_PI * a);
    res->setParameter("c", 0);
    res->setParameter("h", ha);
    res->setParameter("s", 1. / a);

    auto gauss = IPeakFunction_sptr(new DeltaFunctionTest_Gauss());
    double h = 3.0;
    double b = 3.0;
    gauss->setParameter("c", 0);
    gauss->setParameter("h", h);
    gauss->setParameter("s", 1. / b);

    auto delta1 = IPeakFunction_sptr(new DeltaFunction());
    double shift1 = 2;
    double scale1 = 0.3;
    delta1->setParameter("Centre", shift1);
    delta1->setParameter("Height", scale1);

    auto delta2 = IPeakFunction_sptr(new DeltaFunction());
    double shift2 = -2;
    double scale2 = 1.3;
    delta2->setParameter("Centre", shift2);
    delta2->setParameter("Height", scale2);

    Convolution conv;
    conv.addFunction(res);
    conv.addFunction(gauss);
    conv.addFunction(delta1);
    conv.addFunction(delta2);

    FunctionDomain1DVector x(-6, 6, 100);
    FunctionValues y(x);
    conv.function(x, y);

    double hh = h * sqrt(b / (a + b));
    double bb = a + b;
    for (size_t i = 10; i < x.size() - 10; ++i) {
      auto xx = x[i];
      auto xx1 = xx - shift1;
      auto xx2 = xx - shift2;
      auto d = y[i] - hh * exp(-xx * xx / bb) - scale1 * ha * exp(-xx1 * xx1 / a) - scale2 * ha * exp(-xx2 * xx2 / a);
      TS_ASSERT_DELTA(d, 0.0, 1e-11);
    }
  }

  void testThrowsWhenCallingFunctionDeriv1D() {
    DeltaFunctionTest_Delta delta;
    TS_ASSERT_THROWS_EQUALS(delta.callFunctionDeriv1D(), const std::runtime_error &e, std::string(e.what()),
                            "Cannot compute derivative of a delta function");
  }

}; // end of DeltaFunctionTest
