#ifndef PROPERTYTEST_H_
#define PROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Property.h"
#include "MantidKernel/PropertyHistory.h"

using namespace Mantid::Kernel;

// Helper class
class PropertyHelper : public Property
{
public:
  PropertyHelper() : Property( "Test", typeid( int ) ) {}
  PropertyHelper* clone() const { return new PropertyHelper(*this); }
  std::string value() const { return "Nothing"; }
  std::string setValue( const std::string&) { return ""; }
  std::string setValueFromProperty( const Property& ) { return ""; }
  std::string setDataItem( const boost::shared_ptr<DataItem> ) { return ""; }
  bool isDefault() const { return true; }
  std::string getDefault() const { return "Is not implemented in this class, should be overriden"; }
  Property& operator+=( Property const * ) { return *this; }
};

class PropertyTest : public CxxTest::TestSuite
{
public:
  static PropertyTest *createSuite() { return new PropertyTest(); }
  static void destroySuite(PropertyTest *suite) { delete suite; }

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
    TS_ASSERT( pp->documentation().empty() );
    TS_ASSERT( pp->briefDocumentation().empty() );
    delete pp;
  }

  void testType_info()
  {
    TS_ASSERT( typeid( int ) == *p->type_info()  );
  }

  void testType()
  {
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
    const std::string str("Doc comment. This property does something.");
    p->setDocumentation(str);
    TS_ASSERT_EQUALS( p->documentation(), str );
    TS_ASSERT_EQUALS( p->briefDocumentation(), "Doc comment");

    const std::string str2("A string with no period to be seen");
    // Brief documentation not changed if it's not empty
    p->setDocumentation(str2);
    TS_ASSERT_EQUALS( p->documentation(), str2 );
    TS_ASSERT_EQUALS( p->briefDocumentation(), "Doc comment");

    // Make it empty and see that it will now be changed via setDocumentation()
    p->setBriefDocumentation("");
    TS_ASSERT( p->briefDocumentation().empty() );
    p->setDocumentation(str2);
    TS_ASSERT_EQUALS( p->documentation(), str2 );
    TS_ASSERT_EQUALS( p->briefDocumentation(), str2 );

    // Set just the brief documentation
    p->setBriefDocumentation("Brief");
    TS_ASSERT_EQUALS( p->documentation(), str2 );
    TS_ASSERT_EQUALS( p->briefDocumentation(), "Brief" );
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
    Property * p2 = new PropertyHelper;
    //No unit at first
    TS_ASSERT_EQUALS(p2->units(), "");
    p2->setUnits("furlongs/fortnight");
    TS_ASSERT_EQUALS(p2->units(), "furlongs/fortnight");
    delete p2;
  }

  void testRemember()
  {
    Property * p3 = new PropertyHelper;
    TS_ASSERT(p3->remember());
    p3->setRemember(false);
    TS_ASSERT(!p3->remember());
    p3->setRemember(true);
    TS_ASSERT(p3->remember());
    delete p3;
  }

private:
  Property *p;

};

#endif /*PROPERTYTEST_H_*/
