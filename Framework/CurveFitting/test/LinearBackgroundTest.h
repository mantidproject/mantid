#ifndef LINEARBACKGROUNDTEST_H_
#define LINEARBACKGROUNDTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/LinearBackground.h"

#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/Exception.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::LinearBackground;
using Mantid::CurveFitting::Fit;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class LinearBackgroundTest : public CxxTest::TestSuite {
public:
  void testAgainstHRPDData() {
    // create mock data to test against
    std::string wsName = "LinearBackgroundTest";
    int histogramNumber = 1;
    int timechannels = 5;
    Workspace_sptr ws = WorkspaceFactory::Instance().create(
        "Workspace2D", histogramNumber, timechannels, timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < timechannels; i++) {
      ws2D->dataX(0)[i] = i + 1;
      ws2D->dataY(0)[i] = i + 1;
      ws2D->dataE(0)[i] = 1.0;
    }

    // put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT(alg2.isInitialized());

    // set up fitting function
    IFunction_sptr linB(new LinearBackground());
    linB->initialize();

    linB->setParameter("A0", 1.0);

    // alg2.setFunction(linB);
    alg2.setProperty("Function", linB);

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
    TS_ASSERT_DELTA(out->getParameter("A1"), 1.0, 0.0003);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Background");
  }

  void testForCategories() {
    LinearBackground forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT(categories.size() == 1);
    TS_ASSERT(categories[0] == "Background");
  }
};

#endif /*LINEARBACKGROUNDTEST_H_*/
