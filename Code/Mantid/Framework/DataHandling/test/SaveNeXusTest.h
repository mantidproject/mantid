#ifndef SAVENEXUSTEST_H_
#define SAVENEXUSTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
//

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/SaveNeXus.h"
#include "MantidDataHandling/LoadMuonNexus.h"
#include "MantidDataHandling/LoadNeXus.h"
#include <Poco/Path.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class SaveNeXusTest : public CxxTest::TestSuite
{
public: 
  
  
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  
  
void testExecOnMuon()
  {
  //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    LoadNexus nxLoad;
    std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "emu00006473.nxs";
    nxLoad.setPropertyValue("Filename", inputFile);
    outputSpace="outer";
    nxLoad.setPropertyValue("OutputWorkspace", outputSpace);     
    //
    // Test execute to read file and populate workspace
    //
    TS_ASSERT_THROWS_NOTHING(nxLoad.execute());    
    TS_ASSERT( nxLoad.isExecuted() );    
    //
    // get workspace
    //
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    //
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    // specify parameters to algorithm
    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    outputFile = "testOfSaveNexus.nxs";
    algToBeTested.setPropertyValue("Filename", outputFile);
    outputFile = algToBeTested.getPropertyValue("Filename");
    title="Testing SaveNexus with Muon data";
    algToBeTested.setPropertyValue("Title", title);
    // comment line below to check the contents of the o/p file manually
    remove(outputFile.c_str());
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Title") );
    TS_ASSERT( ! result.compare(title));
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("InputWorkspace") );
    TS_ASSERT( ! result.compare(outputSpace));
    
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute()); 
    TS_ASSERT( algToBeTested.isExecuted() );  

    // test writing two entries to one nexus file
    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute()); 
    TS_ASSERT( algToBeTested.isExecuted() );
    remove(outputFile.c_str());
   
#endif /*_WIN64*/  
  }

  
private:
  SaveNexus algToBeTested;
  std::string outputFile;
  std::string title;
  int entryNumber;
  
};
#endif /*SAVENEXUSTEST_H_*/
