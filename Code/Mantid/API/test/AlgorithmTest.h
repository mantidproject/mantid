#ifndef ALGORITHMTEST_H_
#define ALGORITHMTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/Property.h"
#include "MantidAPI/AlgorithmFactory.h"

using namespace Mantid::Kernel; 
using namespace Mantid::API;

class ToyAlgorithm : public Algorithm
{
public:
  ToyAlgorithm() : Algorithm() {}
  virtual ~ToyAlgorithm() {}
  const std::string name() const { return "ToyAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 1;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat";} ///< Algorithm's category for identification

  void init()
  { declareProperty("prop1","value");
    declareProperty("prop2",1);   
  }
  void exec() {}
  
  bool existsProperty( const std::string &name ) const
  {
    return PropertyManagerOwner::existsProperty(name);
  }
  const std::vector< Property* >& getProperties() const
  {
    return PropertyManagerOwner::getProperties();
  }
};

DECLARE_ALGORITHM(ToyAlgorithm)

class AlgorithmTest : public CxxTest::TestSuite
{
public: 
  
  void testAlgorithm()
  {
    std::string theName = alg.name();
    TS_ASSERT( ! theName.compare("ToyAlgorithm") );
    int theVersion = alg.version();
    TS_ASSERT_EQUALS( theVersion,1 );
    TS_ASSERT( ! alg.isInitialized() );
    TS_ASSERT( ! alg.isExecuted() );
  }

  void testName()
  {
    std::string theName = alg.name();
    TS_ASSERT( ! theName.compare("ToyAlgorithm") );
  }

  void testVersion()
  {
    int theVersion = alg.version();
    TS_ASSERT_EQUALS( theVersion,1 );
  }

  void testCategory()
  {
    TS_ASSERT_EQUALS( alg.category(),"Cat" );
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

	
  void testSetPropertyValue()
  {
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("prop1","val") )
    TS_ASSERT_THROWS( alg.setPropertyValue("prop3","1"), Exception::NotFoundError )
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
