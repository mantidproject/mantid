#ifndef LIST_ANY_VALIDATOR_TEST_H_
#define LIST_ANY_VALIDATOR_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ListAnyValidator.h"

using namespace Mantid::Kernel;

class ListAnyValidatorTest : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    ListAnyValidator<int> v;
    TS_ASSERT( v.allowedValues().empty() )
  }

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    vec.push_back("three");
    ListAnyValidator<std::string> v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testVectorConstructor2()
  {
    std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    ListAnyValidator<int> v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testIsValid()
  {
    ListAnyValidator<int> v;

    TS_ASSERT_EQUALS( v.isValid(1),
      "The value \"1\" is not in the list of allowed values")

    v.addAllowedValue(1);
    TS_ASSERT_EQUALS( v.isValid(1), "" )
    TS_ASSERT_EQUALS( v.isValid(2),
      "The value \"2\" is not in the list of allowed values")
  }

  void testAllowedValues()
  {
    ListAnyValidator<int> v;
    v.addAllowedValue(1);
    v.addAllowedValue(2);
    std::set<std::string> s;
    TS_ASSERT_THROWS_NOTHING( s = v.allowedValues() )
    TS_ASSERT_EQUALS( s.size(), 2 )
    TS_ASSERT( s.count("1") )
    TS_ASSERT( s.count("2") )
    TS_ASSERT( ! s.count("3") )
  }

  void testAddAllowedValue()
  {
    ListAnyValidator<int> v;
    TS_ASSERT( v.allowedValues().empty() )
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue(10) )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid(10), "" )
    // Try adding again just to make sure the instruction is ignored, but doesn't throw
    TS_ASSERT_THROWS_NOTHING( v.addAllowedValue(10) )
    TS_ASSERT_EQUALS( v.allowedValues().size(), 1 )
    TS_ASSERT_EQUALS( v.isValid(10), "" )
  }

  void testClone()
  {
    IValidator_sptr v = new ListAnyValidator<int>();
    IValidator_sptr vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    TS_ASSERT( dynamic_cast<ListAnyValidator<int> *>(vv) )
    delete v;
    delete vv;
  }

  void testCast()
  {
    IValidator_sptr v = new ListAnyValidator<int>();
    TS_ASSERT( dynamic_cast<IValidator_sptr >(v) )
    TS_ASSERT( ! dynamic_cast<IValidator_sptr >(v) )
    delete v;
  }

  void testAddStringVSIntValue()
  {
    ListAnyValidator<int> v1;
    ListAnyValidator<int> v2;
    TS_ASSERT_THROWS_NOTHING(v1.addAllowedValue("1"));
    TS_ASSERT_THROWS_NOTHING(v1.addAllowedValue(1));
    TS_ASSERT_THROWS_NOTHING(v2.addAllowedValue("1"));
    std::set<std::string> val1=v1.allowedValues();
    std::set<std::string> val2=v2.allowedValues();
    TS_ASSERT_EQUALS(1,val1.size());
    TS_ASSERT_EQUALS(*val1.begin(),*val2.begin());
  }
  
};

#endif /*LISTVALIDATORTEST_H_*/
