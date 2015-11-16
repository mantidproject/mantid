#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/StaticKuboToyabeTimesStretchExp.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::CurveFitting::Functions::StaticKuboToyabeTimesStretchExp;

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::DataObjects;

class StaticKuboToyabeTimesStretchExpTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StaticKuboToyabeTimesStretchExpTest *createSuite() {
    return new StaticKuboToyabeTimesStretchExpTest();
  }
  static void destroySuite(StaticKuboToyabeTimesStretchExpTest *suite) {
    delete suite;
  }

  void getMockData(Mantid::MantidVec &y, Mantid::MantidVec &e) {
    // Calculated with A = 0.24, Delta = 0.06, Lambda = 0.63 and Beta = 0.63 on
    // an Excel spreadsheet
    y[0] = 0.24;
    y[1] = 0.113248409;
    y[2] = 0.074402367;
    y[3] = 0.052183632;
    y[4] = 0.037812471;
    y[5] = 0.027927981;
    y[6] = 0.020873965;
    y[7] = 0.015717258;
    y[8] = 0.011885418;
    y[9] = 0.009005914;
    y[10] = 0.006825573;
    y[11] = 0.005166593;
    y[12] = 0.003900885;
    y[13] = 0.002934321;
    y[14] = 0.002196637;
    y[15] = 0.001634742;
    y[16] = 0.001208136;
    y[17] = 0.000885707;

    for (int i = 0; i < 18; i++)
      e[i] = 1.0;
  }

  StaticKuboToyabeTimesStretchExpTest() : fn() {}

  void test_Initialize() { TS_ASSERT_THROWS_NOTHING(fn.initialize()); }

  void test_Name() {
    TS_ASSERT_EQUALS(fn.name(), "StaticKuboToyabeTimesStretchExp");
  }

  void test_Params() {
    TS_ASSERT_DELTA(fn.getParameter("A"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Delta"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Lambda"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Beta"), 0.2, 0.0001)
  }

  void test_Category() {
    const std::vector<std::string> categories = fn.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Muon");
  }

  void test_AgainstMockData() {
    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    std::string wsName = "SKTTimesStretchExpMockData";
    Workspace_sptr ws =
        WorkspaceFactory::Instance().create("Workspace2D", 1, 18, 18);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);

    for (int i = 0; i < 18; i++)
      ws2D->dataX(0)[i] = i;

    getMockData(ws2D->dataY(0), ws2D->dataE(0));

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    alg2.setPropertyValue("Function", fn.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "0");
    alg2.setPropertyValue("EndX", "17");

    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0001, 0.0001);

    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A"), 0.24, 0.0001);
    TS_ASSERT_DELTA(out->getParameter("Delta"), 0.06, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Lambda"), 0.63, 0.001);
    TS_ASSERT_LESS_THAN(out->getParameter("Beta"), 1.00);

    AnalysisDataService::Instance().remove(wsName);
  }

  StaticKuboToyabeTimesStretchExp fn;
};

#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESSTRETCHEXPTEST_H_ */