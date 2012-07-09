#ifndef ENDERFCTEST_H_
#define ENDERFCTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/EndErfc.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;


class EndErfcTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    // Based on a curve got for Calibration a MERLIN tube.
    y[0] = 1.0;
    y[1] = 3.0;
    y[2] = 4.0;
    y[3] = 28.0;
    y[4] = 221.0;
    y[5] = 872.0;
    y[6] = 1495.0;
    y[7] = 1832.0;
    y[8] = 1830.0;
    y[9] = 1917.0;
    y[10] = 2045.0;
    y[11] = 1996.0;


    for (int i = 0; i <=12; i++)
    {
      e[i] = 1.0;
    }

  }

  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "EndErcfMockData";
    int histogramNumber = 1;
    int timechannels = 13;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 13; i++) ws2D->dataX(0)[i] = 5*i;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up fitting function
    EndErfc fn;
    fn.initialize();

    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",fn.asString());


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","5");
    alg2.setPropertyValue("EndX","55");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0001,20000);

    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA( out->getParameter("A"), 1000 ,30.0);
    TS_ASSERT_DELTA( out->getParameter("B"), 26 ,0.1);
    TS_ASSERT_DELTA( out->getParameter("C"), 7.7 ,0.1);
    TS_ASSERT_DELTA( out->getParameter("D"), 0 ,0.1);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    // TS_ASSERT( categories[0] == "General" ); inteded to be "General"

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*ENDERFCTEST_H_*/
