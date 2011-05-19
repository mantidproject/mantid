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
  const std::string alias() const { return "Dog";}

  void init()
  { 
    declareProperty("prop1","value");
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

class ToyAlgorithmTwo : public Algorithm
{
public:
  ToyAlgorithmTwo() : Algorithm() {}
  virtual ~ToyAlgorithmTwo() {}

  const std::string name() const { return "ToyAlgorithm";} ///< Algorithm's name for identification
  int version() const  { return 2;} ///< Algorithm's version for identification
  const std::string category() const { return "Cat";} 
  const std::string alias() const { return "Dog";}
  void init()
  { 
    declareProperty("prop1","value");
    declareProperty("prop2",1);   
    declareProperty("prop3",10.5);   
  }
  void exec() {}
};

class AlgorithmTest : public CxxTest::TestSuite
{
public:

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AlgorithmTest *createSuite() { return new AlgorithmTest(); }
  static void destroySuite( AlgorithmTest *suite ) { delete suite; }

  AlgorithmTest()
  {
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithm>();
    Mantid::API::AlgorithmFactory::Instance().subscribe<ToyAlgorithmTwo>();
  }

  ~AlgorithmTest()
  {
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm|1");
    Mantid::API::AlgorithmFactory::Instance().unsubscribe("ToyAlgorithm|2");
  }
  
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

  void testAlias()
  {
    TS_ASSERT_EQUALS( alg.alias(), "Dog");
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

  void testStringization()
  {
    //Set the properties so that we know what they are
    alg.setPropertyValue("prop1", "value1");
    alg.setProperty("prop2", 5);
    std::string expected = "ToyAlgorithm.1(prop1=value1,prop2=5)";
    TS_ASSERT_EQUALS(alg.toString(false), expected);

    // Rest the first property back to default and then include it in the toString list
    alg.setPropertyValue("prop1", "value");
    expected = "ToyAlgorithm.1(prop1=value,prop2=5)";
    TS_ASSERT_EQUALS(alg.toString(true), expected);
  }

  void test_From_String_With_Invalid_Input_Throws()
  {
    const std::string input = "()";
    TS_ASSERT_THROWS(Algorithm::fromString(input), std::runtime_error );
  }

  void test_Construction_Via_Valid_String_With_No_Properties()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
  }

  void test_Construction_Via_Valid_String_With_Version()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
    
    // No brackets
    testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }

  void test_Construction_Via_Valid_String_With_Version_And_Empty_Props()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.1()");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
    
    // No brackets
    testAlg = runFromString("ToyAlgorithm.1");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 1);
  }


  void test_Construction_Via_Valid_String_With_Set_Properties_And_Version()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm.2(prop1=val1,prop2=8,prop3=10.0)");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
    
    // On gcc we get ambiguous function calls doing
    // std::string s;
    // s = getProperty(...);
    // so we have to do this
    try
    {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1,"val1");
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try
    {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 8);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'prop2'");
    }
    try
    {
      double prop3 = testAlg->getProperty("prop3");
      TS_ASSERT_EQUALS(prop3, 10.0);
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop3'");
    }
  }

  void test_Construction_Via_Valid_String_With_Empty_Properties()
  {
    IAlgorithm_sptr testAlg = runFromString("ToyAlgorithm()");
    TS_ASSERT_EQUALS(testAlg->name(), "ToyAlgorithm");
    TS_ASSERT_EQUALS(testAlg->version(), 2);
    
     try
    {
      std::string prop1 = testAlg->getProperty("prop1");
      TS_ASSERT_EQUALS(prop1,"value");
    }
    catch(...)
    {
      TS_FAIL("Cannot retrieve property 'prop1'");
    }
    try
    {
      int prop2 = testAlg->getProperty("prop2");
      TS_ASSERT_EQUALS(prop2, 1);
    }
    catch(...)
    {
       TS_FAIL("Cannot retrieve property 'prop2'");
    }

  }


private:
  IAlgorithm_sptr runFromString(const std::string & input)
  {
    IAlgorithm_sptr testAlg;
    TS_ASSERT_THROWS_NOTHING(testAlg = Algorithm::fromString(input) );
    TS_ASSERT(testAlg);
    if(!testAlg) TS_FAIL("Failed to create algorithm, cannot continue test.");
    return testAlg;
  }

  ToyAlgorithm alg;
  ToyAlgorithmTwo algv2;
};

 

#endif /*ALGORITHMTEST_H_*/
