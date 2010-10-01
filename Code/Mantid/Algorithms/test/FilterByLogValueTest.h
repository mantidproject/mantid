/*
 * FilterByLogValueTest.h
 *
 *  Created on: Sep 15, 2010
 *      Author: janik
 */

#ifndef FILTERBYLOGVALUETEST_H_
#define FILTERBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByLogValue.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class FilterByLogValueTest : public CxxTest::TestSuite
{
public:
  FilterByLogValueTest()
  {
  }


  /** Setup for loading raw data */
  void setUp_Event()
  {
    inputWS = "eventWS";
    LoadEventPreNeXus loader;
    loader.initialize();
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/CNCS_12772/CNCS_12772_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/Data/sns_event_prenexus/CNCS_12772/CNCS_12772_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setProperty("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "../../../../Test/Data/sns_event_prenexus/CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }



  void testExec()
  {
    std::string outputWS;
    this->setUp_Event();

    //Retrieve Workspace
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    TS_ASSERT( num_events > 0 );

    //Do the filtering now.
    FilterByLogValue * alg = new FilterByLogValue();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    outputWS = "eventWS_relative";
    alg->setPropertyValue("OutputWorkspace", outputWS);
    alg->setPropertyValue("LogName", "proton_charge");
    //We set the minimum high enough to cut out some real charge too, not just zeros.
    alg->setPropertyValue("MinimumValue", "1.33e7");
    alg->setPropertyValue("MaximumValue", "1e20");
    alg->setPropertyValue("TimeTolerance", "4e-2");

    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded

//    TimeSeriesProperty<double> * log = dynamic_cast<TimeSeriesProperty<double> * >(outWS->run().getProperty("proton_charge"));
//    std::vector<dateAndTime> times = log->timesAsVector();
//    for (std::size_t i=0; i<times.size(); i++)
//      std::cout << times[i] << " = " << DateAndTime::get_from_absolute_time(times[i]) << "\n";


    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), WS->getNumberHistograms());

    //There should be some events
    TS_ASSERT_LESS_THAN( 0, outWS->getNumberEvents());

    TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), WS->getNumberEvents());
    TS_ASSERT_DELTA(  outWS->getNumberEvents() , 547346, 100);

    //Proton charge is lower
    TS_ASSERT_LESS_THAN( outWS->run().getProtonCharge(), WS->run().getProtonCharge() );
  }





  void setUp_Event2()
  {
    inputWS = "eventWS2";
    LoadEventPreNeXus loader;
    loader.initialize();
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/CNCS_7850_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/Data/sns_event_prenexus/CNCS_7850_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setProperty("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "../../../../Test/Data/sns_event_prenexus/CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", inputWS);
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }

  void xtestExec2_slow()
  {
    std::string outputWS;
    this->setUp_Event2();

    //Retrieve Workspace
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();

    //Do the filtering now.
    FilterByLogValue * alg = new FilterByLogValue();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    outputWS = "eventWS_relative";
    alg->setPropertyValue("OutputWorkspace", outputWS);
    alg->setPropertyValue("LogName", "proton_charge");
    //We set the minimum high enough to cut out some real charge too, not just zeros.
    alg->setPropertyValue("MinimumValue", "5e6");
    alg->setPropertyValue("MaximumValue", "1e20");
    alg->setPropertyValue("TimeTolerance", "3e-3");

    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), WS->getNumberHistograms());

    //There should be some events
    TS_ASSERT_LESS_THAN( 0, outWS->getNumberEvents());

    // Not many events left: 34612
    TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), WS->getNumberEvents());
    TS_ASSERT_DELTA(  outWS->getNumberEvents() , 1093284, 100);

    //Proton charge is lower
    TS_ASSERT_LESS_THAN( outWS->run().getProtonCharge(), WS->run().getProtonCharge() );

    //Check the log entries
    TimeSeriesProperty<double> * log = dynamic_cast<TimeSeriesProperty<double> * >(outWS->run().getProperty("proton_charge"));
    TS_ASSERT(log);
    for (std::size_t i=0; i<log->realSize(); i++)
      TS_ASSERT_LESS_THAN( 0, log->nthValue(i));

  }



private:
  std::string inputWS;
  EventWorkspace_sptr WS;


};


#endif


