#ifndef REBINTEST_H_
#define REBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::API;
using namespace Mantid::Algorithms;


class RebinTest : public CxxTest::TestSuite
{
public:
  double BIN_DELTA;
  int NUMPIXELS, NUMBINS;

  RebinTest()
  {
    BIN_DELTA = 2.0;
    NUMPIXELS = 20;
    NUMBINS = 50;
  }


  void testworkspace1D_dist()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in1D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    // Check it fails if "Params" property not set
    TS_ASSERT_THROWS( rebin.execute(), std::runtime_error );
    TS_ASSERT( ! rebin.isExecuted() );
    // Now set the property
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));
    const Mantid::MantidVec outX=rebindata->readX(0);
    const Mantid::MantidVec outY=rebindata->readY(0);
    const Mantid::MantidVec outE=rebindata->readE(0);

    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7], sqrt(4.5)/2.0  ,0.000001);

    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42 ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25) ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }



//  void testworkspace1D_dist_stupid_bins()
//  {
//    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
//    test_in1D->isDistribution(true);
//    AnalysisDataService::Instance().add("test_in1D", test_in1D);
//
//    Rebin rebin;
//    rebin.initialize();
//    rebin.setPropertyValue("InputWorkspace","test_in1D");
//    rebin.setPropertyValue("OutputWorkspace","test_out");
//    // Check it fails if "Params" property not set
//    TS_ASSERT_THROWS( rebin.execute(), std::runtime_error );
//    TS_ASSERT( ! rebin.isExecuted() );
//    // Way too many bins
//    rebin.setPropertyValue("Params", "0.0,1e-8,10");
//    // Fails to execute
//    TS_ASSERT(!rebin.execute());
//    TS_ASSERT(!rebin.isExecuted());
//    AnalysisDataService::Instance().remove("test_in1D");
//  }

  void testworkspace1D_nondist()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in1D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));

    const Mantid::MantidVec outX=rebindata->readX(0);
    const Mantid::MantidVec outY=rebindata->readY(0);
    const Mantid::MantidVec outE=rebindata->readE(0);

    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],8.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(8.0)  ,0.000001);
    TS_ASSERT_DELTA(outX[12],24.2  ,0.000001);
    TS_ASSERT_DELTA(outY[12],9.68 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(9.68)  ,0.000001);
    TS_ASSERT_DELTA(outX[17],32  ,0.000001);
    TS_ASSERT_DELTA(outY[17],4.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(4.0)  ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(!dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");
  }


  void testworkspace1D_logarithmic_binning()
  {
    Workspace1D_sptr test_in1D = Create1DWorkspace(50);
    test_in1D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in1D", test_in1D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in1D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    // Check it fails if "Params" property not set
    TS_ASSERT_THROWS( rebin.execute(), std::runtime_error );
    TS_ASSERT( ! rebin.isExecuted() );
    // Now set the property
    rebin.setPropertyValue("Params", "1.0,-1.0,1000.0");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));
    const Mantid::MantidVec outX=rebindata->readX(0);
    const Mantid::MantidVec outY=rebindata->readY(0);
    const Mantid::MantidVec outE=rebindata->readE(0);

    TS_ASSERT_EQUALS(outX.size(), 11);
    TS_ASSERT_DELTA(outX[0], 1.0  , 1e-5);
    TS_ASSERT_DELTA(outX[1], 2.0  , 1e-5);
    TS_ASSERT_DELTA(outX[2], 4.0  , 1e-5); //and so on...
    TS_ASSERT_DELTA(outX[10], 1000.0  , 1e-5);

    bool dist=rebindata->isDistribution();
    TS_ASSERT(dist);
    AnalysisDataService::Instance().remove("test_in1D");
    AnalysisDataService::Instance().remove("test_out");

  }

  void testworkspace2D_dist()
  {
    Workspace2D_sptr test_in2D = Create2DWorkspace(50,20);
    test_in2D->isDistribution(true);
    AnalysisDataService::Instance().add("test_in2D", test_in2D);

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_in2D");
    rebin.setPropertyValue("OutputWorkspace","test_out");
    rebin.setPropertyValue("Params", "1.5,2.0,20,-0.1,30,1.0,35");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());
    MatrixWorkspace_sptr rebindata = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out"));

    const Mantid::MantidVec outX=rebindata->readX(5);
    const Mantid::MantidVec outY=rebindata->readY(5);
    const Mantid::MantidVec outE=rebindata->readE(5);
    TS_ASSERT_DELTA(outX[7],15.5  ,0.000001);
    TS_ASSERT_DELTA(outY[7],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[7],sqrt(4.5)/2.0  ,0.000001);

    TS_ASSERT_DELTA(outX[12],24.2 ,0.000001);
    TS_ASSERT_DELTA(outY[12],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[12],sqrt(5.445)/2.42  ,0.000001);

    TS_ASSERT_DELTA(outX[17],32.0  ,0.000001);
    TS_ASSERT_DELTA(outY[17],3.0 ,0.000001);
    TS_ASSERT_DELTA(outE[17],sqrt(2.25)  ,0.000001);
    bool dist=rebindata->isDistribution();
    TS_ASSERT(dist);

    AnalysisDataService::Instance().remove("test_in2D");
    AnalysisDataService::Instance().remove("test_out");
  }




  void do_test_EventWorkspace(EventType eventType, bool inPlace, bool PreserveEvents, bool expectOutputEvent)
  {
    // Two events per bin
    EventWorkspace_sptr test_in = WorkspaceCreationHelper::CreateEventWorkspace2(50, 100);
    test_in->switchEventType(eventType);

    std::string inName("test_inEvent");
    std::string outName("test_inEvent_output");
    if (inPlace) outName = inName;

    AnalysisDataService::Instance().addOrReplace(inName, test_in);
    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace",inName);
    rebin.setPropertyValue("OutputWorkspace",outName);
    rebin.setPropertyValue("Params", "0.0,4.0,100");
    rebin.setProperty("PreserveEvents", PreserveEvents);
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    MatrixWorkspace_sptr outWS;
    EventWorkspace_sptr eventOutWS;
    TS_ASSERT_THROWS_NOTHING( outWS = boost::shared_dynamic_cast<MatrixWorkspace>( AnalysisDataService::Instance().retrieve(outName) ));
    TS_ASSERT( outWS );
    if (!outWS) return;

    // Is the output gonna be events?
    if (expectOutputEvent)
    {
      eventOutWS = boost::dynamic_pointer_cast<EventWorkspace>(outWS);
      TS_ASSERT(eventOutWS);
      if (!eventOutWS) return;
      TS_ASSERT_EQUALS( eventOutWS->getNumberEvents(), 50 * 100 * 2);
      // Check that it is the same workspace
      if (inPlace)
        TS_ASSERT( eventOutWS == test_in );
    }

    const MantidVec & X = outWS->readX(0);
    const MantidVec & Y = outWS->readY(0);
    const MantidVec & E = outWS->readE(0);

    TS_ASSERT_EQUALS( X.size(), 26);
    TS_ASSERT_DELTA( X[0], 0.0, 1e-5);
    TS_ASSERT_DELTA( X[1], 4.0, 1e-5);
    TS_ASSERT_DELTA( X[2], 8.0, 1e-5);

    TS_ASSERT_EQUALS( Y.size(), 25);
    TS_ASSERT_DELTA( Y[0], 8.0, 1e-5);
    TS_ASSERT_DELTA( Y[1], 8.0, 1e-5);
    TS_ASSERT_DELTA( Y[2], 8.0, 1e-5);

    TS_ASSERT_EQUALS( E.size(), 25);
    TS_ASSERT_DELTA( E[0], sqrt(8.0), 1e-5);
    TS_ASSERT_DELTA( E[1], sqrt(8.0), 1e-5);

    AnalysisDataService::Instance().remove(inName);
    AnalysisDataService::Instance().remove(outName);
  }


  void testEventWorkspace_InPlace_PreserveEvents()
  {
    do_test_EventWorkspace(TOF, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weighted()
  {
    do_test_EventWorkspace(WEIGHTED, true, true, true);
  }

  void testEventWorkspace_InPlace_PreserveEvents_weightedNoTime()
  {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, true, true);
  }


  void testEventWorkspace_InPlace_NoPreserveEvents()
  {
    do_test_EventWorkspace(TOF, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weighted()
  {
    do_test_EventWorkspace(WEIGHTED, true, false, false);
  }

  void testEventWorkspace_InPlace_NoPreserveEvents_weightedNoTime()
  {
    do_test_EventWorkspace(WEIGHTED_NOTIME, true, false, false);
  }


  void testEventWorkspace_NotInPlace_NoPreserveEvents()
  {
    do_test_EventWorkspace(TOF, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weighted()
  {
    do_test_EventWorkspace(WEIGHTED, false, false, false);
  }

  void testEventWorkspace_NotInPlace_NoPreserveEvents_weightedNoTime()
  {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, false, false);
  }


  void testEventWorkspace_NotInPlace_PreserveEvents()
  {
    do_test_EventWorkspace(TOF, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weighted()
  {
    do_test_EventWorkspace(WEIGHTED, false, true, true);
  }

  void testEventWorkspace_NotInPlace_PreserveEvents_weightedNoTime()
  {
    do_test_EventWorkspace(WEIGHTED_NOTIME, false, true, true);
  }

  void testRebinPointData()
  {
    Workspace1D_sptr input = Create1DWorkspace(51);
    AnalysisDataService::Instance().add("test_RebinPointDataInput", input);

    Mantid::API::Algorithm_sptr ctpd = Mantid::API::AlgorithmFactory::Instance().create("ConvertToPointData", 1);
    ctpd->initialize();
    ctpd->setPropertyValue("InputWorkspace", "test_RebinPointDataInput");
    ctpd->setPropertyValue("OutputWorkspace", "test_RebinPointDataInput");
    ctpd->execute();

    Mantid::API::Algorithm_sptr reb = Mantid::API::AlgorithmFactory::Instance().create("Rebin", 1);
    reb->initialize();
    TS_ASSERT_THROWS_NOTHING( reb->setPropertyValue("InputWorkspace", "test_RebinPointDataInput") );
    reb->setPropertyValue("OutputWorkspace", "test_RebinPointDataOutput");
    reb->setPropertyValue("Params", "7,0.75,23");
    TS_ASSERT_THROWS_NOTHING( reb->execute() );

    TS_ASSERT( reb->isExecuted() );

    MatrixWorkspace_sptr outWS = boost::shared_dynamic_cast<MatrixWorkspace>( AnalysisDataService::Instance().retrieve("test_RebinPointDataOutput") );

    TS_ASSERT(! outWS->isHistogramData() );
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 1 );

    TS_ASSERT_EQUALS(outWS->readX(0)[0], 7.3750);
    TS_ASSERT_EQUALS(outWS->readX(0)[10], 14.8750);
    TS_ASSERT_EQUALS(outWS->readX(0)[20], 22.3750);

    AnalysisDataService::Instance().remove("test_RebinPointDataInput");
    AnalysisDataService::Instance().remove("test_RebinPointDataOutput");
  }

private:


  Workspace1D_sptr Create1DWorkspace(int size)
  {
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(size-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(size-1,sqrt(3.0)));
    Workspace1D_sptr retVal(new Workspace1D);
    retVal->initialize(1,size,size-1);
    double j=1.0;
    for (int i=0; i<size; i++)
    {
      retVal->dataX()[i]=j*0.5;
      j+=1.5;
    }
    retVal->setData(y1,e1);
    return retVal;
  }
  
  Workspace2D_sptr Create2DWorkspace(int xlen, int ylen)
  {
    boost::shared_ptr<Mantid::MantidVec> x1(new Mantid::MantidVec(xlen,0.0));
    boost::shared_ptr<Mantid::MantidVec> y1(new Mantid::MantidVec(xlen-1,3.0));
    boost::shared_ptr<Mantid::MantidVec> e1(new Mantid::MantidVec(xlen-1,sqrt(3.0)));

    Workspace2D_sptr retVal(new Workspace2D);
    retVal->initialize(ylen,xlen,xlen-1);
    double j=1.0;

    for (int i=0; i<xlen; i++)
    {
      (*x1)[i]=j*0.5;
      j+=1.5;
    }

    for (int i=0; i< ylen; i++)
    {
      retVal->setX(i,x1);
      retVal->setData(i,y1,e1);
    }

    return retVal;
  }


};
#endif /* REBINTEST */
