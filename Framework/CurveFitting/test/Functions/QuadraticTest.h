#ifndef QUADRATICTEST_H_
#define QUADRATICTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Functions/Quadratic.h"

#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::Functions::Quadratic;
using Mantid::CurveFitting::Algorithms::Fit;
using namespace Mantid::DataObjects;

class QuadraticTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static QuadraticTest *createSuite() { return new QuadraticTest(); }
  static void destroySuite(QuadraticTest *suite) { delete suite; }

  void test_category() {
    Quadratic cfn;
    cfn.initialize();

    std::vector<std::string> cats;
    TS_ASSERT_THROWS_NOTHING(cats = cfn.categories());
    TS_ASSERT_LESS_THAN_EQUALS(1, cats.size());
    TS_ASSERT_EQUALS(cats.front(), "Background");
    // This would enfonce one and only one category:
    // TS_ASSERT(cfn.category() == "Background");
  }

  void testAgainstHRPDData() {
    // create mock data to test against
    std::string wsName = "quadraticTest";
    int histogramNumber = 1;
    int timechannels = 5;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < timechannels; i++) {
      ws2D->dataX(0)[i] = i + 1;
      ws2D->dataY(0)[i] = (i + 1) * (i + 1);
      ws2D->dataE(0)[i] = 1.0;
    }

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // set up gaussian fitting function
    Quadratic quad;
    quad.initialize();

    quad.setParameter("A0", 1.0);

    // alg2.setFunction(quad);
    alg2.setPropertyValue("Function", quad.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0, 0.1);

    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA(out->getParameter("A0"), 0.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A1"), 0.0, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A2"), 1.0, 0.0001);
  }
};

#endif /*QUADRATICTEST_H_*/
