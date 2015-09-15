#ifndef LOADRAWSAVENXSLOADNXSTEST_H_
#define LOADRAWSAVENXSLOADNXSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadRaw3.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidGeometry/IComponent.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadNexus.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidDataHandling/SaveNexusProcessed.h"
#include <cxxtest/TestSuite.h>
#include <fstream>
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadRawSaveNxsLoadNxsTest : public CxxTest::TestSuite
{
public:

  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }

void testExecOnLoadraw()
{
    // use SaveNexusProcessed to build a test file to load
    // for this use LoadRaw
    std::string inputFile = "CSP78173.raw";
    TS_ASSERT_THROWS_NOTHING( loader.initialize());
    TS_ASSERT( loader.isInitialized() );
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "csp78173";
    loader.setPropertyValue("OutputWorkspace", outputSpace);

    TS_ASSERT_THROWS_NOTHING(loader.execute());
    TS_ASSERT( loader.isExecuted() );

    //
    // get workspace
    //
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace) );
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    if ( !saveNexusP.isInitialized() ) saveNexusP.initialize();

    //
    saveNexusP.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save workspace to
    outputFile = "testSaveLoadrawCSP.nxs";
    remove(outputFile.c_str());
    std::string dataName = "spectra";
    std::string title = "Workspace from Loadraw CSP78173";
    saveNexusP.setPropertyValue("FileName", outputFile);
    outputFile = saveNexusP.getPropertyValue("Filename");
    //saveNexusP.setPropertyValue("EntryName", entryName);
    saveNexusP.setPropertyValue("Title", title);

    TS_ASSERT_THROWS_NOTHING(saveNexusP.execute());
    TS_ASSERT( saveNexusP.isExecuted() );

}

  void testExecRaw()
  {
    // test LoadNexusProcessed reading the data from SNP on Loadraw CSP78173

    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();

    // specify name of workspace
    myOutputSpace="testLNP3";
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("OutputWorkspace", myOutputSpace));
    // file name to load
    inputFile = outputFile;
    entryNumber=1;
    TS_ASSERT_THROWS_NOTHING(algToBeTested.setPropertyValue("FileName", inputFile));
    algToBeTested.setProperty("EntryNumber", entryNumber);

    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("FileName") );
    TS_ASSERT( ! result.compare(inputFile));
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("OutputWorkspace") );
    TS_ASSERT( ! result.compare(myOutputSpace));
    int res=-1;
    TS_ASSERT_THROWS_NOTHING( res = algToBeTested.getProperty("EntryNumber") );
    TS_ASSERT( res==entryNumber);

    // Test that nexus precessed file is successfully loaded 
    // The loading of the current version of nexus processed is tested here.
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT( algToBeTested.isExecuted() );

    // Get back the saved workspace
   MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(myOutputSpace));
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // set to 4 for CSP78173
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 4);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(1)) == (output2D->dataX(3)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(1).size(), output2D->dataY(2).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(1)[14], 9.0);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->dataE(1)[14], 3.0);
    // Check that the X data is as expected
    TS_ASSERT_EQUALS( output2D->dataX(2)[777], 15550.0);

     // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output->getAxis(0)->unit()->unitID(), "TOF" );
    TS_ASSERT( ! output-> isDistribution() );
    // Check units of Y axis are "Counts"
    TS_ASSERT_EQUALS( output->YUnit(), "Counts" );

    // Check the proton charge has been set correctly
    TS_ASSERT_DELTA( output->run().getProtonCharge(), 0.8347, 0.0001 );

    //
    // check that the instrument data has been loaded, copied from LoadInstrumentTest
    //
    Instrument_const_sptr i = output->getInstrument();
    //std::cerr << "Count = " << i.use_count();
    boost::shared_ptr<const IComponent> source = i->getSource();
    TS_ASSERT( source != NULL);
  if(source != NULL )
    {
        TS_ASSERT_EQUALS( source->getName(), "source");
        TS_ASSERT_DELTA( source->getPos().Y(), 0.0,0.01);

       boost::shared_ptr<const IComponent> samplepos = i->getSample();
        TS_ASSERT_EQUALS( samplepos->getName(), "some-surface-holder");
        TS_ASSERT_DELTA( samplepos->getPos().X(), 0.0,0.01);

      boost::shared_ptr<const Detector> ptrDet103 = boost::dynamic_pointer_cast<const Detector>(i->getDetector(103));
	  if(ptrDet103!=NULL)
	  {
        TS_ASSERT_EQUALS( ptrDet103->getID(), 103);
        TS_ASSERT_EQUALS( ptrDet103->getName(), "linear-detector-pixel");
        TS_ASSERT_DELTA( ptrDet103->getPos().Z(), 12.403,0.01);
        TS_ASSERT_DELTA( ptrDet103->getPos().Y(), 0.1164,0.01);
        double d = ptrDet103->getPos().distance(samplepos->getPos());
        TS_ASSERT_DELTA(d,2.1561,0.0001);
        double cmpDistance = ptrDet103->getDistance(*samplepos);
        TS_ASSERT_DELTA(cmpDistance,2.1561,0.0001);
	  }
    }

    //----------------------------------------------------------------------
    // Tests to check that spectra-detector mapping is done correctly
    //----------------------------------------------------------------------
    TS_ASSERT_EQUALS( output2D->getSpectrum(0)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( output2D->getSpectrum(0)->getSpectrumNo(), 1);
    TS_ASSERT( output2D->getSpectrum(0)->hasDetectorID(1));


    // obtain the expected log data which was read from the Nexus file (NXlog)

    Property *l_property =output->run().getLogData( std::string("height") );
    TimeSeriesProperty<double> *l_timeSeriesDouble1 = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    std::string timeSeriesString = l_timeSeriesDouble1->value();

    //
    // Testing log data - this was failing at one time as internal format of log data changed, but now OK again
    //
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,30), "2008-Jun-17 11:10:44  -0.86526" );

    l_property = output->run().getLogData( std::string("ICPevent") );
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);
    timeSeriesString = l_timeSeriesString->value();

    //
    // Testing log data - this was failing at one time as internal format of log data changed, but now OK again
    // It was disabled, with a TODO comment, with this string: "2008-Jun-17 11:11:13  CHANGE PERIOD 12",
    // now enabled after changing 12=> 1 (and added one more space character before CHANGE).
    std::cout << timeSeriesString;
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,38), "2008-Jun-17 11:11:13   CHANGE PERIOD 1" );

    remove(outputFile.c_str());
  }



private:
  LoadNexus algToBeTested;
  std::string inputFile;
  int entryNumber;

  std::string myOutputSpace;

  SaveNexusProcessed saveNexusP;
  Mantid::DataHandling::LoadRaw3 loader;
  std::string outputSpace;
  std::string outputFile;
};

#endif /* LOADRAWSAVENXSLOADNXSTEST_H_ */
