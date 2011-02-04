#ifndef LOADMAPPINGTABLETEST_H_
#define LOADMAPPINGTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadInstrumentFromRaw.h"
#include "MantidDataHandling/LoadMappingTable.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/ManagedWorkspace2D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMappingTableTest : public CxxTest::TestSuite
{
public:

  static LoadMappingTableTest *createSuite() { return new LoadMappingTableTest(); }
  static void destroySuite(LoadMappingTableTest *suite) { delete suite; }

	LoadMappingTableTest()
	{
	  //
	  inputFile = "HET15869.raw";
		 outputSpace = "outerWS";
		//initialise framework manager to allow logging
		Mantid::API::FrameworkManager::Instance();
		// Create the workspace and add it to the analysis data service
		work1=Mantid::API::WorkspaceFactory::Instance().create("Workspace2D",1,1,1);
		Mantid::API::AnalysisDataService::Instance().add(outputSpace, work1);
	}
	void testInit()
	{
		TS_ASSERT_THROWS_NOTHING( loader.initialize());
		TS_ASSERT( loader.isInitialized() );
	}
	void testExec()
	{
		 // Load the instrument from RAW file
		 if (!load_inst.isInitialized()) load_inst.initialize();
		 load_inst.setPropertyValue("Filename",inputFile);
		 load_inst.setPropertyValue("Workspace", outputSpace);
		 TS_ASSERT_THROWS_NOTHING(load_inst.execute());
		 //
		 // Now start test specific to LoadMappingTable
		 if ( !loader.isInitialized() ) loader.initialize();
	    // Should fail because mandatory parameter has not been set
	    TS_ASSERT_THROWS(loader.execute(),std::runtime_error);
	    // Now set it...
	    // Path to test input file assumes Test directory checked out from SVN
	    loader.setPropertyValue("Filename", inputFile);
	    loader.setPropertyValue("Workspace", outputSpace);
	    TS_ASSERT_THROWS_NOTHING(loader.execute());
	    TS_ASSERT( loader.isExecuted());

	    // Get the map from the wokspace
	    const SpectraDetectorMap& map=work1->spectraMap();

	    // Check the total number of elements in the map for HET
	    TS_ASSERT_EQUALS(map.nElements(),24964);

	    // Test one to one mapping, for example spectra 6 has only 1 pixel
	    TS_ASSERT_EQUALS(map.ndet(6),1);

	    // Test one to many mapping, for example 10 pixels contribute to spectra 2084
	    TS_ASSERT_EQUALS(map.ndet(2084),10);
	    // Check the id number of all pixels contributing
      std::vector<int> detectorgroup;
	    detectorgroup=map.getDetectors(2084);
	    std::vector<int>::const_iterator it;
	    int pixnum=101191;
	    for (it=detectorgroup.begin();it!=detectorgroup.end();it++)
	    TS_ASSERT_EQUALS(*it,pixnum++);

	    // Test with spectra that does not exist
	    // Test that number of pixel=0
	    TS_ASSERT_EQUALS(map.ndet(5),0);
	    // Test that trying to get the Detector throws.
	    std::vector<int> test = map.getDetectors(5);
	    TS_ASSERT(test.empty());
		AnalysisDataService::Instance().remove(outputSpace);
	    return;
}
private:
	LoadInstrumentFromRaw load_inst;
	LoadMappingTable loader;
	std::string inputFile;
	std::string outputSpace;
	MatrixWorkspace_sptr work1;
};
#endif /*LOADMAPPINGTABLETEST_H_*/
