#ifndef ABRAGAMTEST_H_
#define ABRAGAMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Abragam.h"
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


class AbragamTest : public CxxTest::TestSuite
{
public:

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    // Data got from the Abragam function on Excel spreadsheet with
    // A = 0.3, Omega = 0.4, Phi = PI/4, Sigma = 0.2, Tau = 2.0
    y[0] = 0.212132034;
    y[1] = 0.110872429;
    y[2] = -0.004130004;
    y[3] = -0.107644046;
    y[4] = -0.181984622;
    y[5] = -0.218289678;
    y[6] = -0.215908947;
    y[7] = -0.180739307;
    y[8] = -0.123016506;
    y[9] = -0.054943061;
    y[10] = 0.011526466;
    y[11] = 0.066481012;
    y[12] = 0.103250678;
    y[13] = 0.118929645;
    y[14] = 0.114251678;
    y[15] = 0.092934753;
    y[16] = 0.060672555;
    y[17] = 0.023977227;
    y[18] = -0.010929869;
    y[19] = -0.039018774;
    y[20] = -0.057037526;

    for (int i = 0; i <=20; i++)
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
    std::string wsName = "AbragamMockData";
    int histogramNumber = 1;
    int timechannels = 21;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 21; i++) ws2D->dataX(0)[i] = i;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    // set up Lorentzian fitting function
    Abragam fn;
    fn.initialize();

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
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.000001,0.000001);

    // test the output from fit is what you expect
    auto out = FunctionFactory::Instance().createInitialized(alg2.getPropertyValue("Function"));
    TS_ASSERT_DELTA( out->getParameter("A"), 0.3 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Omega"), 0.4 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Phi"), M_PI/4.0 ,0.01);  // 45 degrees
    TS_ASSERT_DELTA( out->getParameter("Sigma"), 0.2 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Tau"), 2.0 ,0.01);

    // check its categories
    const std::vector<std::string> categories = out->categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );

    AnalysisDataService::Instance().remove(wsName);

  }


};

#endif /*ABRAGAMTEST_H_*/
