#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/LoadRaw.h"
#include "../../Kernel/inc/WorkspaceFactory.h"
#include "../../DataObjects/inc/Workspace2D.h"
#include "../../Kernel/inc/AnalysisDataService.h"

using namespace Mantid;

class LoadRawTest : public CxxTest::TestSuite
{
public: 
  
  LoadRawTest()
  {
    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
    loader.setProperty("Filename", inputFile);

    // Next 2 lines should be removed when auto-registration of workspaces implemented
    WorkspaceFactory *factory = WorkspaceFactory::Instance();
    factory->subscribe<Workspace2D>("Workspace2D");
    outputSpace = "outer";
    loader.setProperty("OutputWorkspace", outputSpace);
  }
  
  void testInit()
  {
    std::string result;
    StatusCode status = loader.getProperty("Filename", result);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( ! result.compare(inputFile));

    status = loader.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();
    
    StatusCode status = loader.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isExecuted() );    
    
    // Get back the saved workspace
    AnalysisDataService *data = AnalysisDataService::Instance();
    Workspace *output;
    status = data->retrieve(outputSpace, output);
    TS_ASSERT( ! status.isFailure() );
    Workspace2D *output2D = dynamic_cast<Workspace2D*>(output);
    // Should be 2584 for file HET15869.RAW
    TS_ASSERT_EQUALS( output2D->getHistogramNumber(), 2584);
    // Check two X vectors are the same
    TS_ASSERT( (output2D->getX(99)) == (output2D->getX(1734)) );
    // Check two Y arrays have the same number of elements
    TS_ASSERT_EQUALS( output2D->getY(673).size(), output2D->getY(2111).size() );
    // Check one particular value
    TS_ASSERT_EQUALS( output2D->getY(999)[777], 9);
    // Check that the error on that value is correct
    TS_ASSERT_EQUALS( output2D->getE(999)[777], 3);
  }
  
  void testFinal()
  {
    if ( !loader.isInitialized() ) loader.initialize();
    
    // The final() method doesn't do anything at the moment, but test anyway
    StatusCode status = loader.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isFinalized() );
  }
  
private:
  LoadRaw loader;
  std::string inputFile;
  std::string outputSpace;
  
};
  
#endif /*LOADRAWTEST_H_*/
