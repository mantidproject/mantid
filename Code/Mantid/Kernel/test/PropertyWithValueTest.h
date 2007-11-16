#ifndef PROPERTYWITHVALUETEST_H_
#define PROPERTYWITHVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/PropertyWithValue.h"

using namespace Mantid::Kernel;

class PropertyWithValueTest : public CxxTest::TestSuite
{
public:
  PropertyWithValueTest()
  {
    iProp = new PropertyWithValue<int>("intProp", 1);
    dProp = new PropertyWithValue<double>("doubleProp", 9.99);
    sProp = new PropertyWithValue<std::string>("stringProp", "theValue");
  }
  
  void testConstructor()
  {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT( ! iProp->name().compare("intProp") )
    TS_ASSERT( ! iProp->documentation().compare("") )
    TS_ASSERT( typeid( int ) == *iProp->type_info()  )
    TS_ASSERT( iProp->isDefault() )
    
    TS_ASSERT( ! dProp->name().compare("doubleProp") )
    TS_ASSERT( ! dProp->documentation().compare("") )
    TS_ASSERT( typeid( double ) == *dProp->type_info()  )
    TS_ASSERT( dProp->isDefault() )
    
    TS_ASSERT( ! sProp->name().compare("stringProp") )
    TS_ASSERT( ! sProp->documentation().compare("") )
    TS_ASSERT( typeid( std::string ) == *sProp->type_info()  )
    TS_ASSERT( sProp->isDefault() )
  }
  
	void testValue()
	{
		TS_ASSERT( ! iProp->value().compare("1") )
    TS_ASSERT( ! dProp->value().compare("9.99") )
    TS_ASSERT( ! sProp->value().compare("theValue") )
	}

	void testSetValue()
	{
		PropertyWithValue<int> i("test", 1);
		TS_ASSERT( i.setValue("10") )
		TS_ASSERT_EQUALS( i, 10 )
		TS_ASSERT( ! i.setValue("9.99") )
		TS_ASSERT( ! i.setValue("garbage") )
		
		PropertyWithValue<double> d("test", 5.55);
		TS_ASSERT( d.setValue("-9.99") )
		TS_ASSERT_EQUALS( d, -9.99 )
		TS_ASSERT( d.setValue("0") )
	  TS_ASSERT_EQUALS( d, 0 )
    TS_ASSERT( ! d.setValue("garbage") )
    
    PropertyWithValue<std::string> s("test", "test");
    TS_ASSERT( s.setValue("-9.99") )
    TS_ASSERT_EQUALS( s.operator()(), "-9.99" )
    TS_ASSERT( s.setValue("0") )
    TS_ASSERT_EQUALS( s.operator()(), "0" )
    TS_ASSERT( s.setValue("it works") )
    TS_ASSERT_EQUALS( s.operator()(), "it works" )
	}

	void testCopyConstructor()
	{
	  PropertyWithValue<int> i = *iProp;
    TS_ASSERT( ! i.name().compare("intProp") )
    TS_ASSERT( ! i.documentation().compare("") )
    TS_ASSERT( typeid( int ) == *i.type_info()  )
    TS_ASSERT( i.isDefault() )
    TS_ASSERT_EQUALS( i, 1 )
	}
	
	void testCopyAssignmentOperator()
	{
	  // TODO
	}

	void testAssignmentOperator()
	{
		// TODO: Implement test_global_Mantid_Kernel_PropertyWithValue_operator =() function.
	}

	void testOperatorBrackets()
	{
    TS_ASSERT_EQUALS( iProp->operator()(), 1 )
    TS_ASSERT_EQUALS( dProp->operator()(), 9.99 )
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" )
	}

	void testOperatorNothing()
	{
    int i = *iProp;
    TS_ASSERT_EQUALS( i, 1 )
    double d = *dProp;
    TS_ASSERT_EQUALS( d, 9.99 )
    std::string str(*sProp);
    TS_ASSERT( ! str.compare("theValue") )
	}

private:
	PropertyWithValue<int> *iProp;
	PropertyWithValue<double> *dProp;
	PropertyWithValue<std::string> *sProp;
};

#endif /*PROPERTYWITHVALUETEST_H_*/
