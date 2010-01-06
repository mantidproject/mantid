#ifndef TESTSAMPLE_H_
#define TESTSAMPLE_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Exception.h"

using namespace Mantid::Kernel;
using Mantid::API::Sample;

// Helper class
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

class SampleTest : public CxxTest::TestSuite
{
public:
  void testSetGetName()
  {
    TS_ASSERT( ! sample.getName().compare("") )
    sample.setName("test");
    TS_ASSERT( ! sample.getName().compare("test") )
  }

  void testAddGetLogData()
  {
    Property *p = new ConcreteProperty();
    TS_ASSERT_THROWS_NOTHING( sample.addLogData(p) )

    Property *pp;
    TS_ASSERT_THROWS_NOTHING( pp = sample.getLogData("Test") )
    TS_ASSERT_EQUALS( p, pp )
    TS_ASSERT( ! pp->name().compare("Test") )
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(pp) )
    TS_ASSERT_THROWS( sample.getLogData("NotThere"), Exception::NotFoundError )

    std::vector< Property* > props = sample.getLogData();
    TS_ASSERT( ! props.empty() )
    TS_ASSERT_EQUALS( props.size(), 1 )
    TS_ASSERT( ! props[0]->name().compare("Test") )
    TS_ASSERT( dynamic_cast<ConcreteProperty*>(props[0]) )
  }

  void testGetSetCharge()
  {
    TS_ASSERT_EQUALS( sample.getProtonCharge(), 0.0 )
    TS_ASSERT_THROWS_NOTHING( sample.setProtonCharge(10.0) )
    TS_ASSERT_EQUALS( sample.getProtonCharge(), 10.0 )
  }

private:
  Sample sample;
};

#endif /*TESTSAMPLE_H_*/
