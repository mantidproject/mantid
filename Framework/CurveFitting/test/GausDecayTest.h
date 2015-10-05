#ifndef GAUSDECAYTEST_H_
#define GAUSDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/GausDecay.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"

#include "MantidDataHandling/SaveNexus.h" // Debug

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class GausDecayTest : public CxxTest::TestSuite {
public:
  void getMockData(Mantid::MantidVec &y, Mantid::MantidVec &e) {
    y[0] = 0.01;
    y[1] = 0.16;
    y[2] = 1.2;
    y[3] = 5.6;
    y[4] = 18.2;
    y[5] = 43.68;
    y[6] = 80.08;
    y[7] = 114.4;
    y[8] = 128.7;
    y[9] = 114.4;
    y[10] = 80.08;
    y[11] = 43.68;
    y[12] = 18.2;
    y[13] = 5.6;
    y[14] = 1.2;
    y[15] = 0.16;
    y[16] = 0.01;
    y[17] = 0.00;

    for (int i = 0; i <= 17; i++) {
      e[i] = 1.0;
    }
  }

  void testAgainstMockData() {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // create mock data to test against
    std::string wsName = "GausDecayMockData";
    int histogramNumber = 1;
    int timechannels = 18;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 18; i++)
      ws2D->dataX(0)[i] = i - 8.0;
    Mantid::MantidVec &y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec &e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(
        AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up fitting function
    IFunction_sptr fn(new GausDecay());
    fn->initialize();

    alg2.setProperty("Function", fn);

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "-8");
    alg2.setPropertyValue("EndX", "8");

    alg2.setPropertyValue("Output", "OutputGausDecay"); // debug

    // execute fit
    TS_ASSERT_THROWS_NOTHING(TS_ASSERT(alg2.execute()))

    TS_ASSERT(alg2.isExecuted());

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA(dummy, 0.0, 1.0);

    // test the output from fit is what you expect
    IFunction_sptr out = alg2.getProperty("Function");
    // TS_ASSERT_DELTA( out->getParameter("A"), 128.7 ,0.5);
    // TS_ASSERT_DELTA( out->getParameter("Sigma"), 0.35 ,0.05);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Muon");

    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif /*GAUSDECAYTEST_H_*/
