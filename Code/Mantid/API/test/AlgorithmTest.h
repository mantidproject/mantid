#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"

using namespace Mantid::Kernel; 
using namespace Mantid::API;

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() : Algorithm() {}
  virtual ~ToyAlgorithm() {}
  void init()
  { declareProperty("prop1","value");
    declareProperty("prop2",1);   
  }
  void exec() {}
  void final() {}
  
  bool existsProperty( const std::string &name ) const
  {
    return PropertyManager::existsProperty(name);
  }
  const std::vector< Property* >& getProperties() const
  {
    return PropertyManager::getProperties();
  }
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

  void testIsChild()
  {
    TS_ASSERT_EQUALS(false, alg.isChild());
    alg.setChild(true);
    TS_ASSERT_EQUALS(true, alg.isChild());
    alg.setChild(false);
    TS_ASSERT_EQUALS(false, alg.isChild());
  }

  void testInitialize()
  {
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT( alg.isInitialized() );
  }

  void testExecute()
  {
    ToyAlgorithm myAlg;
    TS_ASSERT_THROWS(myAlg.execute(),std::runtime_error);
    TS_ASSERT( ! myAlg.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( myAlg.initialize());
    TS_ASSERT_THROWS_NOTHING(myAlg.execute() );
    TS_ASSERT( myAlg.isExecuted() );
  }

  void testFinalize()
  {
    ToyAlgorithm myAlg;
    TS_ASSERT_THROWS(myAlg.finalize(),std::runtime_error);;
    // Need to initialize otherwise the finalize method immediately returns
    myAlg.initialize();
    TS_ASSERT_THROWS_NOTHING(myAlg.finalize());
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
	
  void testSetProperty()
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
    TS_ASSERT_THROWS(alg.getPropertyValue("ghjkgh"), Exception::NotFoundError )    
  }
  
  void testGetProperties()
  {
    std::vector<Property*> vec = alg.getProperties();
    TS_ASSERT( ! vec.empty() )
    TS_ASSERT( vec.size() == 2 )
    TS_ASSERT( ! vec[0]->name().compare("prop1") )
  }    
private:
  ToyAlgorithm alg;	
};

 

#endif /*ALGORITHMTEST_H_*/
