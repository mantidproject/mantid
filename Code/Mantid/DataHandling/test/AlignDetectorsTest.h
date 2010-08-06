#ifndef ALIGNDETECTORSTEST_H_
#define ALIGNDETECTORSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/AlignDetectors.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidAPI/SpectraDetectorMap.h"

using namespace Mantid::DataHandling;
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
    LoadRaw3 loader;
    loader.initialize();
    loader.setPropertyValue("Filename","../../../../Test/Data/HRP38692.RAW");
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
    TS_ASSERT_EQUALS( align.category(), "DataHandling\\Detectors" );
  }

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    std::vector<Property*> props = align.getProperties();
    TS_ASSERT_EQUALS( static_cast<int>(props.size()), 3 );
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
    align.setPropertyValue("CalibrationFile", "../../../../Test/Data/hrpd_new_072_01.cal");

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
    LoadEventPreNeXus loader;
    loader.initialize();
    std::string eventfile( "../../../../Test/Data/sns_event_prenexus/CNCS_12772/CNCS_12772_neutron_event.dat" );
    std::string pulsefile( "../../../../Test/Data/sns_event_prenexus/CNCS_12772/CNCS_12772_pulseid.dat" );
    loader.setPropertyValue("EventFilename", eventfile);
    loader.setProperty("PulseidFilename", pulsefile);
    loader.setPropertyValue("MappingFilename", "../../../../Test/Data/sns_event_prenexus/CNCS_TS_2008_08_18.dat");
    loader.setPropertyValue("OutputWorkspace", "eventWS");
    loader.setPropertyValue("InstrumentFilename", "../../../../Test/Instrument/CNCS_Definition.xml");
    loader.execute();
  }


  void testExecEventWorkspace_sameOutputWS()
  {
    this->setUp_Event();

    //Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    //Set all the properties
    align.setPropertyValue("InputWorkspace", "eventWS");
    const std::string outputWS = "eventWS";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "../../../../Test/Data/refl_fake.cal");

    align.execute();
  }

  void testExecEventWorkspace_differentOutputWS()
  {
    this->setUp_Event();

    //Start by init'ing the algorithm
    TS_ASSERT_THROWS_NOTHING( align.initialize() );
    TS_ASSERT( align.isInitialized() );

    //Set all the properties
    align.setPropertyValue("InputWorkspace", "eventWS");
    const std::string outputWS = "alignedWS";
    align.setPropertyValue("OutputWorkspace", outputWS);
    align.setPropertyValue("CalibrationFile", "../../../../Test/Data/refl_fake.cal");

    align.execute();
  }

private:
  AlignDetectors align;
  std::string inputWS;

};

#endif /*ALIGNDETECTORSTEST_H_*/
