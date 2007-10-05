#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/LoadRaw.h"
#include "../../Kernel/inc/WorkspaceFactory.h"
#include "../../DataObjects/inc/Workspace2D.h"

class LoadRawTest : public CxxTest::TestSuite
{
public: 
  
  void testInit()
  {
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    TS_ASSERT( ! status.isFailure() );
    std::string result;
    status = loader.getProperty("Filename", result);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( ! result.compare("HET15869.RAW"));

    Mantid::WorkspaceFactory *factory = Mantid::WorkspaceFactory::Instance();
    factory->registerWorkspace("Workspace2D", Mantid::Workspace2D::create );
    status = loader.setProperty("OutputWorkspace","outer");
    TS_ASSERT( ! status.isFailure() );
    
    status = loader.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExec()
  {
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    status = loader.initialize();
    status = loader.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isExecuted() );    
    
    // When workspace exists, add tests to ensure it's being filled correctly
  }
  
  void testFinal()
  {
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    status = loader.initialize();
    status = loader.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isFinalized() );
  }
  
private:
  Mantid::LoadRaw loader;
  
};
  
#endif /*LOADRAWTEST_H_*/
