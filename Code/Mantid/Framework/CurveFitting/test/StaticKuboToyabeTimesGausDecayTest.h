#ifndef MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_
#define MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/StaticKuboToyabeTimesGausDecay.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidDataObjects/Workspace2D.h"

using Mantid::CurveFitting::StaticKuboToyabeTimesGausDecay;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;

class StaticKuboToyabeTimesGausDecayTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static StaticKuboToyabeTimesGausDecayTest *createSuite() { return new StaticKuboToyabeTimesGausDecayTest(); }
  static void destroySuite( StaticKuboToyabeTimesGausDecayTest *suite ) { delete suite; }

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    // A = 0.24, Delta = 0.16, Sigma = 0.1
    y[0] = 0.24;
    y[1] = 0.231594;
    y[2] = 0.207961;
    y[3] = 0.173407;
    y[4] = 0.133761;
    y[5] = 0.0948783;
    y[6] = 0.0613345;
    y[7] = 0.035692;
    y[8] = 0.0184429;
    y[9] = 0.0084925;
    y[10] = 0.00390022;
    y[11] = 0.00258855;
    y[12] = 0.00283237;
    y[13] = 0.00347216;
    y[14] = 0.00390132;

    for (int i = 0; i < 15; i++)
      e[i] = 1.0;
  }

  StaticKuboToyabeTimesGausDecayTest()
    : fn()
  {}

  void test_Initialize()
  {
    TS_ASSERT_THROWS_NOTHING(fn.initialize());
  }

  void test_Name()
  {
    TS_ASSERT_EQUALS(fn.name(), "StaticKuboToyabeTimesGausDecay");
  }

  void test_Params()
  {
    TS_ASSERT_DELTA(fn.getParameter("A"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Delta"), 0.2, 0.0001);
    TS_ASSERT_DELTA(fn.getParameter("Sigma"), 0.2, 0.0001);
  }

  void test_Category()
  {
    const std::vector<std::string> categories = fn.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Muon" );
  }

  void test_AgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "SKTTimesGausDecayMockData";
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D", 1, 15, 15);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    
    for (int i = 0; i < 15; i++) 
      ws2D->dataX(0)[i] = i;
    
    getMockData(ws2D->dataY(0), ws2D->dataE(0));

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().addOrReplace(wsName, ws2D));

    alg2.setPropertyValue("Function",fn.asString());

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex", "0");
    alg2.setPropertyValue("StartX", "0");
    alg2.setPropertyValue("EndX", "14");

    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0001,0.0001);

    IFunction_sptr out = alg2.getProperty("Function");
    TS_ASSERT_DELTA( out->getParameter("A"), 0.24 ,0.0001);
    TS_ASSERT_DELTA( out->getParameter("Delta"), 0.16 ,0.001);
    TS_ASSERT_DELTA( out->getParameter("Sigma"), 0.1 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  StaticKuboToyabeTimesGausDecay fn;
};


#endif /* MANTID_CURVEFITTING_STATICKUBOTOYABETIMESGAUSDECAYTEST_H_ */