#ifndef ALIGNDETECTORSTEST_H_
#define ALIGNDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/AlignDetectors.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidDataObjects/EventWorkspace.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class AlignDetectorsTest : public CxxTest::TestSuite
{
public:
  AlignDetectorsTest()
  {
  }

  /** Setup for loading raw data */
  void setUp_Raw()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "ISIS");
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","HRP38692.raw");
    inputWS = "rawWS";
    loader.setPropertyValue("OutputWorkspace",inputWS);
    loader.setProperty("SpectrumMin",320);
    loader.setProperty("SpectrumMax",330);
    loader.execute();
  }

  void testTheBasics()
  {
    TS_ASSERT_EQUALS( align.name(), "AlignDetectors" );
    TS_ASSERT_EQUALS( align.version(), 1 );
    TS_ASSERT_EQUALS( align.category(), "Diffraction" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    std::vector<Property*> props = align.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 4 );
  }

  /** Test alignDetectors for a Workspace2D loaded from some
   * raw data file.
   */
  void testExecWorkspace2D()
  {
    setUp_Raw();
    if ( !align.isInitialized() ) align.initialize();

    TS_ASSERT_THROWS( align.execute(), std::runtime_error );

    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "aligned";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "hrpd_new_072_01.cal");

    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    boost::shared_ptr<MatrixWorkspace> inWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    boost::shared_ptr<MatrixWorkspace> outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));

    TS_ASSERT_EQUALS( outWS->getAxis(0)->unit()->unitID(), "dSpacing" );
    TS_ASSERT_EQUALS( &(outWS->spectraMap()), &(inWS->spectraMap()) );
    TS_ASSERT_EQUALS( outWS->size(), inWS->size() );
    TS_ASSERT_EQUALS( outWS->blocksize(), inWS->blocksize() );

    TS_ASSERT_DELTA( outWS->dataX(2)[50], 0.7223, 0.0001 );
    TS_ASSERT_EQUALS( outWS->dataY(2)[50], inWS->dataY(1)[50] );
    TS_ASSERT_EQUALS( outWS->dataY(2)[50], inWS->dataY(1)[50] );
    AnalysisDataService::Instance().remove(outputWS);
  }




  /** Setup for loading raw data */
  void setUp_Event()
  {
    Mantid::Kernel::ConfigService::Instance().setString("default.facility", "SNS");
    inputWS = "eventWS";
    LoadEventPreNeXus loader;
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


  void testExecEventWorkspace_sameOutputWS()
  {
    this->setUp_Event();

    //Retrieve Workspace
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded
    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    double a_tof = WS->getEventList(0).getEvents()[0].tof();

    //Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    //Set all the properties
    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = inputWS;
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "refl_fake.cal");

    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    // WS hasn;t changed

    //Things that haven't changed
    TS_ASSERT_EQUALS( start_blocksize, WS->blocksize());
    TS_ASSERT_EQUALS( num_events, WS->getNumberEvents() );
    //But a TOF changed.
    TS_ASSERT_DIFFERS(a_tof, WS->getEventList(0).getEvents()[0].tof());
  }


  void testExecEventWorkspace_differentOutputWS()
  {
    this->setUp_Event();

    //Retrieve Workspace
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded

    //Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    //Set all the properties
    align.setPropertyValue("InputWorkspace", inputWS);
    const std::string outputWS = "eventWS_changed";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "refl_fake.cal");

    TS_ASSERT_THROWS_NOTHING( align.execute() );
    TS_ASSERT( align.isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberEvents(), WS->getNumberEvents() );
    //But a TOF changed.
    TS_ASSERT_DIFFERS( outWS->getEventList(0).getEvents()[0].tof(), WS->getEventList(0).getEvents()[0].tof());
  }

private:
  AlignDetectors align;
  std::string inputWS;
  EventWorkspace_sptr WS;


};

#endif /*ALIGNDETECTORSTEST_H_*/
