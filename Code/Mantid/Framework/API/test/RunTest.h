#ifndef RUNTEST_H_
#define RUNTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/Run.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Property.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Helper class
namespace
{
  class ConcreteProperty : public Property
  {
  public:
    ConcreteProperty() : Property( "Test", typeid( int ) ) {}
    Property* clone() { return new ConcreteProperty(*this); }
    bool isDefault() const { return true; }
    std::string getDefault() const { return "getDefault() is not implemented in this class"; }
    std::string value() const { return "Nothing"; }
    std::string setValue( const std::string& ) { return ""; }
    Property& operator+=( Property const * ) { return *this; }
  };
}


class RunTest : public CxxTest::TestSuite
{
 
public:
  RunTest()
  {
  }

  void testAddGetData()
  {
    Run runInfo;

    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );

    Property *pp = NULL;
    TS_ASSERT_THROWS_NOTHING( pp = runInfo.getProperty("Test") );
    TS_ASSERT_EQUALS( p, pp );
    TS_ASSERT( ! pp->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(pp) );
    TS_ASSERT_THROWS( pp = runInfo.getProperty("NotThere"), Exception::NotFoundError );

    std::vector< Property* > props = runInfo.getProperties();
    TS_ASSERT( ! props.empty() );
    TS_ASSERT_EQUALS( props.size(), 1 );
    TS_ASSERT( ! props[0]->name().compare("Test") );
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(props[0]) );
  }

  void testRemoveLogData()
  {
    Run runInfo;
    
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_THROWS_NOTHING( runInfo.removeProperty("Test") );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 0 );
  }

  void testGetSetProtonCharge()
  {
    Run runInfo;
    TS_ASSERT_THROWS(runInfo.getProtonCharge(), Exception::NotFoundError);
    TS_ASSERT_THROWS_NOTHING( runInfo.setProtonCharge(10.0) );
    TS_ASSERT_EQUALS( runInfo.getProtonCharge(), 10.0 );
  }

  void testCopyAndAssignment()
  {
    Run runInfo;
    runInfo.setProtonCharge(10.0);
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) );
    TS_ASSERT_EQUALS( runInfo.getProperties().size(), 2);
    
    //Copy constructor
    Run runInfo_2(runInfo);
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 2);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 10.0, 1e-8);
    TS_ASSERT_EQUALS( runInfo_2.getLogData("Test")->value(), "Nothing" );    


    // Now assignment
    runInfo.setProtonCharge(15.0);
    runInfo.removeProperty("Test");
    runInfo_2 = runInfo;
    TS_ASSERT_EQUALS( runInfo_2.getProperties().size(), 1);
    TS_ASSERT_DELTA( runInfo_2.getProtonCharge(), 15.0, 1e-8);
  }

  void testMemory()
  {
    Run runInfo;
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), 0);
    
    Property *p = new ConcreteProperty();
    runInfo.addProperty(p);
    TS_ASSERT_EQUALS( runInfo.getMemorySize(), sizeof(ConcreteProperty) + sizeof( void *));
  }

  void test_getGoniometer()
  {
    Run runInfo;
    TS_ASSERT_THROWS_NOTHING( runInfo.getGoniometer() );
    // No axes by default
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 0 );
    // Now does copy work?
    runInfo.getGoniometer().makeUniversalGoniometer();
    TS_ASSERT_EQUALS( runInfo.getGoniometer().getNumberAxes(), 3 );
    Run runCopy(runInfo);
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );
    runCopy = runInfo;
    TS_ASSERT_EQUALS( runCopy.getGoniometer().getNumberAxes(), 3 );

  }

};



#endif
