#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/Algorithm.h"

using namespace Mantid::Kernel;

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() {}
  virtual ~ToyAlgorithm() {}
  StatusCode init() { return StatusCode::SUCCESS; }
  StatusCode exec() { return StatusCode::SUCCESS; }
  StatusCode final() { return StatusCode::SUCCESS; }
};

DECLARE_ALGORITHM(ToyAlgorithm)

class AlgorithmTest : public CxxTest::TestSuite
{
public: 
  
	void testAlgorithm()
	{
	  std::string theName = alg.name();
	  TS_ASSERT( ! theName.compare("unknown") );
	  std::string theVersion = alg.version();
	  TS_ASSERT( ! theVersion.compare("unknown") );
    TS_ASSERT( ! alg.isInitialized() );
	  TS_ASSERT( ! alg.isExecuted() );
	  TS_ASSERT( ! alg.isFinalized() );
	  // Check this points to something
    std::vector<Algorithm*> testPointer = alg.subAlgorithms();
	  TS_ASSERT(&testPointer);
	}

	void testName()
	{
	  std::string theName = alg.name();
	  TS_ASSERT( ! theName.compare("unknown") );
	}

	void testVersion()
	{
	  std::string theVersion = alg.version();
	  TS_ASSERT( ! theVersion.compare("unknown") );
	}

	void testInitialize()
	{
    StatusCode status = alg.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isInitialized() );
	}

	void testExecute()
	{
	  ToyAlgorithm myAlg;
	  StatusCode status = myAlg.execute();
    TS_ASSERT( status.isFailure() );
    TS_ASSERT( ! myAlg.isExecuted() );
    status = myAlg.initialize();
    status = myAlg.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( myAlg.isExecuted() );
	}

	void testFinalize()
	{
    ToyAlgorithm myAlg;
    StatusCode status = myAlg.finalize();
    TS_ASSERT( status.isFailure() );
    // Need to initialize otherwise the finalize method immediately returns
    myAlg.initialize();
    status = myAlg.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( myAlg.isFinalized() );
	}

//	void testCreateSubAlgorithm()
//	{
//	  // Method not implemented yet in Algorithm.cpp (need algorithm factory)
//	}

	void testSubAlgorithm()
	{
    std::vector<Algorithm*> testSubs = alg.subAlgorithms();
    // Check that the newly created vector is empty
    TS_ASSERT( testSubs.empty() );
	}
	
	void testSetProprerty()
	{
    StatusCode status = alg.setProperty("prop1");
    TS_ASSERT( ! status.isFailure() );
    status = alg.setProperty("prop2","val");
    TS_ASSERT( ! status.isFailure() );    
	}
	
	void testGetProperty()
	{
    StatusCode status = alg.setProperty("prop2","yes");
    std::string value;
    status = alg.getProperty("ghjkgh",value);
    TS_ASSERT( status.isFailure() );
    status = alg.getProperty("prop2",value);
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( ! value.compare("yes") );
    
	}
	
private:
  ToyAlgorithm alg;
	
};

#endif /*ALGORITHMTEST_H_*/
