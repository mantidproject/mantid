/*
 * FilterByTimeTest.h
 *
 *  Created on: Sep 14, 2010
 *      Author: janik
 */

#ifndef FILTERBYTIMETEST_H_
#define FILTERBYTIMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByTime.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class FilterByTimeTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterByTimeTest *createSuite() { return new FilterByTimeTest(); }
  static void destroySuite( FilterByTimeTest *suite ) { delete suite; }

  FilterByTimeTest()
  {
    inWS = "filterbytime_input";
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace2(4,1);
    // Add proton charge
    TimeSeriesProperty<double> * pc = new TimeSeriesProperty<double>("proton_charge");
    pc->setUnits("picoCoulomb");
    DateAndTime run_start("2010-01-01T00:00:00"); //NOTE This run_start is hard-coded in WorkspaceCreationHelper.
    for (double i=0; i<100; i++)
      pc->addValue( run_start+i, 1.0);
    ws->mutableRun().addProperty( pc );

    AnalysisDataService::Instance().add(inWS,ws);
  }

  ~FilterByTimeTest()
  {
    AnalysisDataService::Instance().clear();
  }

  /** Setup for loading raw data */
  void setUp_Event()
  {
    inputWS = "eventWS";
    LoadEventPreNexus loader;
    loader.initialize();
    std::string eventfile( "CNCS_7860_neutron_event.dat" );
    std::string pulsefile( "CNCS_7860_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setPropertyValue("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }


  void testTooManyParams()
  {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::CreateEventWorkspace(1,1);
    AnalysisDataService::Instance().addOrReplace("eventWS", ws);

    //Do the filtering now.
    FilterByTime * alg = new FilterByTime();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", "eventWS");
    alg->setPropertyValue("OutputWorkspace", "out");
    alg->setPropertyValue("StopTime", "120");
    alg->setPropertyValue("AbsoluteStartTime", "2010");
    alg->execute();
    TS_ASSERT( !alg->isExecuted() );

    alg = new FilterByTime(); alg->initialize();
    alg->setPropertyValue("InputWorkspace", "eventWS");
    alg->setPropertyValue("OutputWorkspace", "out");
    alg->setPropertyValue("StartTime", "60");
    alg->setPropertyValue("StopTime", "120");
    alg->setPropertyValue("AbsoluteStartTime", "2010");
    alg->execute();
    TS_ASSERT( !alg->isExecuted() );

    alg = new FilterByTime(); alg->initialize();
    alg->setPropertyValue("InputWorkspace", "eventWS");
    alg->setPropertyValue("OutputWorkspace", "out");
    alg->setPropertyValue("StopTime", "120");
    alg->setPropertyValue("AbsoluteStartTime", "2010");
    alg->setPropertyValue("AbsoluteStopTime", "2010-03");
    alg->execute();
    TS_ASSERT( !alg->isExecuted() );
  }

  void testExecEventWorkspace_relativeTime_and_absolute_time()
  {
    std::string outputWS;
    this->setUp_Event();

    //Retrieve Workspace
    WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inputWS);
    TS_ASSERT( WS ); //workspace is loaded

    //Do the filtering now.
    FilterByTime * alg = new FilterByTime();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    outputWS = "eventWS_relative";
    alg->setPropertyValue("OutputWorkspace", outputWS);
    //Get 1 minute worth
    alg->setPropertyValue("StartTime", "60");
    alg->setPropertyValue("StopTime", "120");

    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT( outWS ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), WS->getNumberHistograms());
    //Things that changed
    TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), WS->getNumberEvents() );

    //Proton charge is lower
    TS_ASSERT_LESS_THAN( outWS->run().getProtonCharge(), WS->run().getProtonCharge() );

    //-------------- Absolute time filtering --------------------
    alg = new FilterByTime();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    outputWS = "eventWS_absolute";
    alg->setPropertyValue("OutputWorkspace", outputWS);
    //Get 1 minutes worth, starting at minute 1
    alg->setPropertyValue("AbsoluteStartTime", "2010-03-25T16:09:37.46");
    alg->setPropertyValue("AbsoluteStopTime", "2010-03-25T16:10:37.46");
    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    EventWorkspace_sptr outWS2;
    outWS2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT( outWS2 ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS2->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS2->getNumberHistograms(), WS->getNumberHistograms());
    //Things that changed
    TS_ASSERT_LESS_THAN( outWS2->getNumberEvents(), WS->getNumberEvents() );
    TS_ASSERT_LESS_THAN( outWS2->run().getProtonCharge(), WS->run().getProtonCharge() );

    //------------------ Comparing both -----------------------
    //Similar total number of events
    TS_ASSERT_DELTA( outWS->getNumberEvents(), outWS2->getNumberEvents(), 10 );
    int count = 0;
    for (size_t i=0; i<outWS->getNumberHistograms(); i++)
    {
      double diff = fabs(double(outWS->getEventList(i).getNumberEvents() - outWS2->getEventList(i).getNumberEvents()));
      //No more than 2 events difference because of rounding to 0.01 second
      TS_ASSERT_LESS_THAN( diff, 3);
      if (diff > 3) count++;
      if (count > 50) break;
    }

    //Almost same proton charge
    TS_ASSERT_DELTA( outWS->run().getProtonCharge(), outWS2->run().getProtonCharge(), 0.01 );


  }

  void test_same_output_and_input_workspaces()
  {
    FilterByTime alg;
    alg.initialize();
    alg.setProperty("InputWorkspace",inWS);
    alg.setProperty("OutputWorkspace",inWS);
    alg.setProperty("StartTime", 20.5 );
    alg.setProperty("StopTime", 70.5 );
    TS_ASSERT( alg.execute() );

    EventWorkspace_sptr outWS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWS);
    TS_ASSERT_LESS_THAN( 0, outWS->getNumberEvents() );

  }

private:
  std::string inputWS;
  EventWorkspace_sptr WS;
  std::string inWS;
};


#endif /* FILTERBYTIMETEST_H_ */


