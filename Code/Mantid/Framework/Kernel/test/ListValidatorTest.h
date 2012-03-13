#ifndef LISTVALIDATORTEST_H_
#define LISTVALIDATORTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;

class ListValidatorTest : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    StringListValidator v;
    TS_ASSERT( v.allowedValues().empty() )
  }

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    vec.push_back("three");
    StringListValidator v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testIsValid()
  {
    StringListValidator v;
    TS_ASSERT_EQUALS( v.isValid(""), "Select a value" )

    TS_ASSERT_EQUALS( v.isValid("b"),
      "The value \"b\" is not in the list of allowed values")

    v.addAllowedValue("a");
    TS_ASSERT_EQUALS( v.isValid(""),
      "Select a value" )
    TS_ASSERT_EQUALS( v.isValid("a"), "" )
    TS_ASSERT_EQUALS( v.isValid("b"),
      "The value \"b\" is not in the list of allowed values")
    TS_ASSERT_EQUALS( v.isValid("A"),
      "The value \"A\" is not in the list of allowed values")
  }

  void testAllowedValues()
  {
    StringListValidator v;
    v.addAllowedValue("one");
    v.addAllowedValue("two");
    std::set<std::string> s;
    TS_ASSERT_THROWS_NOTHING( s = v.allowedValues() )
    TS_ASSERT_EQUALS( s.size(), 2 )
    TS_ASSERT( s.count("one") )
    TS_ASSERT( s.count("two") )
    TS_ASSERT( ! s.count("three") )
  }

  void testAddAllowedValue()
  {
    StringListValidator v;
    TS_ASSERT( v.allowedValues().empty() )
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue("x") )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid("x"), "" )
    // Try adding again just to make sure the instruction is ignored, but doesn't throw
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue("x") )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid("x"), "" )
  }

  void testClone()
  {
    IValidator_sptr v(new StringListValidator);
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv );
    TS_ASSERT( boost::dynamic_pointer_cast<StringListValidator>(vv) );
  }
  
};

#endif /*LISTVALIDATORTEST_H_*/
