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
#include "Poco/Path.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
using namespace Mantid::DataObjects;

//
// test does:
//          load workspace from Muon(1) file emu..73.nxs using LoadNexus
//          load workspace from Muon(1) file emu..75.nxs using LoadNexus
//
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
    //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
    if ( !algToBeTested.isInitialized() ) algToBeTested.initialize();
  
    outputSpace="LoadNexusTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // Should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(),std::runtime_error);
        
    
    // Now set it...
    // specify name of file to load workspace from
    inputFile = "emu00006473.nxs";
    algToBeTested.setPropertyValue("FileName", inputFile);
   
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());    
    TS_ASSERT( algToBeTested.isExecuted() );
    //
    //  test workspace, copied from LoadMuonNexusTest.h
    MatrixWorkspace_sptr output;
    (output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));

	// MatrixWorkspace_sptr output;
    //TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_1"))); 

    MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<MatrixWorkspace>(output);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006473.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->dataY(11)[686], 81);

#endif /*_WIN64*/
  }

  void testExec2()
  {
  //This test does not compile on Windows64 as is does not support HDF4 files
#ifndef _WIN64
	  //multi period test
    // same tests but with 2nd Muon Nexus file that contains 4 periods
    if ( !alg2.isInitialized() ) alg2.initialize();
  
    outputSpace="LoadNexusTest2";
    alg2.setPropertyValue("OutputWorkspace", outputSpace);     
    
    // specify name of file to load workspace from
    inputFile = "emu00006475.nxs";
    alg2.setPropertyValue("FileName", inputFile);
   
    TS_ASSERT_THROWS_NOTHING(alg2.execute());    
    TS_ASSERT( alg2.isExecuted() );    
    //
    // Copied from LoadMuonTest.h
    // Test workspace data - should be 4 separate workspaces for this 4 period file
    //
   // MatrixWorkspace_sptr output,output2,output3,output4;
	 MatrixWorkspace_sptr output,output1,output2,output3,output4;
   // TS_ASSERT_THROWS_NOTHING(output = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace)));
	WorkspaceGroup_sptr work_grpout;
	  (work_grpout = boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve(outputSpace)));
	  (output1 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_1")));
    (output2 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_2")));
    (output3 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_3")));
    (output4 = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(outputSpace+"_4")));
    
	MatrixWorkspace_sptr output2D = boost::dynamic_pointer_cast<MatrixWorkspace>(output1);
    MatrixWorkspace_sptr output2D2 = boost::dynamic_pointer_cast<MatrixWorkspace>(output2);
    // Should be 32 for file inputFile = "../../../../Test/Nexus/emu00006475.nxs";
    TS_ASSERT_EQUALS( output2D->getNumberHistograms(), 32);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->dataX(3)) == (output2D->dataX(31)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->dataY(5).size(), output2D->dataY(17).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D2->dataY(8)[502], 121);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D2->dataE(8)[502], 11);
    // Check that the time is as expected from bin boundary update
    TS_ASSERT_DELTA( output2D->dataX(11)[687], 10.738,0.001);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS( output2->getAxis(0)->unit()->unitID(), "TOF" );
    
#endif /*_WIN64*/
  }
 
private:
  LoadNexus algToBeTested,alg2;
  std::string inputFile;
  std::string outputSpace;

};

#endif /*LOADNEXUSTEST_H_*/
