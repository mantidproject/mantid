#ifndef MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_
#define MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadILLIndirect.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::API;
using Mantid::DataHandling::LoadILLIndirect;

class LoadILLIndirectTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LoadILLIndirectTest *createSuite() { return new LoadILLIndirectTest(); }
  static void destroySuite( LoadILLIndirectTest *suite ) { delete suite; }

	LoadILLIndirectTest() :
		m_dataFile("ILLIN16B_034745.nxs") {
	}
  	
	void testInit()
	{
		LoadILLIndirect loader;
		TS_ASSERT_THROWS_NOTHING( loader.initialize() )
		TS_ASSERT( loader.isInitialized() )
	}
  
  	void testName() {
		LoadILLIndirect loader;
		TS_ASSERT_EQUALS( loader.name(), "LoadILLIndirect");
	}

	void testVersion() {
		LoadILLIndirect loader;
		TS_ASSERT_EQUALS( loader.version(), 1);
	}
  
  
  void testExec()
  {
    // Name of the output workspace.
    std::string outWSName("LoadILLIndirectTest_OutputWS");
  
    LoadILLIndirect loader;
    TS_ASSERT_THROWS_NOTHING( loader.initialize() )
    TS_ASSERT( loader.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( loader.setPropertyValue("Filename",m_dataFile ) );
    TS_ASSERT_THROWS_NOTHING( loader.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( loader.execute(); );
    TS_ASSERT( loader.isExecuted() );
    
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName);
    TS_ASSERT(output);

    MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 2057);

    if (!output) return;
    
    // TODO: Check the results
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().clear();
  }
  

private:
	std::string m_dataFile;

};


#endif /* MANTID_DATAHANDLING_LOADILLINDIRECTTEST_H_ */
