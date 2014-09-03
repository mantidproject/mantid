#ifndef STARTSWITHVALIDATORTEST_H_
#define STARTSWITHVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/StartsWithValidator.h"

using namespace Mantid::Kernel;

class StartsWithValidatorTest : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    StartsWithValidator v;
    TS_ASSERT( v.allowedValues().empty() )
  }

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    vec.push_back("three");
    StartsWithValidator v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testIsValid()
  {
    StartsWithValidator v;
    TS_ASSERT_EQUALS( v.isValid(""), "Select a value" )

    TS_ASSERT_EQUALS( v.isValid("b"),
      "The value \"b\" does not start with any of the allowed values")

    v.addAllowedValue("a");
    TS_ASSERT_EQUALS( v.isValid(""),
      "Select a value" )
    TS_ASSERT_EQUALS( v.isValid("alpha"), "" )
    TS_ASSERT_EQUALS( v.isValid("beta"),
      "The value \"beta\" does not start with any of the allowed values")
    TS_ASSERT_EQUALS( v.isValid("ALPHA"),
      "The value \"ALPHA\" does not start with any of the allowed values")
  }

  void testAllowedValues()
  {
    StartsWithValidator v;
    v.addAllowedValue("one");
    v.addAllowedValue("two");
    std::vector<std::string> s;
    TS_ASSERT_THROWS_NOTHING( s = v.allowedValues() )
    TS_ASSERT_EQUALS( s.size(), 2 )
    TS_ASSERT( std::find( s.begin(), s.end(), "one")  != s.end() )
    TS_ASSERT( std::find( s.begin(), s.end(), "two")  != s.end() )
    TS_ASSERT( std::find( s.begin(), s.end(), "three")  == s.end() )
  }

  void testLongValues()
  {
    StartsWithValidator v;
    v.addAllowedValue("one");
    v.addAllowedValue("two");
    TS_ASSERT_EQUALS( v.isValid("one"), "" )
    TS_ASSERT_EQUALS( v.isValid("two"), "" )
    TS_ASSERT_EQUALS( v.isValid("two and a half"), "" )
    TS_ASSERT_EQUALS( v.isValid("on"), "The value \"on\" does not start with any of the allowed values" )
    TS_ASSERT_EQUALS( v.isValid(" one"), "The value \" one\" does not start with any of the allowed values" )
    TS_ASSERT_EQUALS( v.isValid("twenty-one"), "The value \"twenty-one\" does not start with any of the allowed values" )
  }

  void testAddAllowedValue()
  {
    StartsWithValidator v;
    TS_ASSERT( v.allowedValues().empty() )
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue("x") )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid("x1"), "" )
    // Try adding again just to make sure the instruction is ignored, but doesn't throw
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue("x") )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid("x2"), "" )
  }

  void testClone()
  {
    IValidator_sptr v(new StartsWithValidator);
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv );
    TS_ASSERT( boost::dynamic_pointer_cast<StartsWithValidator>(vv) );
  }
  
};

#endif /*STARTSWITHVALIDATORTEST_H_*/
