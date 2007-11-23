#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"

using namespace Mantid::Kernel; 

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() : Algorithm() {}
  virtual ~ToyAlgorithm() {}
  StatusCode init()
  { declareProperty("prop1","value");
    declareProperty("prop2",1);
    return StatusCode::SUCCESS; 
  }
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
    TS_ASSERT( ! status.isFailure() );
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
    std::vector<Algorithm*>& testVec = alg.subAlgorithms();
    // Check that the newly created vector is empty
    TS_ASSERT( testVec.empty() );
  }
	
  void testSetProprerty()
  {
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("prop1","val") )
    TS_ASSERT_THROWS( alg.setProperty("prop3","1"), Exception::NotFoundError )
  }

  void testExistsProperty()
  {
    TS_ASSERT( alg.existsProperty("prop1") )
    TS_ASSERT( ! alg.existsProperty("notThere") )
  }
  
  void testGetPropertyValue()
  {
    std::string value;
    TS_ASSERT_THROWS_NOTHING( value = alg.getPropertyValue("prop2") )
    TS_ASSERT( ! value.compare("1") )
    TS_ASSERT_THROWS(alg.getProperty("ghjkgh"), Exception::NotFoundError )    
  }
  
  void testGetProperty()
  {
    Property *p;
    TS_ASSERT_THROWS_NOTHING( p = alg.getProperty("prop2") )
    TS_ASSERT( ! p->name().compare("prop2") )
    TS_ASSERT( ! p->value().compare("1") )
    TS_ASSERT( typeid( int) == *p->type_info() )
    
    TS_ASSERT_THROWS( alg.getProperty("wrong"), Exception::NotFoundError )
	}

  void testGetProperties()
  {
    std::vector<Property*> vec = alg.getProperties();
    TS_ASSERT( ! vec.empty() )
    TS_ASSERT( vec.size() == 4 )
    TS_ASSERT( ! vec[0]->name().compare("InputWorkspace") )
  }
    
private:
  ToyAlgorithm alg;
	
};

 

#endif /*ALGORITHMTEST_H_*/
