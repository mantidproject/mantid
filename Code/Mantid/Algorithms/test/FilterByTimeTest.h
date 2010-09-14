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
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class FilterByTimeTest : public CxxTest::TestSuite
{
public:
  FilterByTimeTest()
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
//    loader.setPropertyValue("InstrumentFilename", "../../../../Test/Instrument/CNCS_Definition.xml");
    loader.execute();
    TS_ASSERT (loader.isExecuted() );
  }


  void testExecEventWorkspace_differentOutputWS()
  {
    this->setUp_Event();

    //Retrieve Workspace
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();

    //Do the filtering now.
    FilterByTime alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "eventWS_changed";
    alg.setPropertyValue("OutputWorkspace", outputWS);

    alg.execute();
    TS_ASSERT( alg.isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), WS->getNumberHistograms());

    //TODO: More tests of pulse time
    //TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), WS->getNumberEvents() );
  }

private:
  std::string inputWS;
  EventWorkspace_sptr WS;


};


#endif /* FILTERBYTIMETEST_H_ */


