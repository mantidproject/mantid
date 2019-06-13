// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/HistogramDomainCreator.h"

#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidCurveFitting/GSLJacobian.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <algorithm>
#include <math.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::CurveFitting::Functions;
using Mantid::CurveFitting::Algorithms::Fit;
using Mantid::CurveFitting::GSLJacobian;
using Mantid::CurveFitting::HistogramDomainCreator;

class HistogramDomainCreatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramDomainCreatorTest *createSuite() {
    return new HistogramDomainCreatorTest();
  }
  static void destroySuite(HistogramDomainCreatorTest *suite) { delete suite; }

  void test_declared_properties() {
    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    TS_ASSERT(manager.existsProperty("WorkspaceIndex"));
    TS_ASSERT(manager.existsProperty("StartX"));
    TS_ASSERT(manager.existsProperty("EndX"));

    creator.declareDatasetProperties("_suffix");
    TS_ASSERT(manager.existsProperty("WorkspaceIndex_suffix"));
    TS_ASSERT(manager.existsProperty("StartX_suffix"));
    TS_ASSERT(manager.existsProperty("EndX_suffix"));
  }

  void test_point_data_not_allowed() {
    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    // Data points workspace
    auto ws = createTestWorkspace(false);
    manager.setProperty("InputWorkspace", ws);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    TS_ASSERT_THROWS(creator.createDomain(domain, values),
                     const std::runtime_error &);
  }

  void test_domain_values() {
    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    // Histogram workspace
    auto ws = createTestWorkspace(true);
    manager.setProperty("InputWorkspace", ws);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    TS_ASSERT_THROWS_NOTHING(creator.createDomain(domain, values));
    TS_ASSERT_EQUALS(domain->size(), 10);
    TS_ASSERT_EQUALS(values->size(), 10);
    TS_ASSERT_EQUALS(ws->blocksize(), 10);
    auto &h = dynamic_cast<FunctionDomain1DHistogram &>(*domain);
    auto &x = ws->x(0);
    for (size_t j = 0; j < 10; ++j) {
      TS_ASSERT_EQUALS(h[j], x[j + 1]);
    }
    TS_ASSERT_EQUALS(h.leftBoundary(), x[0]);
  }

  void test_Lorentzian() {

    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    manager.declareProperty(std::make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");

    auto ws = createLorentzWorkspace(10);
    manager.setProperty("InputWorkspace", ws);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);

    Lorentzian fun;
    fun.initialize();
    fun.setParameter("Amplitude", 2.1);
    fun.setParameter("FWHM", 1.0);
    fun.function(*domain, *values);

    TS_ASSERT_DELTA(values->getCalculated(0), 2.1 * 0.0302240668, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(1), 2.1 * 0.0433343771, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(2), 2.1 * 0.0640812259, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(3), 2.1 * 0.0936577709, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(4), 2.1 * 0.121118942, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(5), 2.1 * 0.121118942, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(6), 2.1 * 0.0936577709, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(7), 2.1 * 0.0640812259, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(8), 2.1 * 0.0433343771, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(9), 2.1 * 0.0302240668, 1e-9);

    GSLJacobian jacobian(fun, 10);
    fun.functionDeriv(*domain, jacobian);

    FunctionValues values1(*domain);
    double dp = 1e-9;
    fun.setParameter("Amplitude", 2.1 + dp);
    fun.function(*domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(
          jacobian.get(i, 0),
          (values1.getCalculated(i) - values->getCalculated(i)) / dp, 1e-5);
    }

    fun.setParameter("Amplitude", 2.1);
    fun.setParameter("PeakCentre", dp);
    fun.function(*domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(
          jacobian.get(i, 1),
          (values1.getCalculated(i) - values->getCalculated(i)) / dp, 1e-5);
    }

    fun.setParameter("PeakCentre", 0.0);
    fun.setParameter("FWHM", 1.0 + dp);
    fun.function(*domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(
          jacobian.get(i, 2),
          (values1.getCalculated(i) - values->getCalculated(i)) / dp, 1e-5);
    }
  }

  void test_Lorentzian_integral() {
    Lorentzian fun;
    fun.initialize();
    fun.setParameter("Amplitude", 1.0);
    fun.setParameter("FWHM", 1.0);

    FunctionDomain1DHistogram domain({-10000.0, 10000.0});
    FunctionValues values(domain);

    fun.function(domain, values);
    TS_ASSERT_DELTA(fun.intensity(), 1.0, 1e-15);
    TS_ASSERT_DELTA(values[0], 1.0, 1e-4);
  }

  void test_fit() {
    auto ws = createLorentzWorkspace(3);
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Lorentzian,FWHM=0.5");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("Amplitude"), 1.0, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("PeakCentre"), 0.0, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("FWHM"), 0.4, 1e-5);

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "fit_Workspace");
    TS_ASSERT(outWS);

    auto &y = outWS->y(0);
    auto &f = outWS->y(1);
    auto &d = outWS->y(2);
    for (size_t i = 0; i < y.size(); ++i) {
      TS_ASSERT_DELTA(y[i], f[i], 1e-5);
      TS_ASSERT_DELTA(d[i], 0.0, 1e-5);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_Gaussian() {

    FunctionDomain1DHistogram domain(
        {-1.0, -0.8, -0.6, -0.4, -0.2, 0.0, 0.2, 0.4, 0.6, 0.8, 1.0});
    FunctionValues values(domain);

    Gaussian fun;
    fun.initialize();
    fun.setParameter("Height", 2.1);
    fun.setParameter("Sigma", 0.3);
    fun.function(domain, values);

    TS_ASSERT_DELTA(values.getCalculated(0), 0.00537128264648, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(1), 0.0298776137685, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(2), 0.108112093951, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(3), 0.254691556195, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(4), 0.390857798247, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(5), 0.390857798247, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(6), 0.254691556195, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(7), 0.108112093951, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(8), 0.0298776137685, 1e-9);
    TS_ASSERT_DELTA(values.getCalculated(9), 0.00537128264648, 1e-9);

    GSLJacobian jacobian(fun, 10);
    fun.functionDeriv(domain, jacobian);

    FunctionValues values1(domain);
    double dp = 1e-9;
    fun.setParameter("Height", 2.1 + dp);
    fun.function(domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(jacobian.get(i, 0),
                      (values1.getCalculated(i) - values.getCalculated(i)) / dp,
                      1e-5);
    }

    fun.setParameter("Height", 2.1);
    fun.setParameter("PeakCentre", dp);
    fun.function(domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(jacobian.get(i, 1),
                      (values1.getCalculated(i) - values.getCalculated(i)) / dp,
                      1e-5);
    }

    fun.setParameter("PeakCentre", 0.0);
    auto oldPar = fun.activeParameter(2);
    fun.setActiveParameter(2, oldPar + dp);
    fun.function(domain, values1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_DELTA(jacobian.get(i, 2),
                      (values1.getCalculated(i) - values.getCalculated(i)) / dp,
                      1e-5);
    }
  }

  void test_Gaussian_integral() {
    Gaussian fun;
    fun.initialize();
    double sigma = 0.2;
    double a = 1.3;
    fun.setParameter("Sigma", sigma);
    fun.setIntensity(a);

    {
      FunctionDomain1DHistogram domain({-10.0, 10.0});
      FunctionValues values(domain);
      fun.function(domain, values);
      TS_ASSERT_DELTA(fun.intensity(), a, 1e-15);
      TS_ASSERT_DELTA(values[0], a, 1e-15);
    }
    { // 1-sigma
      FunctionDomain1DHistogram domain({-sigma, sigma});
      FunctionValues values(domain);
      fun.function(domain, values);
      TS_ASSERT_DELTA(values[0], 0.6826 * a, 1e-3);
    }
    { // 2-sigma
      FunctionDomain1DHistogram domain({-2.0 * sigma, 2.0 * sigma});
      FunctionValues values(domain);
      fun.function(domain, values);
      TS_ASSERT_DELTA(values[0], 0.9544 * a, 1e-3);
    }
    { // 3-sigma
      FunctionDomain1DHistogram domain({-3.0 * sigma, 3.0 * sigma});
      FunctionValues values(domain);
      fun.function(domain, values);
      TS_ASSERT_DELTA(values[0], 0.9973 * a, 1e-3);
    }
  }

  void test_fit_Gaussian() {
    auto ws = createGaussWorkspace(3);
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Gaussian,Height=1,Sigma=0.5");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("Height"), 1.0 / 0.2 / sqrt(2.0 * M_PI),
                    1e-5);
    TS_ASSERT_DELTA(fun->getParameter("PeakCentre"), 0.0, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("Sigma"), 0.2, 1e-5);

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "fit_Workspace");
    TS_ASSERT(outWS);

    auto &y = outWS->y(0);
    auto &f = outWS->y(1);
    auto &d = outWS->y(2);
    for (size_t i = 0; i < y.size(); ++i) {
      TS_ASSERT_DELTA(y[i], f[i], 1e-5);
      TS_ASSERT_DELTA(d[i], 0.0, 1e-5);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_fit_Flat() {
    auto ws = createFlatWorkspace();
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("A0"), 3.1, 1e-5);

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "fit_Workspace");
    TS_ASSERT(outWS);

    auto &y = outWS->y(0);
    auto &f = outWS->y(1);
    auto &d = outWS->y(2);
    for (size_t i = 0; i < y.size(); ++i) {
      TS_ASSERT_DELTA(y[i], f[i], 1e-5);
      TS_ASSERT_DELTA(d[i], 0.0, 1e-5);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_fit_Linear() {
    auto ws = createLinearWorkspace();
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=LinearBackground");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("A0"), 3.1, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("A1"), 0.3, 1e-5);

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "fit_Workspace");
    TS_ASSERT(outWS);

    auto &y = outWS->y(0);
    auto &f = outWS->y(1);
    auto &d = outWS->y(2);
    for (size_t i = 0; i < y.size(); ++i) {
      TS_ASSERT_DELTA(y[i], f[i], 1e-5);
      TS_ASSERT_DELTA(d[i], 0.0, 1e-5);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_fit_GaussLinear() {
    auto ws = createGaussLinearWorkspace();
    Fit fit;
    fit.initialize();
    fit.setProperty("Function",
                    "name=LinearBackground;name=Gaussian,Height=1,Sigma=0.3");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("f0.A0"), 3.1, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("f0.A1"), 0.3, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("f1.Height"),
                    1.0 / 0.2 / sqrt(2.0 * M_PI), 1e-4);
    TS_ASSERT_DELTA(fun->getParameter("f1.PeakCentre"), 0.0, 1e-5);
    TS_ASSERT_DELTA(fun->getParameter("f1.Sigma"), 0.2, 1e-5);

    auto outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
        "fit_Workspace");
    TS_ASSERT(outWS);

    auto &y = outWS->y(0);
    auto &f = outWS->y(1);
    auto &d = outWS->y(2);

    for (size_t i = 0; i < y.size(); ++i) {
      TS_ASSERT_DELTA(y[i], f[i], 1e-5);
      TS_ASSERT_DELTA(d[i], 0.0, 1e-5);
    }

    AnalysisDataService::Instance().clear();
  }

  void test_distribution() {
    auto ws = createFlatWorkspace();
    auto alg = AlgorithmFactory::Instance().create("ConvertToDistribution", -1);
    alg->setChild(true);
    alg->initialize();
    alg->setProperty("Workspace", ws);
    alg->execute();

    TS_ASSERT(ws->isDistribution());

    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=FlatBackground");
    fit.setProperty("EvaluationType", "Histogram");
    fit.setProperty("InputWorkspace", ws);
    fit.setProperty("Output", "fit");
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");

    TS_ASSERT_DELTA(fun->getParameter("A0"), 3.1, 1e-5);

    AnalysisDataService::Instance().clear();
  }

private:
  MatrixWorkspace_sptr createTestWorkspace(const bool histogram) {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    size_t ny = 10;
    size_t nx = ny + (histogram ? 1 : 0);
    ws2->initialize(2, nx, ny);

    for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {
      auto &x = ws2->mutableX(is);
      auto &y = ws2->mutableY(is);
      const double is_d = static_cast<double>(is);
      for (size_t i = 0; i < y.size(); ++i) {
        x[i] = 0.1 * static_cast<double>(i) + 0.01 * is_d;
        y[i] = (10.0 + is_d) * exp(-(x[i]) / (0.5 * (1. + is_d)));
      }
      if (histogram)
        x.back() = x[x.size() - 2] + 0.1;
    }
    return ws2;
  }

  MatrixWorkspace_sptr createFitWorkspace(const size_t ny,
                                          std::function<double(double)> fun) {
    MatrixWorkspace_sptr ws(new WorkspaceTester);
    size_t nx = ny + 1;
    double x0 = -1.0;
    double x1 = 1.0;
    double dx = (x1 - x0) / static_cast<double>(ny);
    ws->initialize(1, nx, ny);
    auto &x = ws->mutableX(0);
    auto &y = ws->mutableY(0);
    auto &e = ws->mutableE(0);
    x.front() = x0;
    for (size_t i = 0; i < ny; ++i) {
      double xl = x0 + dx * double(i);
      double xr = x0 + dx * double(i + 1);
      x[i + 1] = xr;
      y[i] = fun(xr) - fun(xl);
      e[i] = 1.0;
    }
    return ws;
  }

  MatrixWorkspace_sptr createLorentzWorkspace(const size_t ny = 10) {
    double gamma = 0.2;
    auto cumulFun = [gamma](double x) { return atan(x / gamma) / M_PI; };
    return createFitWorkspace(ny, cumulFun);
  }

  MatrixWorkspace_sptr createGaussWorkspace(const size_t ny = 10) {
    double sigma = 0.2;
    auto cumulFun = [sigma](double x) {
      return 0.5 * erf(x / sigma / sqrt(2.0));
    };
    return createFitWorkspace(ny, cumulFun);
  }

  MatrixWorkspace_sptr createFlatWorkspace(const size_t ny = 10) {
    double a = 3.1;
    auto cumulFun = [a](double x) { return a * x; };
    return createFitWorkspace(ny, cumulFun);
  }

  MatrixWorkspace_sptr createLinearWorkspace(const size_t ny = 10) {
    double a0 = 3.1;
    double a1 = 0.3;
    auto cumulFun = [a0, a1](double x) { return (a0 + 0.5 * a1 * x) * x; };
    return createFitWorkspace(ny, cumulFun);
  }

  MatrixWorkspace_sptr createGaussLinearWorkspace(const size_t ny = 10) {
    double a0 = 3.1;
    double a1 = 0.3;
    double sigma = 0.2;
    auto cumulFun = [a0, a1, sigma](double x) {
      return (a0 + 0.5 * a1 * x) * x + 0.5 * erf(x / sigma / sqrt(2.0));
    };
    return createFitWorkspace(ny, cumulFun);
  }
};

#endif /* MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_ */
