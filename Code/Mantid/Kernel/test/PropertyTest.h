#ifndef PROPERTYTEST_H_
#define PROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Property.h"

using namespace Mantid::Kernel;

// Helper class
class PropertyHelper : public Property
{
public:
  PropertyHelper() : Property( "Test", typeid( int ) ) {}
  Property* clone() { return new PropertyHelper(*this); }
  std::string value() const { return "Nothing"; }
  std::string setValue( const std::string& value ) { return ""; }
  bool isDefault() const { return true; }
  std::string getDefault() const { return "Is not implemented in this class, should be overriden"; }
};

class PropertyTest : public CxxTest::TestSuite
{
public:
  PropertyTest()
  {
    p = new PropertyHelper;
  }

  ~PropertyTest()
  {
    delete p;
  }
  
  void testName()
  {
    TS_ASSERT( ! p->name().compare("Test") );
  }

  void testDocumentation()
  {
    Property *pp = new PropertyHelper;
    TS_ASSERT( ! pp->documentation().compare("") );
    delete pp;
  }

  void testType_info()
  {
    TS_ASSERT( typeid( int ) == *p->type_info()  );
  }

  void testType()
  {
//#ifdef __GNUC__
//    TS_ASSERT( ! p->type().compare("i") )
//#else
//    TS_ASSERT( ! p->type().compare("int") )
//#endif
// MG 16/07/2009: Type now returns something standardized across platforms
// so that it can be shown in the interfaces
  TS_ASSERT( ! p->type().compare("number") );
  
  }

  void testisValid()
  {
    TS_ASSERT_EQUALS( p->isValid(), "" );
  }

  void testIsDefault()
  {
    TS_ASSERT( p->isDefault() );
  }

  void testSetDocumentation()
  {
    const std::string str("Documentation comment");
    p->setDocumentation(str);
    TS_ASSERT( ! p->documentation().compare(str) );
  }

  void testAllowedValues()
  {
    TS_ASSERT( p->allowedValues().empty() );
  }

  void testCreateHistory()
  {
    PropertyHistory history = p->createHistory();
    TS_ASSERT_EQUALS( history.name(), "Test" );
    TS_ASSERT_EQUALS( history.value(), "Nothing" );
    TS_ASSERT( history.isDefault() );
    TS_ASSERT_EQUALS( history.type(), p->type() );
    TS_ASSERT_EQUALS( history.direction(), 0 );
  }

  void testUnits()
  {
    Property * p2;
    p2 = new PropertyHelper;
    //No unit at first
    TS_ASSERT_EQUALS(p2->units(), "");
    p2->setUnits("furlongs/fortnight");
    TS_ASSERT_EQUALS(p2->units(), "furlongs/fortnight");
  }

private:
  Property *p;

};

#endif /*PROPERTYTEST_H_*/
