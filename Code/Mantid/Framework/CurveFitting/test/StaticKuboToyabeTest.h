#ifndef STATICKUBOTOYABETEST_H_
#define STATICKUBOTOYABETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/StaticKuboToyabe.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class StaticKuboToyabeTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    // Calculated with A = 0.24 and Delta = 0.16 on an Excel spreadsheet
    y[0] = 0.24;
    y[1] = 0.233921146;
    y[2] = 0.216447929;
    y[3] = 0.189737312;
    y[4] = 0.156970237;
    y[5] = 0.121826185;
    y[6] = 0.08791249;
    y[7] = 0.058260598;
    y[8] = 0.034976545;
    y[9] = 0.019090369;
    y[10] = 0.01060189;
    y[11] = 0.008680652;
    y[12] = 0.011954553;
    y[13] = 0.018817301;
    y[14] = 0.027696749;
    y[15] = 0.037247765;
    y[16] = 0.046457269;
    y[17] = 0.054669182;

    for (int i = 0; i <=17; i++)
    {
      e[i] = 0.01;
    }

  }

  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "StaticKuboToyabeData";
    int histogramNumber = 1;
    int timechannels = 18;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 18; i++) ws2D->dataX(0)[i] = i;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up fitting function
    StaticKuboToyabe fn;
    fn.initialize();

    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",fn.asString());


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","17");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0001,0.0001);

    IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("A"), 0.24 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Delta"), 0.16 ,0.001);

    // check it categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*STATICKUBOTOYABETEST_H_*/
