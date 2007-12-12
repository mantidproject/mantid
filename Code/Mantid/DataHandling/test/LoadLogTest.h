#ifndef LOADLOGTEST_H_
#define LOADLOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadLog.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/Instrument.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/Exception.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/Algorithm.h"
#include "Component.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <vector>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using Mantid::DataObjects::Workspace2D;

class LoadLogTest : public CxxTest::TestSuite
{
public: 
  
  LoadLogTest()
  {	
	  //initialise framework manager to allow logging
	  FrameworkManager manager;
	  manager.initialize();
  }
  void testInit()
  {
    TS_ASSERT( !loader.isInitialized() );
    TS_ASSERT_THROWS_NOTHING(loader.initialize());    
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExecWithSingleLogFile()
  {
    if ( !loader.isInitialized() ) loader.initialize();

	  // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HRP37129_ICPevent.txt";
    loader.setProperty("Filename", inputFile);

    outputSpace = "LoadLogTest-singleLogFile";
    loader.setProperty("OutputWorkspace", outputSpace);

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( ! result.compare(outputSpace));


	  TS_ASSERT_THROWS_NOTHING(loader.execute());    

    TS_ASSERT( loader.isExecuted() );    
    
    // Get back the saved workspace
    AnalysisDataService *data = AnalysisDataService::Instance();
    Workspace *output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
    
	  Sample& sample = output->getSample();

    Property *l_property = sample.getLogData(inputFile);
    TimeSeriesProperty<std::string> *l_timeSeries = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);

    std::string timeSeriesString = l_timeSeries->value();

    // test that log file read in ok
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,26), "2007-Nov-16 13:25:48   END" );
  }


  void testExecWithRawDatafile()
  {
    //if ( !loader.isInitialized() ) loader.initialize();

    LoadLog loaderRawFile;
    loaderRawFile.initialize();

	  // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HRP37125.RAW";
    loaderRawFile.setProperty("Filename", inputFile);

    outputSpace = "LoadLogTest-rawdatafile";
    loaderRawFile.setProperty("OutputWorkspace", outputSpace);

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("OutputWorkspace") )
    TS_ASSERT( ! result.compare(outputSpace));


	  TS_ASSERT_THROWS_NOTHING(loaderRawFile.execute());    

    TS_ASSERT( loaderRawFile.isExecuted() );    
    
    // Get back the saved workspace
    AnalysisDataService *data = AnalysisDataService::Instance();
    Workspace *output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
   
	  Sample& sample = output->getSample(); 

    // obtain the expected log files which should be in the same directory as the raw datafile

    Property *l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_ICPevent.txt") );
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);
    std::string timeSeriesString = l_timeSeriesString->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,28), "2007-Nov-13 15:19:13   BEGIN" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_cphs_6.txt") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_PROP3.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_SE_He_Level.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,24), "2007-Nov-13 15:17:08  -1" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_TEMP1.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,23), "2007-Nov-13 15:16:20  0" );
  }
  

  void testFinal()
  {
    if ( !loader.isInitialized() ) loader.initialize();
    
    // The final() method doesn't do anything at the moment, but test anyway
    TS_ASSERT_THROWS_NOTHING(loader.finalize());    
    TS_ASSERT( loader.isFinalized() );
  }


  
private:
  LoadLog loader;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;
  
};
  
#endif /*LOADLOGTEST_H_*/
