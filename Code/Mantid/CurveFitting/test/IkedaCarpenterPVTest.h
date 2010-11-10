#ifndef IKEDACARPENTERPVTEST_H_
#define IKEDACARPENTERPVTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/IkedaCarpenterPV.h"
#include "MantidCurveFitting/Fit.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataHandling/LoadRaw.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidKernel/Exception.h"
#include "MantidAlgorithms/ConvertUnits.h"
#include "MantidAlgorithms/DiffractionFocussing2.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidNexus/LoadNeXus.h"
#include "MantidKernel/ConfigService.h"
#include <limits>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;
using namespace Mantid::NeXus;

class IkedaCarpenterPVTest : public CxxTest::TestSuite
{
public:

  IkedaCarpenterPVTest()
  {
    ConfigService::Instance().setString("curvefitting.peakRadius","100");
  }

  void getMockData(Mantid::MantidVec& y, Mantid::MantidVec& e)
  {
    y[0]  =    0.0000;
    y[1]  =    0.0003;
    y[2]  =    0.0028;
    y[3]  =    0.0223;
    y[4]  =    0.1405;
    y[5]  =    0.6996;
    y[6]  =    2.7608;
    y[7]  =    8.6586;
    y[8]  =   21.6529;
    y[9]  =   43.3558;
    y[10] =   69.8781;
    y[11] =   91.2856;
    y[12] =   97.5646;
    y[13] =   86.4481;
    y[14] =   64.7703;
    y[15] =   42.3348;
    y[16] =   25.3762;
    y[17] =   15.0102;
    y[18] =    9.4932;
    y[19] =    6.7037;
    y[20] =    5.2081;
    y[21] =    4.2780;
    y[22] =    3.6037;
    y[23] =    3.0653;
    y[24] =    2.6163;
    y[25] =    2.2355;
    y[26] =    1.9109;
    y[27] =    1.6335;
    y[28] =    1.3965;
    y[29] =    1.1938;
    y[30] =    1.0206;

    e[0]  =      0.0056;
    e[1]  =      0.0176;
    e[2]  =      0.0539;
    e[3]  =      0.1504;
    e[4]  =      0.3759;
    e[5]  =      0.8374;
    e[6]  =      1.6626;
    e[7]  =      2.9435;
    e[8]  =      4.6543;
    e[9]  =      6.5855;
    e[10] =      8.3603;
    e[11] =      9.5553;
    e[12] =      9.8785;
    e[13] =      9.2987;
    e[14] =      8.0490;
    e[15] =      6.5075;
    e[16] =      5.0385;
    e[17] =      3.8753;
    e[18] =      3.0821;
    e[19] =      2.5902;
    e[20] =      2.2831;
    e[21] =      2.0693;
    e[22] =      1.8993;
    e[23] =      1.7518;
    e[24] =      1.6185;
    e[25] =      1.4962;
    e[26] =      1.3833;
    e[27] =      1.2791;
    e[28] =      1.1827;
    e[29] =      1.0936;
    e[30] =      1.0112;
  }

  void t1estAgainstMockData()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // create mock data to test against
    std::string wsName = "IkedaCarpenterPV1D_GaussMockData";
    int histogramNumber = 1;
    int timechannels = 31;
    Workspace_sptr ws = WorkspaceFactory::Instance().create("Workspace2D",histogramNumber,timechannels,timechannels);
    Workspace2D_sptr ws2D = boost::dynamic_pointer_cast<Workspace2D>(ws);
    for (int i = 0; i < timechannels; i++) ws2D->dataX(0)[i] = i*5;
    Mantid::MantidVec& y = ws2D->dataY(0); // y-values (counts)
    Mantid::MantidVec& e = ws2D->dataE(0); // error values of counts
    getMockData(y, e);

    //put this workspace in the data service
    TS_ASSERT_THROWS_NOTHING(AnalysisDataService::Instance().add(wsName, ws2D));


    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", wsName);
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","0");
    alg2.setPropertyValue("EndX","150");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV(1.0);
    icpv->initialize();
    icpv->setWorkspace(ws2D, 1,0,1);

    icpv->setParameter("I",95000);
    icpv->tie("Alpha0", "1.597107");
    icpv->tie("Alpha1", "1.496805");
    icpv->tie("Beta0", "31.891718");
    icpv->tie("Kappa", "46.025921");
    icpv->tie("SigmaSquared", "100.0");
    icpv->setParameter("X0",45.0);
    icpv->tie("Gamma", "1.0");

    alg2.setPropertyValue("Function",*icpv);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    return;

    // test the output from fit is what you expect
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.0,0.0001);

    //TS_ASSERT_DELTA( gaus->height(), 31 ,0.001);
    TS_ASSERT_DELTA( icpv->centre(), 50 ,0.0001);
    //TS_ASSERT_DELTA( gaus->width(), 2.2284 ,0.0001);

    TS_ASSERT_DELTA( icpv->getParameter("I"), 3101 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 1.597107 ,0.0001);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 1.496805 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Beta0"), 31.891718 ,0.0001);
    TS_ASSERT_DELTA( icpv->getParameter("Kappa"), 46.025921 ,0.0001);
    TS_ASSERT_DELTA( icpv->getParameter("SigmaSquared"), 100.0 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Gamma"), 1.0 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("X0"), 50.0 ,0.001);

    AnalysisDataService::Instance().remove(wsName);
  }

  void testAgainstHRPD_data()
  {
    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    LoadRaw loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "../../../../Test/AutoTestData/HRP39182.raw");
    loader.setPropertyValue("OutputWorkspace", "HRP39182");
    loader.execute();

    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("HRP39182"));

    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", "HRP39182");
    alg2.setPropertyValue("WorkspaceIndex","3");
    alg2.setPropertyValue("StartX","79300");
    alg2.setPropertyValue("EndX","79600");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV();
    icpv->initialize();
    icpv->setWorkspace(wsToPass, 3,0,1);

    icpv->setParameter("I",9500);
    icpv->tie("Alpha0", "1.597107");
    icpv->tie("Alpha1", "1.496805");
    icpv->tie("Beta0", "31.891718");
    icpv->tie("Kappa", "46.025921");
    icpv->setParameter("SigmaSquared",100);
    icpv->tie("SigmaSquared", "100.0");
    icpv->tie("X0", "79400");
    icpv->tie("Gamma", "1.0");

    alg2.setFunction(icpv);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    // note this test will never produce good fit because it assumes no background
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 11.67,1);

    AnalysisDataService::Instance().remove("HRP39182");
  }


  // motivation for this test is to figure out way IC function goes absolutely
  // nuts when a large data range are selection
  void testAgainstGEM_dataLargeDataRange()
  {
    LoadNexus load;
    load.initialize();
    load.setPropertyValue("FileName", "../../../../Test/AutoTestData/focussedGEM38370_TOF.nxs");
    std::string wsname = "GEM38370nexus";
    load.setPropertyValue("OutputWorkspace", wsname);
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT( load.isExecuted() );

    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(wsname));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", wsname);
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","5000");
    alg2.setPropertyValue("EndX","10000");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->tie("A1", "0.0");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV();
    icpv->initialize();

    icpv->setParameter("I",25094.45);
    icpv->setParameter("X0",7316);
    //icpv->setParameter("Gamma",1);
    //icpv->tie("Gamma", "1");

    icpv->setWorkspace(wsToPass, 1,0,1);  // for unit testing purpose set workspace here

    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 0.734079 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 2.067249 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("SigmaSquared"), 6403 ,1);


    std::vector<double> testing;
    for (double d=5000; d<=10000; d+=1000)
      testing.push_back(d);

    std::vector<double> out=testing;

    icpv->function(&out[0], &testing[0], testing.size());

    TS_ASSERT_DELTA( out[0], 0.2694,0.001);

    AnalysisDataService::Instance().remove(wsname);
    // Append value of date-time tag inside the geometry file to the constructor handle 
    // for change to LoadInstrument
    InstrumentDataService::Instance().remove("GEM_Definition.xml16th Sep 2008");
  }


  void testAgainstGEM_data()
  {
    LoadNexus load;
    load.initialize();
    load.setPropertyValue("FileName", "../../../../Test/AutoTestData/focussedGEM38370_TOF.nxs");
    std::string wsname = "GEM38370nexus";
    load.setPropertyValue("OutputWorkspace", wsname);
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT( load.isExecuted() );

    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve(wsname));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", wsname);
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","6935.79");
    alg2.setPropertyValue("EndX","7682.56");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    bk->tie("A1", "0.0");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV();
    icpv->initialize();

    icpv->setParameter("I",106860.45);
    icpv->setParameter("X0",7326.34);
    icpv->setParameter("Gamma",1);
    icpv->tie("Gamma", "1");

    icpv->setWorkspace(wsToPass, 1,0,1);  // for unit testing purpose set workspace here

    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 0.734079 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 2.067249 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("SigmaSquared"), 6422 ,1);

    fnWithBk->addFunction(icpv);
    fnWithBk->addFunction(bk);

    alg2.setFunction(fnWithBk);

    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    // note this test will never produce good fit because it assumes no background
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.831,0.01);

    TS_ASSERT_DELTA( icpv->getParameter("I"), 69562 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 0.734079 ,0.1);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 2.067249 ,0.1);
    TS_ASSERT_DELTA( icpv->getParameter("SigmaSquared"), 3567 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("X0"), 7301 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("Gamma"), 1 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 90.0 ,1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.000000001); 

    AnalysisDataService::Instance().remove(wsname);
  }

  /* This test is basically a repeat of the test testAgainstGEM_data(). However I had various
     problems with getting the saved nexus focused GEM to work probably in that test hence this
     test repeats testAgainstGEM_data() but directly from focused data in C++ */
  void testAgainstGEM_data2()
  {
    LoadRaw3 load;
    load.initialize();
    load.setPropertyValue("FileName", "../../../../Test/AutoTestData/GEM38370.raw");
    load.setPropertyValue("OutputWorkspace", "GEM38370");
    TS_ASSERT_THROWS_NOTHING(load.execute());
    TS_ASSERT( load.isExecuted() );

    AlignDetectors align;
    align.initialize();
    align.setPropertyValue("InputWorkspace", "GEM38370");
    align.setPropertyValue("OutputWorkspace", "GEM38370");
    align.setPropertyValue("CalibrationFile", "../../../../Test/AutoTestData/offsets_2006_cycle064.cal");
    TS_ASSERT_THROWS_NOTHING(align.execute());
    TS_ASSERT( align.isExecuted() );

    DiffractionFocussing2 focused;
    focused.initialize();
    focused.setPropertyValue("InputWorkspace", "GEM38370");
    focused.setPropertyValue("OutputWorkspace", "focused");
    focused.setPropertyValue("GroupingFileName", "../../../../Test/AutoTestData/offsets_2006_cycle064.cal");
    TS_ASSERT_THROWS_NOTHING(focused.execute());
    TS_ASSERT( focused.isExecuted() );

    ConvertUnits units;
    units.initialize();
    units.setPropertyValue("InputWorkspace", "focused");
    units.setPropertyValue("OutputWorkspace", "tof");
    units.setPropertyValue("Target", "TOF");
    units.setPropertyValue("EMode", "Direct");
    TS_ASSERT_THROWS_NOTHING(units.execute());
    TS_ASSERT( units.isExecuted() );

    Mantid::DataObjects::Workspace2D_sptr wsToPass = boost::dynamic_pointer_cast<Mantid::DataObjects::Workspace2D>(AnalysisDataService::Instance().retrieve("tof"));

    Fit alg2;
    TS_ASSERT_THROWS_NOTHING(alg2.initialize());
    TS_ASSERT( alg2.isInitialized() );

    // Set general Fit parameters
    alg2.setPropertyValue("InputWorkspace", "tof");
    alg2.setPropertyValue("WorkspaceIndex","1");
    alg2.setPropertyValue("StartX","6935.79");
    alg2.setPropertyValue("EndX","7682.56");

    // create function you want to fit against
    CompositeFunction *fnWithBk = new CompositeFunction();

    LinearBackground *bk = new LinearBackground();
    bk->initialize();

    bk->setParameter("A0",0.0);
    bk->setParameter("A1",0.0);
    //bk->removeActive(1);
    bk->tie("A1", "0.0");

    // set up fitting function and pass to Fit
    IkedaCarpenterPV* icpv = new IkedaCarpenterPV();
    icpv->initialize();
    //int b;
    //std::cin >> b;
    icpv->setWorkspace(wsToPass, 1,0,1);

    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 0.734079 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 2.067249 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Beta0") ,32.017204 ,0.001);
    TS_ASSERT_DELTA( icpv->getParameter("Kappa"), 48.734158, 0.001);

    icpv->setParameter("I",106860.45);

    icpv->setParameter("SigmaSquared",10075.96);
    icpv->setParameter("X0",7326.34);
    icpv->setParameter("Gamma",1);
    icpv->tie("Gamma", "1");

    fnWithBk->addFunction(icpv);
    fnWithBk->addFunction(bk);

    alg2.setFunction(fnWithBk);


    // execute fit
    TS_ASSERT_THROWS_NOTHING(
      TS_ASSERT( alg2.execute() )
    )
    TS_ASSERT( alg2.isExecuted() );

    // test the output from fit is what you expect
    // note this test will never produce good fit because it assumes no background
    double dummy = alg2.getProperty("Output Chi^2/DoF");
    TS_ASSERT_DELTA( dummy, 0.831,0.01);

    TS_ASSERT_DELTA( icpv->getParameter("I"), 69562 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha0"), 0.734079 ,0.1);
    TS_ASSERT_DELTA( icpv->getParameter("Alpha1"), 2.067249 ,0.1);
    TS_ASSERT_DELTA( icpv->getParameter("SigmaSquared"), 3567 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("X0"), 7301 ,1);
    TS_ASSERT_DELTA( icpv->getParameter("Gamma"), 1 ,0.1);
    TS_ASSERT_DELTA( bk->getParameter("A0"), 90.0 ,1);
    TS_ASSERT_DELTA( bk->getParameter("A1"), 0.0 ,0.000000001); 

    AnalysisDataService::Instance().remove("GEM38370");
    AnalysisDataService::Instance().remove("focused");
    AnalysisDataService::Instance().remove("tof");
  }


};

#endif /*IKEDACARPENTERPVTEST_H_*/
