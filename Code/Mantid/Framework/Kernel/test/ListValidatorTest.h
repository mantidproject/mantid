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
    ListValidator v;
    TS_ASSERT( v.allowedValues().empty() )
  }

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    vec.push_back("three");
    ListValidator v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testIsValid()
  {
    ListValidator v;
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
    ListValidator v;
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
    ListValidator v;
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
    IValidator<std::string> *v = new ListValidator;
    IValidator<std::string> *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    TS_ASSERT( dynamic_cast<ListValidator*>(vv) )
    delete v;
    delete vv;
  }

  void testCast()
  {
    ListValidator *v = new ListValidator;
    TS_ASSERT( dynamic_cast<IValidator<std::string>*>(v) )
    TS_ASSERT( ! dynamic_cast<IValidator<int>*>(v) )
    delete v;
  }
  
};

#endif /*LISTVALIDATORTEST_H_*/
