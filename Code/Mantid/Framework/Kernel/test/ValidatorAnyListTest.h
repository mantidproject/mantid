#ifndef VALIDATOR_ANYLIST_TEST_H_
#define  VALIDATOR_ANYLIST_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ValidatorAnyList.h"

using namespace Mantid::Kernel;

class ValidatorAnyListTest : public CxxTest::TestSuite
{
public:
  void testEmptyConstructor()
  {
    ValidatorAnyList<int> v;
    TS_ASSERT( v.allowedValues().empty() )
  }

  void testVectorConstructor()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    vec.push_back("three");
    ValidatorAnyList<std::string> v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }
  void testVectorConstructor2()
  {
    std::vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    ValidatorAnyList<int> v(vec);
    TS_ASSERT_EQUALS( v.allowedValues().size(), 3 )
  }

  void testIsValid()
  {
    ValidatorAnyList<int> v;
    //TS_ASSERT_EQUALS( v.isValid(""), "Select a value" )

    TS_ASSERT_EQUALS( v.isValid(1),
      "The value \"1\" is not in the list of allowed values")

    v.addAllowedValue(1);
   // TS_ASSERT_EQUALS( v.isValid(""),
   //   "Select a value" )
    TS_ASSERT_EQUALS( v.isValid(1), "" )
    TS_ASSERT_EQUALS( v.isValid(2),
      "The value \"2\" is not in the list of allowed values")
  }

  void testAllowedValues()
  {
    ValidatorAnyList<int> v;
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
    ValidatorAnyList<int> v;
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
    IValidator<int> *v = new ValidatorAnyList<int>();
    IValidator<int> *vv = v->clone();
    TS_ASSERT_DIFFERS( v, vv )
    TS_ASSERT( dynamic_cast<ValidatorAnyList<int> *>(vv) )
    delete v;
    delete vv;
  }

  void testCast()
  {
    IValidator<int> *v = new ValidatorAnyList<int>();
    TS_ASSERT( dynamic_cast<IValidator<int>*>(v) )
    TS_ASSERT( ! dynamic_cast<IValidator<double>*>(v) )
    delete v;
  }
  
};

#endif /*LISTVALIDATORTEST_H_*/
