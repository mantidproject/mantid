#ifndef REBINTEST_H_
#define REBINTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataObjects/Workspace1D.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAPI/WorkspaceProperty.h"

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

  void do_testEventWorkspace_SameOutputWorkspace(EventType eventType)
  {
    EventWorkspace_sptr test_in = CreateEventWorkspace(NUMBINS, NUMPIXELS, eventType);
    AnalysisDataService::Instance().add("test_inEvent", test_in);

    const EventList el(test_in->getEventList(1));
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA);
    //Because of the way the events were faked, bins 0 to pixel-1 are 0, rest are 1
    TS_ASSERT_EQUALS( (*el.dataY())[0], 1);
    TS_ASSERT_EQUALS( (*el.dataY())[1], 1);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMBINS-2], 1); //The last bin

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_inEvent");
    rebin.setPropertyValue("OutputWorkspace","test_inEvent");
    rebin.setPropertyValue("Params", "0.0,4.0,100");
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    const EventList el2(test_in->getEventList(2));
    TS_ASSERT_EQUALS( el2.dataX()[0], 0.0);
    TS_ASSERT_EQUALS( el2.dataX()[1], 4.0);

    //Correct # of bins?
    TS_ASSERT_EQUALS(test_in->blocksize(), 25);
    TS_ASSERT_EQUALS(el2.dataX().size(), 26);
    TS_ASSERT_EQUALS(el2.dataY()->size(), 25);
    TS_ASSERT_EQUALS(el2.dataE()->size(), 25);

    //# of events per bin was doubled
    TS_ASSERT_EQUALS( (*el2.dataY())[0], 2);
    TS_ASSERT_EQUALS( (*el2.dataY())[1], 2);
    TS_ASSERT_EQUALS( (*el2.dataY())[NUMBINS/2-2], 2); //The last bin
    TS_ASSERT_EQUALS( (*el2.dataY())[NUMBINS/2-1], 2); //The last bin

    //Now do it a second time
    rebin.setPropertyValue("InputWorkspace","test_inEvent");
    rebin.setPropertyValue("OutputWorkspace","test_inEvent");
    rebin.setPropertyValue("Params", "0.0,4.0,100");
    TS_ASSERT(rebin.execute());
    const EventList el3(test_in->getEventList(2));
    TS_ASSERT_EQUALS(el3.dataX().size(), 26);


    //Now do it a third time
    rebin.setPropertyValue("InputWorkspace","test_inEvent");
    rebin.setPropertyValue("OutputWorkspace","test_inEvent");
    rebin.setPropertyValue("Params", "-100.0,4.0,100");
    TS_ASSERT(rebin.execute());
    const EventList el4(test_in->getEventList(2));
    TS_ASSERT_EQUALS(el4.dataX().size(), 51);

    EventWorkspace_const_sptr outWS = boost::dynamic_pointer_cast<const EventWorkspace>(AnalysisDataService::Instance().retrieve("test_inEvent"));
    TS_ASSERT_EQUALS(outWS->dataY(0).size(), 50);

    AnalysisDataService::Instance().remove("test_inEvent");
  }


  void dotestEventWorkspace_DifferentOutputWorkspace(EventType eventType)
  {
    EventWorkspace_sptr test_in = CreateEventWorkspace(NUMBINS, NUMPIXELS, eventType);
    AnalysisDataService::Instance().add("test_inEvent2", test_in);

    const EventList el(test_in->getEventList(1));
    //Correct # of bins?
    TS_ASSERT_EQUALS(test_in->blocksize(), NUMBINS-1);
    TS_ASSERT_EQUALS( el.dataX().size(), NUMBINS);
    TS_ASSERT_EQUALS( el.dataY()->size(), NUMBINS-1);
    TS_ASSERT_EQUALS( el.dataE()->size(), NUMBINS-1);
    //Good histogram
    TS_ASSERT_EQUALS( el.dataX()[0], 0);
    TS_ASSERT_EQUALS( el.dataX()[1], BIN_DELTA);
    //Because of the way the events were faked, bins 0 to pixel-1 are 0, rest are 1
    TS_ASSERT_EQUALS( (*el.dataY())[0], 1);
    TS_ASSERT_EQUALS( (*el.dataY())[1], 1);
    TS_ASSERT_EQUALS( (*el.dataY())[NUMBINS-2], 1); //The last bin

    Rebin rebin;
    rebin.initialize();
    rebin.setPropertyValue("InputWorkspace","test_inEvent2");
    rebin.setPropertyValue("OutputWorkspace","test_out2");
    rebin.setPropertyValue("Params", "0.0,4.0,100");
       
    TS_ASSERT(rebin.execute());
    TS_ASSERT(rebin.isExecuted());

    MatrixWorkspace_sptr test_out = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("test_out2"));
    TS_ASSERT_EQUALS(test_out->getNumberHistograms(), NUMPIXELS);
    TS_ASSERT_EQUALS(test_out->readX(0)[0], 0.0);
    TS_ASSERT_EQUALS(test_out->readX(0)[1], 4.0);
    TS_ASSERT_EQUALS(test_out->readX(0)[25], 100.0);
    //Correct # of bins?
    TS_ASSERT_EQUALS(test_out->blocksize(), 25);
    TS_ASSERT_EQUALS(test_out->readX(0).size(), 26);
    TS_ASSERT_EQUALS(test_out->readY(0).size(), 25);
    TS_ASSERT_EQUALS(test_out->readE(0).size(), 25);
    //Number of events was doubled
    TS_ASSERT_EQUALS(test_out->readY(0)[0], 2);
    TS_ASSERT_EQUALS(test_out->readY(0)[1], 2);
    TS_ASSERT_EQUALS(test_out->readY(0)[24], 2);
    //E is sqrt(2)
    TS_ASSERT_DELTA(test_out->readE(0)[0],std::sqrt(test_out->readY(0)[0]), 0.0001);
    //Should say it is histogram data
    TS_ASSERT(test_out->isHistogramData());


    //Axes?
    TS_ASSERT_EQUALS(test_in->axes(), test_out->axes());
    //Match the workspace index->spectraNo map.
    for (int i=0; i<NUMPIXELS; i++)
      TS_ASSERT_EQUALS(test_in->getAxis(1)->spectraNo(i), test_out->getAxis(1)->spectraNo(i));

    AnalysisDataService::Instance().remove("test_inEvent2");

  }
    
  void testEventWorkspace_SameOutputWorkspace()
  {
    do_testEventWorkspace_SameOutputWorkspace(TOF);
  }

  void testEventWorkspace_SameOutputWorkspace_weighted()
  {
    do_testEventWorkspace_SameOutputWorkspace(WEIGHTED);
  }

  void testEventWorkspace_SameOutputWorkspace_weightedNoTime()
  {
    do_testEventWorkspace_SameOutputWorkspace(WEIGHTED_NOTIME);
  }

  void testEventWorkspace_DifferentOutputWorkspace()
  {
    dotestEventWorkspace_DifferentOutputWorkspace(TOF);
  }

  void testEventWorkspace_Weighted_And_DifferentOutputWorkspace()
  {
    dotestEventWorkspace_DifferentOutputWorkspace(WEIGHTED);
  }

  void testEventWorkspace_WeightedNoTime_And_DifferentOutputWorkspace()
  {
    dotestEventWorkspace_DifferentOutputWorkspace(WEIGHTED_NOTIME);
  }



private:

  EventWorkspace_sptr CreateEventWorkspace(int numbins, int numpixels, EventType theType)
  {
    EventWorkspace_sptr retVal(new EventWorkspace);
    retVal->initialize(numpixels,numbins,numbins-1);

    //Create the original X axis to histogram on.
    //Create the x-axis for histogramming.
    Kernel::cow_ptr<MantidVec> axis;
    MantidVec& xRef = axis.access();
    xRef.resize(numbins);
    for (int i = 0; i < numbins; ++i)
      xRef[i] = i*BIN_DELTA;


    //Make up some data for each pixels
    for (int i=0; i< numpixels; i++)
    {
      //Create one event for each bin
      EventList& events = retVal->getEventListAtPixelID(i);
      for (double ie=0; ie<numbins; ie++)
      {
        //Create a list of events in order, one per bin.
        events += TofEvent((ie*BIN_DELTA)+0.5, 1);
      }
      events.switchTo(theType);
   }


    retVal->doneLoadingData();
    retVal->setAllX(axis);


    return retVal;
  }


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
