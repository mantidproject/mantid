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
#include "MantidNexus/SaveNeXus.h"
#include "MantidNexus/LoadMuonNexus.h"
#include "MantidNexus/LoadNeXus.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class SaveNeXusTest : public CxxTest::TestSuite
{
public: 
  
  SaveNeXusTest()
  {
    // create dummy 1D-workspace
    
    std::vector<double> lVecX; for(double d=0.0; d<0.95; d=d+0.1) lVecX.push_back(d);
    std::vector<double> lVecY; for(double d=0.0; d<0.95; d=d+0.1) lVecY.push_back(d);
    std::vector<double> lVecE; for(double d=0.0; d<0.95; d=d+0.1) lVecE.push_back(d);
   
    Workspace_sptr localWorkspace = Workspace_sptr(new Workspace1D);
    
    Workspace1D_sptr localWorkspace1D = boost::dynamic_pointer_cast<Workspace1D>(localWorkspace);
   
    localWorkspace1D->setX(lVecX);
    localWorkspace1D->setData(lVecY, lVecE);

    AnalysisDataService::Instance().add("SAVENEXUSTEST-testSpace", localWorkspace);
}
  
  
  void testInit()
  {
    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT( algToBeTested.isInitialized() );
  }
  
  
  void testExec()
  {
    
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    algToBeTested.setPropertyValue("InputWorkspace", "SAVENEXUSTEST-testSpace");     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
        
    
    // Now set it...
    // specify name of file to save 1D-workspace to
    outputFile = "testOfSaveNeXus.nxs";
    entryName = "test";
	dataName = "spectra";
    algToBeTested.setPropertyValue("FileName", outputFile);
    algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("DataName", dataName);
    remove(outputFile.c_str());
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(outputFile)); 
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") )
    TS_ASSERT( ! result.compare(entryName)); 
    
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );    

    remove(outputFile.c_str());
   
  }
void testExecOnMuon()
  {
    LoadNeXus nxLoad;
	std::string outputSpace,inputFile;
    nxLoad.initialize();
    // Now set required filename and output workspace name
    inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    nxLoad.setPropertyValue("FileName", inputFile);
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
  
    algToBeTested.setPropertyValue("InputWorkspace", outputSpace);
    // specify name of file to save 1D-workspace to
    outputFile = "testOfSaveNeXusMuon.nxs";
    entryName = "entry4";
	dataName = "spectra";
    algToBeTested.setPropertyValue("FileName", outputFile);
    algToBeTested.setPropertyValue("EntryName", entryName);
    algToBeTested.setPropertyValue("DataName", dataName);
    remove(outputFile.c_str());
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") );
    TS_ASSERT( ! result.compare(outputFile));
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("EntryName") );
    TS_ASSERT( ! result.compare(entryName));
    
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute()); 
    TS_ASSERT( algToBeTested.isExecuted() );  

    remove(outputFile.c_str());
	// test writing two entries into one nexus file with different names.
	entryName="entry5";
    algToBeTested.setPropertyValue("EntryName", entryName);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute()); 
    TS_ASSERT( algToBeTested.isExecuted() );
    remove(outputFile.c_str());
   
  }

  
private:
  SaveNeXus algToBeTested;
  std::string outputFile;
  std::string entryName;
  std::string dataName;
  
};
  
#endif /*SAVENEXUSTEST_H_*/
