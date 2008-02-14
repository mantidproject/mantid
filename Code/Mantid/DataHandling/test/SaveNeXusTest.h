#ifndef SAVENEXUSTEST_H_
#define SAVENEXUSTEST_H_

#include <fstream>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace1D.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidDataHandling/SaveNeXus.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
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

    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    
    Workspace_sptr localWorkspace = factory->create("Workspace1D");
    Workspace1D_sptr localWorkspace1D = boost::dynamic_pointer_cast<Workspace1D>(localWorkspace);

    localWorkspace1D->setX(lVecX);
    localWorkspace1D->setData(lVecY, lVecE);

    AnalysisDataService *data = AnalysisDataService::Instance();
    data->add("SAVENEXUSTEST-testSpace", localWorkspace);
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
    algToBeTested.setPropertyValue("FileName", outputFile);
    algToBeTested.setPropertyValue("EntryName", entryName);
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

  
private:
  SaveNeXus algToBeTested;
  std::string outputFile;
  std::string entryName;
  
};
  
#endif /*SAVENEXUSTEST_H_*/
