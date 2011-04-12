#ifndef LORENTZIAN1DTEST_H_
#define LORENTZIAN1DTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Lorentzian1D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Algorithm.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::CurveFitting::Lorentzian1D;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class Lorentzian1DTest : public CxxTest::TestSuite
{
public:
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testAgainstMockData()
  {
    Lorentzian1D alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "LorentzianMockData";
    int histogramNumber = 1;
    int timechannels = 20;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < 20; i++) ws2D->dataX(0)[i] = i+1;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    y[0] =     4.1733;
    y[1] =     4.3729;
    y[2] =     4.8150;
    y[3] =     5.3402;
    y[4] =     6.0909;
    y[5] =     7.3389;
    y[6] =     9.4883;
    y[7] =    13.6309;
    y[8] =    23.1555;
    y[9] =    48.9471;
    y[10] =  100.4982;
    y[11] =   68.8164;
    y[12] =   30.3590;
    y[13] =   16.4184;
    y[14] =   10.7455;
    y[15] =    8.0570;
    y[16] =    6.5158;
    y[17] =    5.5496;
    y[18] =    5.0087;
    y[19] =    4.5027;
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    e[0] =       2.0429;
    e[1] =       2.0911;
    e[2] =       2.1943;
    e[3] =       2.3109;
    e[4] =       2.4680;
    e[5] =       2.7090;
    e[6] =       3.0803;
    e[7] =       3.6920;
    e[8] =       4.8120;
    e[9] =       6.9962;
    e[10] =     10.0249;
    e[11] =      8.2956;
    e[12] =      5.5099;
    e[13] =      4.0520;
    e[14] =      3.2780;
    e[15] =      2.8385;
    e[16] =      2.5526;
    e[17] =      2.3558;
    e[18] =      2.2380;
    e[19] =      2.1220;

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));

    // Set which spectrum to fit against and initial starting values
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","20");
    alg2.setPropertyValue("BG0", "2.0");
    alg2.setPropertyValue("BG1", "0.0");
    alg2.setPropertyValue("Height", "105.7");
    alg2.setPropertyValue("PeakCentre", "13.5");
    alg2.setPropertyValue("HWHM", "1.2");

    // execute fit
   TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )

    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("OutputChi2overDoF");
    TS_ASSERT_DELTA( dummy, 0.0002,0.0005);
    dummy = alg2.getProperty("BG0");
    TS_ASSERT_DELTA( dummy, 3.017,0.002);
    dummy = alg2.getProperty("BG1");
    TS_ASSERT_DELTA( dummy, 0.0 ,0.005);
    dummy = alg2.getProperty("Height");
    TS_ASSERT_DELTA( dummy, 100.69 ,0.01);
    dummy = alg2.getProperty("PeakCentre");
    TS_ASSERT_DELTA( dummy, 11.20 ,0.01);
    dummy = alg2.getProperty("HWHM");
    TS_ASSERT_DELTA( dummy, 1.10 ,0.01);

  }


private:
  Lorentzian1D alg;
};

#endif /*LORENTZIAN1DTEST_H_*/
