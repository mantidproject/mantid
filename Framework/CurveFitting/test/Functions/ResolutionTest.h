// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef RESOLUTIONTEST_H_
#define RESOLUTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IPeakFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Functions/Resolution.h"
#include <Poco/File.h>

#include <fstream>

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class ResolutionTest_Gauss : public IPeakFunction {
public:
  ResolutionTest_Gauss() {
    declareParameter("c");
    declareParameter("h", 1.);
    declareParameter("s", 1.);
  }

  std::string name() const override { return "ResolutionTest_Gauss"; }

  void functionLocal(double *out, const double *xValues,
                     const size_t nData) const override {
    double c = getParameter("c");
    double h = getParameter("h");
    double w = getParameter("s");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i] - c;
      out[i] = h * exp(-x * x * w);
    }
  }
  void functionDerivLocal(Jacobian *out, const double *xValues,
                          const size_t nData) override {
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
};

class ResolutionTest_Jacobian : public Jacobian {
public:
  void set(size_t, size_t, double) override {
    throw std::runtime_error("Set method shouldn't be called.");
  }

  double get(size_t, size_t) override {
    throw std::runtime_error("Get method shouldn't be called.");
  }
  void zero() override {
    throw std::runtime_error("Zero method shouldn't be called.");
  }
};

DECLARE_FUNCTION(ResolutionTest_Gauss)

class ResolutionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ResolutionTest *createSuite() { return new ResolutionTest(); }
  static void destroySuite(ResolutionTest *suite) { delete suite; }

  ResolutionTest()
      : resH(3), resS(acos(0.)), N(117), DX(10), X0(-DX / 2), dX(DX / (N - 1)),
        yErr(0), resFileName("ResolutionTestResolution.res") {}

  void setUp() override {
    std::ofstream fil(resFileName.c_str());

    double y0 = 0;
    for (int i = 0; i < N; i++) {
      double x = X0 + i * dX;
      double y = resH * exp(-x * x * resS);
      double err = fabs(y - y0) / 10;
      if (err > yErr)
        yErr = err;
      fil << x << ' ' << y << " 0\n";
      y0 = y;
    }
  }

  void tearDown() override {
    Poco::File phandle(resFileName);
    if (phandle.exists()) {
      phandle.remove();
    }
  }

  void testIt() {
    Resolution res;
    res.setAttributeValue("FileName", resFileName);
    const int n = 50;
    double x[n];
    double y[n];
    double xStart = -2;
    double xEnd = 3.;
    double dx = (xEnd - xStart) / (n - 1);
    for (int i = 0; i < n; i++) {
      x[i] = xStart + dx * i;
    }
    res.function1D(y, x, n);
    for (int i = 0; i < n; i++) {
      double xi = x[i];
      TS_ASSERT_DELTA(y[i], resH * exp(-xi * xi * resS), yErr);
    }
  }

  void testForCategories() {
    Resolution forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "General");
  }

  void test_derivatives_not_calculated() {
    auto ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 10, 10);
    AnalysisDataService::Instance().add("ResolutionTest_WS", ws);
    Resolution res;
    res.setAttributeValue("Workspace", "ResolutionTest_WS");
    std::vector<double> x(10);
    ResolutionTest_Jacobian jacobian;
    TS_ASSERT_THROWS_NOTHING(res.functionDeriv1D(&jacobian, x.data(), 10));
    AnalysisDataService::Instance().clear();
  }

private:
  const double resH, resS;
  const int N;
  const double DX, X0, dX;
  double yErr;
  const std::string resFileName;
};

#endif /*RESOLUTIONTEST_H_*/
