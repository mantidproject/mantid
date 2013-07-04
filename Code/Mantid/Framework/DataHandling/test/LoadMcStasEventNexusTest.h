#ifndef LOADMCSTASEVENTNEXUSTESTTEST_H_
#define LOADMCSTASEVENTNEXUSTESTTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/LoadMcStasEventNexus.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

//
// Test checks if number  of workspace equals one
// Test checks if number  getNumberHistograms = 16384. (128x128=16384 pixels in detector)
// 
class LoadMcStasEventNexusTest : public CxxTest::TestSuite
{
public: 
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  

  void testExec()
  {
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    outputSpace="LoadMcStasEventNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
        
    
    // Now set it... 
    // specify name of file to load workspace from
    inputFile = "mcstas_event.h5";
    algToBeTested.setPropertyValue("Filename", inputFile);
 
   
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );
    //
    //  test workspace created by LoadMcStasEventNexus
    WorkspaceGroup_sptr output = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(outputSpace);
    TS_ASSERT_EQUALS( output->getNumberOfEntries(), 1); // 1 NXdata groups
    //
    //
    MatrixWorkspace_sptr outputItem1 = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace+"_1");
    TS_ASSERT_EQUALS( outputItem1->getNumberHistograms(), 16384);  }


 
private:
  LoadMcStasEventNexus algToBeTested;
  std::string inputFile;
  std::string outputSpace;

};

#endif /*LOADMCSTASEVENTNEXUSTESTTEST_H_*/
