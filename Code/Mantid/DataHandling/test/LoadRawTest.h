#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadRaw.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidAPI/AnalysisDataService.h"

using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using Mantid::DataObjects::Workspace2D;

class LoadRawTest : public CxxTest::TestSuite
{
public:
  void testInit()
  {
    StatusCode status = loader.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExec()
  {
    if ( !loader.isInitialized() ) loader.initialize();

    // Path to test input file assumes Test directory checked out from SVN
    inputFile = "../../../../Test/Data/HET15869.RAW";
    loader.setProperty("Filename", inputFile);

    outputSpace = "outer";
    loader.setProperty("OutputWorkspace", outputSpace);    
    
    std::string result;
    TS_ASSERT_THROWS_NOTHING( result = loader.getPropertyValue("Filename") )
    TS_ASSERT( ! result.compare(inputFile));
    
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
