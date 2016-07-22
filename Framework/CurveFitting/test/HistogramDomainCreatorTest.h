#ifndef MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_
#define MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/HistogramDomainCreator.h"

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidCurveFitting/Functions/Lorentzian.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/FakeObjects.h"

#include <algorithm>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::CurveFitting::Functions;
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
};

#endif /* MANTID_CURVEFITTING_HISTOGRAMDOMAINCREATORTEST_H_ */
