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
using namespace Mantid::DataObjects;

class LoadLogTest : public CxxTest::TestSuite
{
public: 
  
  LoadLogTest()
  {	
	  //initialise framework manager to allow logging
	 FrameworkManager* manager=Mantid::API::FrameworkManager::Instance();
	manager->initialize();
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
    loader.setPropertyValue("Filename", inputFile);

    outputSpace = "LoadLogTest-singleLogFile";
    loader.setPropertyValue("Workspace", outputSpace);
    // Create an empty workspace and put it in the AnalysisDataService
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    Workspace_sptr ws = factory->create("Workspace2D");
    AnalysisDataService *data = AnalysisDataService::Instance();
    TS_ASSERT_THROWS_NOTHING(data->add(outputSpace, ws));    

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(outputSpace));


	  TS_ASSERT_THROWS_NOTHING(loader.execute());    

    TS_ASSERT( loader.isExecuted() );    
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
    
	  Sample& sample = output->getSample();

    Property *l_property = sample.getLogData(inputFile);
    TimeSeriesProperty<std::string> *l_timeSeries = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);

    std::string timeSeriesString = l_timeSeries->value();

    // test that log file read in ok
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,30), "Fri Nov 16 13:25:48 2007   END" );
  }


  void testExecWithRawDatafile()
  {
    //if ( !loader.isInitialized() ) loader.initialize();

    LoadLog loaderRawFile;
    loaderRawFile.initialize();

	  // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HRP37125.RAW";
    loaderRawFile.setPropertyValue("Filename", inputFile);

    outputSpace = "LoadLogTest-rawdatafile";
    loaderRawFile.setPropertyValue("Workspace", outputSpace);
    // Create an empty workspace and put it in the AnalysisDataService
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    Workspace_sptr ws = factory->create("Workspace1D");
    AnalysisDataService *data = AnalysisDataService::Instance();
    TS_ASSERT_THROWS_NOTHING(data->add(outputSpace, ws));    

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(outputSpace));


	  TS_ASSERT_THROWS_NOTHING(loaderRawFile.execute());    

    TS_ASSERT( loaderRawFile.isExecuted() );    
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
   
	  Sample& sample = output->getSample(); 

    // obtain the expected log files which should be in the same directory as the raw datafile

    Property *l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_ICPevent.txt") );
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);
    std::string timeSeriesString = l_timeSeriesString->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,32), "Tue Nov 13 15:19:13 2007   BEGIN" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_cphs_6.txt") );
    TimeSeriesProperty<double> *l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "Tue Nov 13 15:16:20 2007  0" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_PROP3.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "Tue Nov 13 15:16:20 2007  0" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_SE_He_Level.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,28), "Tue Nov 13 15:17:08 2007  -1" );

    l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37125_TEMP1.txt") );
    l_timeSeriesDouble = dynamic_cast<TimeSeriesProperty<double>*>(l_property);
    timeSeriesString = l_timeSeriesDouble->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,27), "Tue Nov 13 15:16:20 2007  0" );
  }
  

  // Same idea as testExecWithRawDataFile() but testing on a raw file with the extension .s#
  // where # is some integer ranging from 01,02,...,99 I believe
  void testExecWithRawDatafile_s_type()
  {
    //if ( !loader.isInitialized() ) loader.initialize();

    LoadLog loaderRawFile;
    loaderRawFile.initialize();

	  // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HRP37129.S02";
    loaderRawFile.setPropertyValue("Filename", inputFile);

    outputSpace = "LoadLogTest-rawdatafile_so_type";
    loaderRawFile.setPropertyValue("Workspace", outputSpace);
    // Create an empty workspace and put it in the AnalysisDataService
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    Workspace_sptr ws = factory->create("Workspace1D");
    AnalysisDataService *data = AnalysisDataService::Instance();
    TS_ASSERT_THROWS_NOTHING(data->add(outputSpace, ws));    

	  std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));

    TS_ASSERT_THROWS_NOTHING( result = loaderRawFile.getPropertyValue("Workspace") )
    TS_ASSERT( ! result.compare(outputSpace));


	  TS_ASSERT_THROWS_NOTHING(loaderRawFile.execute());    

    TS_ASSERT( loaderRawFile.isExecuted() );    
    
    // Get back the saved workspace
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = data->retrieve(outputSpace));
   
	  Sample& sample = output->getSample(); 

    // obtain the expected log files which should be in the same directory as the raw datafile

    Property *l_property = sample.getLogData( std::string("../../../../Test/Data/HRP37129_ICPevent.txt") );
    TimeSeriesProperty<std::string> *l_timeSeriesString = dynamic_cast<TimeSeriesProperty<std::string>*>(l_property);
    std::string timeSeriesString = l_timeSeriesString->value();
    TS_ASSERT_EQUALS( timeSeriesString.substr(0,30), "Fri Nov 16 13:25:48 2007   END" );
  }

  
private:
  LoadLog loader;
  std::string inputFile;
  std::string outputSpace;
  std::string inputSpace;
  
};
  
#endif /*LOADLOGTEST_H_*/
