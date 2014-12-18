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
    std::vector<std::string> s;
    TS_ASSERT_THROWS_NOTHING( s = v.allowedValues() )
    TS_ASSERT_EQUALS( s.size(), 2 )
    TS_ASSERT( std::find( s.begin(), s.end(), "one")  != s.end() )
    TS_ASSERT( std::find( s.begin(), s.end(), "two")  != s.end() )
    TS_ASSERT( std::find( s.begin(), s.end(), "three")  == s.end() )
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

  void testAliasString()
  {
    std::vector<std::string> values;
    values.push_back("one");
    values.push_back("three");
    values.push_back("two");
    std::map<std::string,std::string> aliases;
    aliases["1"] = "one";
    aliases["2"] = "two";
    aliases["3"] = "three";
    StringListValidator validator(values, aliases);
    TS_ASSERT_EQUALS( validator.isValid("one"), "" )
    TS_ASSERT_EQUALS( validator.isValid("two"), "" )
    TS_ASSERT_EQUALS( validator.isValid("three"), "" )

    TS_ASSERT_EQUALS( validator.isValid("1"), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid("2"), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid("3"), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid("4"), "The value \"4\" is not in the list of allowed values" )

    TS_ASSERT_EQUALS( validator.getValueForAlias("1"), "one" )
    TS_ASSERT_EQUALS( validator.getValueForAlias("2"), "two" )
    TS_ASSERT_EQUALS( validator.getValueForAlias("3"), "three" )

    TS_ASSERT_THROWS( validator.getValueForAlias("4"), std::invalid_argument )
  }
  
  void testAliasInt()
  {
    std::vector<int> values;
    values.push_back(1);
    values.push_back(5);
    values.push_back(3);
    std::map<std::string,std::string> aliases;
    aliases["11"] = "1";
    aliases["33"] = "3";
    aliases["55"] = "5";
    ListValidator<int> validator(values, aliases);
    TS_ASSERT_EQUALS( validator.isValid(1), "" )
    TS_ASSERT_EQUALS( validator.isValid(3), "" )
    TS_ASSERT_EQUALS( validator.isValid(5), "" )

    TS_ASSERT_EQUALS( validator.isValid(11), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid(33), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid(55), "_alias" )
    TS_ASSERT_EQUALS( validator.isValid(4), "The value \"4\" is not in the list of allowed values" )

    TS_ASSERT_EQUALS( validator.getValueForAlias("11"), "1" )
    TS_ASSERT_EQUALS( validator.getValueForAlias("33"), "3" )
    TS_ASSERT_EQUALS( validator.getValueForAlias("55"), "5" )

    TS_ASSERT_THROWS( validator.getValueForAlias("13"), std::invalid_argument )
  }
  
  void test_wrong_alias()
  {
    std::vector<std::string> values;
    values.push_back("one");
    values.push_back("three");
    std::map<std::string,std::string> aliases;
    aliases["1"] = "one";
    aliases["2"] = "two";
    TS_ASSERT_THROWS( StringListValidator validator(values, aliases), std::invalid_argument );
  }
  
  void test_self_alias()
  {
    std::vector<std::string> values;
    values.push_back("one");
    values.push_back("three");
    std::map<std::string,std::string> aliases;
    aliases["1"] = "one";
    aliases["three"] = "three";
    aliases["one"] = "three";
    StringListValidator validator(values, aliases);

    TS_ASSERT_EQUALS( validator.isValid("one"), "" )
    TS_ASSERT_EQUALS( validator.isValid("three"), "" )
    TS_ASSERT_EQUALS( validator.isValid("1"), "_alias" )
  }
  
};

#endif /*LISTVALIDATORTEST_H_*/
