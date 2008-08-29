#ifndef LOADNEXUSTEST_H_
#define LOADNEXUSTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidNexus/LoadNeXus.h"
// These includes seem to make the difference between initialization of the
// workspace names (workspace2D/1D etc), instrument classes and not for this test case.
#include "MantidDataObjects/WorkspaceSingleValue.h" 
#include "MantidDataHandling/LoadInstrument.h" 
//

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

class LoadNeXusTest : public CxxTest::TestSuite
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
  
    outputSpace="LoadNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
        
    
    // Now set it...
    // specify name of file to load workspace from
    inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    algToBeTested.setPropertyValue("FileName", inputFile);
   
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = algToBeTested.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile)); 
  
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );
    //
    //  test workspace, copied from LoadMuonNexusTest.h
    Workspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieve(outputSpace));    
    Workspace2D_sptr output2D = boost::dynamic_pointer_cast<Workspace2D>(output);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);

  }

  void testExec2()
  {
    // same tests but with 2nd Muon Nexus file that contains 4 periods
    if ( !alg2.isInitialized() ) alg2.initialize();
  
    alg2.setPropertyValue("OutputWorkspace", "LoadNexusTest2");     
    
    // specify name of file to load workspace from
    inputFile = "../../../../Test/Nexus/emu00006475.nxs";
    alg2.setPropertyValue("FileName", inputFile);
   
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = alg2.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile)); 
  
    TS_ASSERT_THROWS_NOTHING(alg2.execute());    
    TS_ASSERT( alg2.isExecuted() );    

  }

  
private:
  LoadNeXus algToBeTested,alg2;
  std::string inputFile;
  std::string outputSpace;

};
  
#endif /*LOADNEXUSTEST_H_*/
