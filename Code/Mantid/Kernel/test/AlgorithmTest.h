#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/Algorithm.h"

class AlgorithmTest : public CxxTest::TestSuite
{
public: 
  
	void testAlgorithm()
	{
	  Mantid::Algorithm alg("Hello","1.1");
	  std::string theName = alg.name();
	  TS_ASSERT( ! theName.compare("Hello") );
	  std::string theVersion = alg.version();
	  TS_ASSERT( ! theVersion.compare("1.1") );
    TS_ASSERT( ! alg.isInitialized() );
	  TS_ASSERT( ! alg.isExecuted() );
	  TS_ASSERT( ! alg.isFinalized() );
	  // Check this points to something
    std::vector<Mantid::Algorithm*>* testPointer = alg.subAlgorithms();
	  TS_ASSERT(testPointer);
	}

	void testName()
	{
	  Mantid::Algorithm alg("Hello","1.1");
	  std::string theName = alg.name();
	  TS_ASSERT( ! theName.compare("Hello") );
	}

	void testVersion()
	{
	  Mantid::Algorithm alg("Hello","1.1");
	  std::string theVersion = alg.version();
	  TS_ASSERT( ! theVersion.compare("1.1") );
	}

	void testInitialize()
	{
    Mantid::Algorithm alg("Hello","1.1");
    Mantid::StatusCode status = alg.initialize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isInitialized() );
	}

	void testExecute()
	{
    Mantid::Algorithm alg("Hello","1.1");
    Mantid::StatusCode status = alg.execute();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isExecuted() );
	}

	void testFinalize()
	{
    Mantid::Algorithm alg("Hello","1.1");
    // Need to initialize otherwise the finalize method immediately returns
    alg.initialize();
    Mantid::StatusCode status = alg.finalize();
    TS_ASSERT( ! status.isFailure() );
    TS_ASSERT( alg.isFinalized() );
	}

	void testIsInitialized()
  {
	  Mantid::Algorithm alg("Hello","1.1");
	  TS_ASSERT( ! alg.isInitialized() );
  }

	void testIsExecuted()
	{
	  Mantid::Algorithm alg("Hello","1.1");
	  TS_ASSERT( ! alg.isExecuted() );
	}

	void testIsFinalized()
	{
	  Mantid::Algorithm alg("Hello","1.1");
	  TS_ASSERT( ! alg.isFinalized() );	    
	}

//	void testCreateSubAlgorithm()
//	{
//	  // Method not implemented yet in Algorithm.cpp (need algorithm factory)
//	}

	void testSubAlgorithm()
	{
    Mantid::Algorithm alg("Hello","1.1");
    std::vector<Mantid::Algorithm*>* testPointer = alg.subAlgorithms();
    // Check that the newly created vector is empty
    TS_ASSERT( testPointer->empty() );
	}
	
};

#endif /*ALGORITHMTEST_H_*/
