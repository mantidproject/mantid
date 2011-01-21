#ifndef EXPDECAYTEST_H_
#define EXPDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ExpDecay.h"
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


class ExpDecayTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    
    y[0] = 5;
    y[1] = 3.582656552869;
    y[2] = 2.567085595163;
    y[3] = 1.839397205857;
    y[4] = 1.317985690579;
    y[5] = 0.9443780141878;
    y[6] = 0.6766764161831;
    y[7] = 0.484859839322;
    y[8] = 0.347417256114;
    y[9] = 0.2489353418393;
    y[10] = 0.1783699667363;
    y[11] = 0.1278076660325;
    y[12] = 0.09157819444367;
    y[13] = 0.0656186436847;
    y[14] = 0.04701781275748;
    y[15] = 0.03368973499543;
    y[16] = 0.02413974996916;
    y[17] = 0.01729688668232;
    y[18] = 0.01239376088333;

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
    std::string wsName = "ExpDecayMockData";
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
    ExpDecay* fn = new ExpDecay();
    fn->initialize();

    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",*fn);


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
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0001,0.0001);

    IFitFunction *out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("Height"), 5 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("Lifetime"), 3 ,0.001);

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*EXPDECAYTEST_H_*/
