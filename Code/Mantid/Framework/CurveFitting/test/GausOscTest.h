#ifndef GAUSOSCTEST_H_
#define GAUSOSCTEST_H_

#include <cxxtest/TestSuite.h>
#include <cmath>

#include "MantidCurveFitting/GausOsc.h"
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

#include "MantidDataHandling/SaveNexus.h"  // Debug

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;


class GausOscTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    const double sqrh = 0.70710678; // cos( 45 degrees )

    y[0] = 0.01*sqrh;
    y[1] = 0.00;
    y[2] = -1.2*sqrh;
    y[3] = -5.6;
    y[4] = -18.2*sqrh;
    y[5] = 0.0;
    y[6] = 80.08*sqrh;
    y[7] = 114.4;
    y[8] = 128.7*sqrh;
    y[9] = 0.0;
    y[10] = -80.08*sqrh;
    y[11] = -43.68;
    y[12] = -18.2*sqrh;
    y[13] = 0.0;
    y[14] = 1.2*sqrh;
    y[15] = 0.16;
    y[16] = 0.01*sqrh;
    y[17] = 0.00;

    for (int i = 0; i <=17; i++)
    {
      e[i] = 1.0;
    }

  }

  void testAgainstMockData()  // Parts of test disabled because it does not give result like that obtained in mantidplot.
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "GausOscMockData";
    int histogramNumber = 1;
    int timechannels = 18;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 18; i++) ws2D->dataX(0)[i] = i-8.0;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up fitting function
    GausOsc fn;
    fn.initialize();

    //alg2.setFunction(fn);
    alg2.setPropertyValue("Function",fn.asString());


    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","0");
    alg2.setPropertyValue("StartX","-8");
    alg2.setPropertyValue("EndX","8");

    alg2.setPropertyValue("Output","OutputGausDecay");  //debug


    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0, 1.0);

    // test the output from fit is what you expect
    auto out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("A"), 128.7 ,0.9);
    TS_ASSERT_DELTA( out->getParameter("Sigma"), 0.35 ,0.005);
    TS_ASSERT_DELTA( out->getParameter("Frequency"), 1/8.0 ,0.01);  // Period of 8
    TS_ASSERT_DELTA( out->getParameter("Phi"), M_PI_4 ,0.01);  //  45 degrees

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*GAUSOSCTEST_H_*/
