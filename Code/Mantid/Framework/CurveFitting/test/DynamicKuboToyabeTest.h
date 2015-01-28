#ifndef DYNAMICKUBOTOYABETEST_H_
#define DYNAMICKUBOTOYABETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/DynamicKuboToyabe.h"
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

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;


class DynamicKuboToyabeTest : public CxxTest::TestSuite
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

    for (int i = 0; i <10; i++)
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
    std::string wsName = "DynamicKuboToyabeData";
    int histogramNumber = 1;
    int timechannels = 10;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 10; i++) ws2D->dataX(0)[i] = i;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up fitting function
//    DynamicKuboToyabe fn;
    std::string fnString = "name=DynamicKuboToyabe,ties=(Field=0,Nu=0);";
    IFunction_sptr fn = FunctionFactory::Instance().createInitialized(fnString);

    //alg2.setFunction(fn);
    TS_ASSERT_THROWS_NOTHING(alg2.setProperty("Function",fnString));

    // Set which spectrum to fit against and initial starting values
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("InputWorkspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("WorkspaceIndex","0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("StartX","0"));
    TS_ASSERT_THROWS_NOTHING(alg2.setPropertyValue("EndX","17"));

    fn->applyTies();
    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    auto out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("Asym"),  0.238 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Delta"), 0.157 ,0.001);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*DYNAMICKUBOTOYABETEST_H_*/
