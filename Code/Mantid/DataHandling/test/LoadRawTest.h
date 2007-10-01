#ifndef LOADRAWTEST_H_
#define LOADRAWTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/LoadRaw.h"

class LoadRawTest : public CxxTest::TestSuite
{
public: 
  
  void testInit()
  {
    Mantid::LoadRaw loader;
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    TS_ASSERT( ! status.isFailure() );
    std::string result;
    status = loader.getProperty("Filename", result);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( ! result.compare("HET15869.RAW"));
    status = loader.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isInitialized() );
  }
  
  void testExec()
  {
    Mantid::LoadRaw loader;
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    status = loader.initialize();
    status = loader.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isExecuted() );    
  }
  
  void testFinal()
  {
    Mantid::LoadRaw loader;
    Mantid::StatusCode status = loader.setProperty("Filename","HET15869.RAW");
    status = loader.initialize();
    status = loader.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( loader.isFinalized() );
  }
  

  
};
  
#endif /*LOADRAWTEST_H_*/
