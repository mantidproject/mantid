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
    std::string setValue( const std::string& value ) { return ""; }
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
    TS_ASSERT_THROWS_NOTHING( runInfo.addProperty(p) )

    Property *pp;
    TS_ASSERT_THROWS_NOTHING( pp = runInfo.getProperty("Test") )
    TS_ASSERT_EQUALS( p, pp )
    TS_ASSERT( ! pp->name().compare("Test") )
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(pp) )
    TS_ASSERT_THROWS( pp = runInfo.getProperty("NotThere"), Exception::NotFoundError )

    std::vector< Property* > props = runInfo.getProperties();
    TS_ASSERT( ! props.empty() )
    TS_ASSERT_EQUALS( props.size(), 1 )
    TS_ASSERT( ! props[0]->name().compare("Test") )
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(props[0]) )
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
    TS_ASSERT_THROWS(runInfo.getProtonCharge(), Exception::NotFoundError)
    TS_ASSERT_THROWS_NOTHING( runInfo.setProtonCharge(10.0) )
    TS_ASSERT_EQUALS( runInfo.getProtonCharge(), 10.0 )
  }
};



#endif
