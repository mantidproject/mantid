#ifndef EXPDECAYOSCTEST_H_
#define EXPDECAYOSCTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidCurveFitting/ExpDecayOsc.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidCurveFitting/BoundaryConstraint.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;


class ExpDecayOscTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    const double sqrh = 0.70710678; // cos( 45 degrees )
    
    y[0] = 5*sqrh;
    y[1] = 0.0;
    y[2] = -2.567085595163*sqrh;
    y[3] = -1.839397205857;
    y[4] = -1.317985690579*sqrh;
    y[5] = 0.0;
    y[6] = 0.6766764161831*sqrh;
    y[7] = 0.484859839322;
    y[8] = 0.347417256114*sqrh;
    y[9] = 0.0;
    y[10] = -0.1783699667363*sqrh;
    y[11] = -0.1278076660325;
    y[12] = -0.09157819444367*sqrh;
    y[13] = 0.0;
    y[14] = 0.04701781275748*sqrh;
    y[15] = 0.03368973499543;
    y[16] = 0.02413974996916*sqrh;
    y[17] = 0.0;
    y[18] = -0.01239376088333*sqrh;
    y[19] = 0.0;

    for (int i = 0; i <=19; i++)
    {
      e[i] = 1.;
    }

  }

  void testAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "ExpDecayOscMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up Lorentzian fitting function
    ExpDecayOsc fn;
    fn.initialize();
    BoundaryConstraint* bc = new BoundaryConstraint(&fn,"Frequency",0.01,.2);
    fn.addConstraint(bc);
    bc = new BoundaryConstraint(&fn,"Phi",0.01,1.0);
    fn.addConstraint(bc);

    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",fn.asString());


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA( out->getParameter("A"), 5 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("Lambda"), 1/3.0 ,0.01);
    TS_ASSERT_DELTA( out->getParameter("Frequency"), 1/8.0 ,0.01);  // Period of 8
    TS_ASSERT_DELTA( out->getParameter("Phi"), M_PI_4 ,0.01);  // 45 degrees

    // check it categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*EXPDECAYOSCTEST_H_*/
