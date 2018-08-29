#ifndef MUONFINTERACTIONTEST_H_
#define MUONFINTERACTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidCurveFitting/Algorithms/Fit.h"
#include "MantidCurveFitting/Functions/MuonFInteraction.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::Algorithms;
using namespace Mantid::DataObjects;

class MuonFInteractionTest : public CxxTest::TestSuite {
public:
  void getMockData(Mantid::MantidVec &y, Mantid::MantidVec &e) {
    // Mock data got from an Excel spreadsheet with
    // Lambda = 0.16, Omega = 0.4, Beta = 1.2 &  A = 1.5

    y[0] = 1.5;
    y[1] = 1.141313628;
    y[2] = 0.591838582;
    y[3] = 0.217069719;
    y[4] = 0.143355934;
    y[5] = 0.256915274;
    y[6] = 0.365739273;
    y[7] = 0.360727646;
    y[8] = 0.260023319;
    y[9] = 0.146136639;
    y[10] = 0.080853314;
    y[11] = 0.068393706;
    y[12] = 0.075537727;
    y[13] = 0.071800717;
    y[14] = 0.051659705;
    y[15] = 0.028746883;
    y[16] = 0.017073081;
    y[17] = 0.018710399;
    y[18] = 0.025298535;
    y[19] = 0.027436201;

    for (int i = 0; i <= 20; i++) {
      e[i] = 0.01;
    }
  }

  void testAgainstMockData() {
    Algorithms::Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    std::string wsName = "MuonFInteractionMockData";
    int histogramNumber = 1;
    int timechannels = 21;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 21; i++)
      ws2D->dataX(0)[i] = i;
    Mantid::MantidVec &y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec &e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up Lorentzian fitting function
    MuonFInteraction fn;
    fn.initialize();

    // alg2.setFunction(fn);
    alg2.setPropertyValue("Function", fn.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "0");
    alg2.setPropertyValue("EndX", "19");

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0001, 0.0001);

    auto out = FunctionFactory::Instance().createInitialized(
        alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA(out->getParameter("Lambda"), 0.16, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Omega"), 0.4, 0.001);
    TS_ASSERT_DELTA(out->getParameter("Beta"), 1.2, 0.01);
    TS_ASSERT_DELTA(out->getParameter("A"), 1.5, 0.01);

    // check it categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Muon");

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /*MUONFINTERACTONTEST_H_*/
