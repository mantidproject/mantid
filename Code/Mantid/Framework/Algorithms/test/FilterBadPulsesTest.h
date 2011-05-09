#ifndef FILTERBADPULSESTEST_H_
#define FILTERBADPULSESTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAlgorithms/FilterBadPulses.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;

class FilterBadPulsesTest : public CxxTest::TestSuite
{
public:
  FilterBadPulsesTest(): inputWS("testInput"), outputWS("testOutput")
  {
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void setUp_Event()
  {
    IAlgorithm_sptr loader = AlgorithmManager::Instance().create("LoadEventNexus");
    loader->initialize();
    loader->setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader->setPropertyValue("OutputWorkspace", inputWS);
    loader->execute();
    TS_ASSERT (loader->isExecuted() );
  }

  void testExec()
  {    
    //Retrieve Workspace
    this->setUp_Event();
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded    
    size_t start_num_events = WS->getNumberEvents();
    double start_proton_charge = WS->run().getProtonCharge();
    size_t num_sample_logs = WS->run().getProperties().size();
    TS_ASSERT_EQUALS( start_num_events, 112266 );
    TS_ASSERT_DELTA( start_proton_charge,26.4589, 0.0001);
    alg.setPropertyValue("InputWorkspace", inputWS);
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

    //There should be some events
    TS_ASSERT_LESS_THAN( 0, outWS->getNumberEvents());

    TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), start_num_events);
    TS_ASSERT_DELTA(  outWS->getNumberEvents() , 83434, 100);

    //Proton charge is lower
    TS_ASSERT_EQUALS( outWS->run().getProperties().size(), num_sample_logs);
    TS_ASSERT_DELTA( outWS->run().getProtonCharge(), 20.576,0.001 );

    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(outputWS);
  }

private:
  std::string inputWS;
  std::string outputWS;
  FilterBadPulses alg; 
  EventWorkspace_sptr WS;
};

#endif
