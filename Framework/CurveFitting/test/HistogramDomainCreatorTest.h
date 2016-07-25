#ifndef MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/HistogramDomainCreator.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <algorithm>
#include <math.h>

#include "D:/Work/mantid_stuff/Testing/class/MyTestDef.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::CurveFitting::Functions;
using Mantid::CurveFitting::HistogramDomainCreator;
using Mantid::CurveFitting::Algorithms::Fit;

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
    manager.declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");
    // Data points workspace
    auto ws = createTestWorkspace(false);
    manager.setProperty("InputWorkspace", ws);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    TS_ASSERT_THROWS(creator.createDomain(domain, values), std::runtime_error);
  }

  void test_domain_values() {
    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    manager.declareProperty(make_unique<WorkspaceProperty<Workspace>>(
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
    auto &h = dynamic_cast<FunctionDomain1DHistogram&>(*domain);
    auto &x = ws->readX(0);
    auto &y = ws->readY(0);
    for(size_t j = 0; j < 10; ++j) {
      TS_ASSERT_EQUALS(h[j], x[j + 1]);
    }
  }

  void test_Lorentzian() {

    PropertyManager manager;
    HistogramDomainCreator creator(manager, "InputWorkspace");
    creator.declareDatasetProperties();
    manager.declareProperty(make_unique<WorkspaceProperty<Workspace>>(
                                "InputWorkspace", "", Direction::Input),
                            "Name of the input Workspace");

    auto ws = createFitWorkspace(10);
    manager.setProperty("InputWorkspace", ws);
    FunctionDomain_sptr domain;
    FunctionValues_sptr values;
    creator.createDomain(domain, values);

    Lorentzian fun;
    fun.initialize();
    fun.setParameter("FWHM", 1.0);
    fun.function(*domain, *values);

    auto &d1d = dynamic_cast<FunctionDomain1D&>(*domain);
    TS_ASSERT_DELTA(values->getCalculated(0), 0.0302240668, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(1), 0.0433343771, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(2), 0.0640812259, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(3), 0.0936577709, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(4), 0.121118942, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(5), 0.121118942, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(6), 0.0936577709, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(7), 0.0640812259, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(8), 0.0433343771, 1e-9);
    TS_ASSERT_DELTA(values->getCalculated(9), 0.0302240668, 1e-9);
  }

  void test_fit() {
    auto ws = createFitWorkspace(3);
    Fit fit;
    fit.initialize();
    fit.setProperty("Function", "name=Lorentzian,FWHM=0.5");
    fit.setProperty("HistogramFit", true);
    fit.setProperty("InputWorkspace", ws);
    fit.execute();
    IFunction_sptr fun = fit.getProperty("Function");
    for(size_t i = 0; i < fun->nParams(); ++i) {
      std::cerr << i << ' ' << fun->parameterName(i) << ' ' << fun->getParameter(i) << std::endl;
    }
  }

private:
  MatrixWorkspace_sptr createTestWorkspace(const bool histogram) {
    MatrixWorkspace_sptr ws2(new WorkspaceTester);
    size_t ny = 10;
    size_t nx = ny + (histogram ? 1 : 0);
    ws2->initialize(2, nx, ny);

    for (size_t is = 0; is < ws2->getNumberHistograms(); ++is) {
      Mantid::MantidVec &x = ws2->dataX(is);
      Mantid::MantidVec &y = ws2->dataY(is);
      for (size_t i = 0; i < ws2->blocksize(); ++i) {
        x[i] = 0.1 * double(i) + 0.01 * double(is);
        y[i] = (10.0 + double(is)) * exp(-(x[i]) / (0.5 * (1 + double(is))));
      }
      if (histogram)
        x.back() = x[x.size() - 2] + 0.1;
    }
    return ws2;
  }

  MatrixWorkspace_sptr createFitWorkspace(const size_t ny = 10) {
    MatrixWorkspace_sptr ws(new WorkspaceTester);
    size_t nx = ny + 1;
    double x0 = -1.0;
    double x1 = 1.0;
    double dx = (x1 - x0) / ny;
    double gamma = 0.2;
    double A = 10.;
    auto cumulFun = [gamma](double x){return atan(x / gamma) / M_PI;};
    ws->initialize(1, nx, ny);
    Mantid::MantidVec &x = ws->dataX(0);
    Mantid::MantidVec &y = ws->dataY(0);
    Mantid::MantidVec &e = ws->dataE(0);
    x.front() = x0;
    for (size_t i = 0; i < ny; ++i) {
      double xl = x0 + dx * double(i);
      double xr = x0 + dx * double(i + 1);
      x[i + 1] = xr;
      y[i] = cumulFun(xr) - cumulFun(xl);
      e[i] = 1.0;
    }
    return ws;
  }
};

#endif /* MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_ */
