#ifndef PROPERTYWITHVALUETEST_H_
#define PROPERTYWITHVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ListValidator.h"

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

  ~PropertyWithValueTest()
  {
    delete iProp;
    delete dProp;
    delete sProp;
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
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    TS_ASSERT( ! dProp->value().substr(0,4).compare("9.99") )
    TS_ASSERT( ! sProp->value().compare("theValue") )
  }

  void testSetValue()
  {
    PropertyWithValue<int> i("test", 1);
    TS_ASSERT_EQUALS( i.setValue("10"), "" )
    TS_ASSERT_EQUALS( i, 10 )
    TS_ASSERT_EQUALS( i.setValue("9.99"),
      "Could not set property test. Can not convert \"9.99\" to " + i.type() )
    TS_ASSERT_EQUALS( i.setValue("garbage"),
      "Could not set property test. Can not convert \"garbage\" to " + i.type() )

    PropertyWithValue<double> d("test", 5.55);
    TS_ASSERT_EQUALS( d.setValue("-9.99"), "" )
    TS_ASSERT_EQUALS( d, -9.99 )
    TS_ASSERT_EQUALS( d.setValue("0"), "" )
    TS_ASSERT_EQUALS( d, 0 )
    TS_ASSERT_EQUALS( d.setValue("garbage"),
      "Could not set property test. Can not convert \"garbage\" to " + d.type())

    PropertyWithValue<std::string> s("test", "test");
    TS_ASSERT_EQUALS( s.setValue("-9.99"), "" )
    TS_ASSERT_EQUALS( s.operator()(), "-9.99" )
    TS_ASSERT_EQUALS( s.setValue("0"), "" )
    TS_ASSERT_EQUALS( s.operator()(), "0" )
    TS_ASSERT_EQUALS( s.setValue("it works"), "" )
    TS_ASSERT_EQUALS( s.operator()(), "it works" )
  }

  void testGetValue()
  {
    PropertyWithValue<std::string> s("defau=theDef", "theDef");
    TS_ASSERT_EQUALS( s.getDefault(), "theDef" )
    TS_ASSERT_EQUALS( s.setValue("somethingElse"), "" )
    TS_ASSERT_EQUALS( s.getDefault(), "theDef" )

    PropertyWithValue<int> i("defau1", 3);
    TS_ASSERT_EQUALS( i.getDefault(), "3" )
    TS_ASSERT_EQUALS( i.setValue("5"), "" )
    TS_ASSERT_EQUALS( i.getDefault(), "3" )
    TS_ASSERT_EQUALS( i.setValue("garbage"),
      "Could not set property defau1. Can not convert \"garbage\" to " + i.type() )
    TS_ASSERT_EQUALS( i.getDefault(), "3" )

    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    PropertyWithValue<double> d("defau3.33", 3.33);
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" )
    TS_ASSERT_EQUALS( d.setValue("1.6"), "" )
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" )
    TS_ASSERT_EQUALS( d.setValue("garbage"),
      "Could not set property defau3.33. Can not convert \"garbage\" to " + d.type() )
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" )
  }

  void testCopyConstructor()
  {
    PropertyWithValue<int> i = *iProp;
    TS_ASSERT( ! i.name().compare("intProp") )
    TS_ASSERT( ! i.documentation().compare("") )
    TS_ASSERT( typeid( int ) == *i.type_info()  )
    TS_ASSERT( i.isDefault() )
    TS_ASSERT_EQUALS( i, 1 )

    PropertyWithValue<double> d = *dProp;
    TS_ASSERT( ! d.name().compare("doubleProp") )
    TS_ASSERT( ! d.documentation().compare("") )
    TS_ASSERT( typeid( double ) == *d.type_info()  )
    TS_ASSERT( d.isDefault() )
    TS_ASSERT_EQUALS( d, 9.99 )

    PropertyWithValue<std::string> s = *sProp;
    TS_ASSERT( ! s.name().compare("stringProp") )
    TS_ASSERT( ! s.documentation().compare("") )
    TS_ASSERT( typeid( std::string ) == *s.type_info()  )
    TS_ASSERT( s.isDefault() )
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" )
  }

  void testCopyAssignmentOperator()
  {
    PropertyWithValue<int> i("Prop1",5);
    i = *iProp;
    TS_ASSERT( ! i.name().compare("Prop1") )
    TS_ASSERT( ! i.documentation().compare("") )
    TS_ASSERT( ! i.isDefault() )
    TS_ASSERT_EQUALS( i, 1 )

    PropertyWithValue<double> d("Prop2",5.5);
    d = *dProp;
    TS_ASSERT( ! d.name().compare("Prop2") )
    TS_ASSERT( ! d.documentation().compare("") )
    TS_ASSERT( ! d.isDefault() )
    TS_ASSERT_EQUALS( d, 9.99 )

    PropertyWithValue<std::string> s("Prop3","test");
    s = *sProp;
    TS_ASSERT( ! s.name().compare("Prop3") )
    TS_ASSERT( ! s.documentation().compare("") )
    TS_ASSERT( ! s.isDefault() )
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" )
  }

  void testAssignmentOperator()
  {
    PropertyWithValue<int> i("Prop1",5);
    TS_ASSERT_EQUALS( i = 2, 2 )
    TS_ASSERT( !i.isDefault() )
    i = 5;
    TS_ASSERT( i.isDefault() )

    PropertyWithValue<double> d("Prop2",5.5);
    TS_ASSERT_EQUALS( d = 7.77, 7.77 )
    TS_ASSERT( !d.isDefault() )
    d = 5.5;
    TS_ASSERT( d.isDefault() )

    PropertyWithValue<std::string> s("Prop3", "testing");
    s = "test";
    TS_ASSERT_EQUALS( s.operator()(), "test" )
    TS_ASSERT( !s.isDefault() )
    s = "testing";
    TS_ASSERT( i.isDefault() )

    PropertyWithValue<int> ii("Prop1.1",6);
    i = ii = 10;
    TS_ASSERT_EQUALS( ii, 10 )
    TS_ASSERT_EQUALS( i, 10 )

    PropertyWithValue<double> dd("Prop2.2",6.5);
    d = dd = 1.111;
    TS_ASSERT_EQUALS( dd, 1.111 )
    TS_ASSERT_EQUALS( d, 1.111 )

    PropertyWithValue<std::string> ss("Prop3", "testing2");
    s = ss = "tested";
    TS_ASSERT_EQUALS( ss.operator()(), "tested" )
    TS_ASSERT_EQUALS( s.operator()(), "tested" )
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

  void testAllowedValues()
  {
    TS_ASSERT( iProp->allowedValues().empty() )
    TS_ASSERT( dProp->allowedValues().empty() )
    TS_ASSERT( sProp->allowedValues().empty() )
    // Tests using a ListValidator are below
  }

  void testCasting()
  {
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) )
    PropertyWithValue<int> i("Prop1",5);
    Property *p = dynamic_cast<Property*>(&i);
    TS_ASSERT( ! p->name().compare("Prop1") )
    TS_ASSERT( ! p->value().compare("5") )
    TS_ASSERT_EQUALS( p->setValue("10"), "" )
    TS_ASSERT( ! p->value().compare("10") )
    TS_ASSERT_EQUALS( i, 10 )

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) )
    PropertyWithValue<double> d("Prop2",5.5);
    Property *pp = dynamic_cast<Property*>(&d);
    TS_ASSERT( ! pp->name().compare("Prop2") )
    TS_ASSERT( ! pp->value().compare("5.5") )
    TS_ASSERT_EQUALS( pp->setValue("7.777"), "" )
    
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
      TS_ASSERT( ! pp->value().substr(0,5).compare("7.777") )
    TS_ASSERT_EQUALS( d, 7.777 )

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) )
    PropertyWithValue<std::string> s("Prop3", "testing");
    Property *ppp = dynamic_cast<Property*>(&s);
    TS_ASSERT( ! ppp->name().compare("Prop3") )
    TS_ASSERT( ! ppp->value().compare("testing") )
    TS_ASSERT_EQUALS( ppp->setValue("newValue"), "" )
    TS_ASSERT( ! ppp->value().compare("newValue") )
    TS_ASSERT_EQUALS( s.operator()(), "newValue" )
  }

  void testMandatoryValidator()
  {
    PropertyWithValue<std::string> p("test", "", new MandatoryValidator<std::string>());
    TS_ASSERT_EQUALS( p.isValid(), "A value must be entered for this parameter");
    TS_ASSERT_EQUALS( p.setValue("I'm here"), "" );
    TS_ASSERT_EQUALS(p.isValid(), "" )
    TS_ASSERT_EQUALS( p.setValue(""), "A value must be entered for this parameter" )
    TS_ASSERT_EQUALS(p.value(),"I'm here");
  }

  void testIntBoundedValidator()
  {
    std::string start("Selected value "), end(")"),
      greaterThan(" is > the upper bound ("),
      lessThan(" is < the lower bound (");

	//int tests
	PropertyWithValue<int> pi("test", 11, new BoundedValidator<int>(1,10));
    TS_ASSERT_EQUALS(pi.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS( pi.setValue("0"), start + "0" + lessThan + "1" + end );
    TS_ASSERT_EQUALS(pi.value(),"11");
    TS_ASSERT_EQUALS(pi.isValid(), start + "11" + greaterThan + "10" + end );
    TS_ASSERT_EQUALS( pi.setValue("1"), "" );
    TS_ASSERT_EQUALS(pi.isValid(), "");
    TS_ASSERT_EQUALS( pi.setValue("10"), "" );
    TS_ASSERT_EQUALS(pi.isValid(), "");
    TS_ASSERT_EQUALS( pi.setValue("11"), start + "11" + greaterThan + "10" + end )
    TS_ASSERT_EQUALS(pi.value(),"10");
    TS_ASSERT_EQUALS(pi.isValid(), "")
    std::string errorMsg = pi.setValue("");
    //when the string can't be converted to the correct type we get a system dependent meassage that in this case should look like the string below
    TS_ASSERT_EQUALS( errorMsg.find("Could not set property test. Can not convert \"\" to ",0), 0)
  
	//double tests
    PropertyWithValue<double> pd("test", 11.0, new BoundedValidator<double>(1.0,10.0));
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS( pd.setValue("0.9"), start + "0.9" + lessThan + "1" + end );
    TS_ASSERT_EQUALS(pd.value(),"11");
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end )
    TS_ASSERT_EQUALS( pd.setValue("1"), "" );
    TS_ASSERT_EQUALS(pd.isValid(), "");
    TS_ASSERT_EQUALS( pd.setValue("10"), "" );
    TS_ASSERT_EQUALS(pd.isValid(), "");
    TS_ASSERT_EQUALS( pd.setValue("10.1"), start + "10.1" + greaterThan + "10" + end );
    TS_ASSERT_EQUALS(pd.value(),"10");
    TS_ASSERT_EQUALS(pd.isValid(), "");

	//string tests
    PropertyWithValue<std::string> ps("test", "", new BoundedValidator<std::string>("B","T"));
    TS_ASSERT_EQUALS(ps.isValid(), start + "" + lessThan + "B" + end);
    TS_ASSERT_EQUALS( ps.setValue("AZ"), start + "AZ" + lessThan + "B" + end )
    TS_ASSERT_EQUALS(ps.value(),"");
    TS_ASSERT_EQUALS(ps.isValid(), start + "" + lessThan + "B" + end);
    TS_ASSERT_EQUALS( ps.setValue("B"), "" );
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS( ps.setValue("T"), "" );
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS( ps.setValue("TA"), start + "TA" + greaterThan + "T" + end);
    TS_ASSERT_EQUALS(ps.value(),"T");
    TS_ASSERT_EQUALS(ps.isValid(), "");
  }

  void testListValidator()
  {
    std::string start("The value '"),
      end("' is not in the list of allowed values");

    std::vector<std::string> empt, vec;
    PropertyWithValue<std::string> empty("test","", new ListValidator(empt));
    TS_ASSERT_EQUALS( empty.isValid(), "Select a value" )
    vec.push_back("one");
    vec.push_back("two");
    PropertyWithValue<std::string> p("test","", new ListValidator(vec));
    TS_ASSERT_EQUALS( p.isValid(), "Select a value" )
    TS_ASSERT_EQUALS( p.setValue("one"), "" )
    TS_ASSERT_EQUALS( p.isValid(), "" )
    TS_ASSERT_EQUALS( p.setValue("two"), "" )
    TS_ASSERT_EQUALS( p.isValid(), "" )
    TS_ASSERT_EQUALS( p.setValue("three"), "The value \"three\" is not in the list of allowed values" )
    TS_ASSERT_EQUALS( p.value(), "two" )
    TS_ASSERT_EQUALS( p.isValid(), "" )
    std::set<std::string> vals;
    TS_ASSERT_THROWS_NOTHING( vals = p.allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 2 )
    TS_ASSERT( vals.count("one") )
    TS_ASSERT( vals.count("two") )
  }
  
  void testIsDefault()
  {
    TS_ASSERT_EQUALS( iProp->setValue("1"), "" );
    //1 is was the initial value and so the next test should pass
    TS_ASSERT( iProp->isDefault() )
    TS_ASSERT_EQUALS( iProp->setValue("2"), "" );
    TS_ASSERT( !iProp->isDefault() ) 
  }
 
  
private:
  PropertyWithValue<int> *iProp;
  PropertyWithValue<double> *dProp;
  PropertyWithValue<std::string> *sProp;
};

#endif /*PROPERTYWITHVALUETEST_H_*/
