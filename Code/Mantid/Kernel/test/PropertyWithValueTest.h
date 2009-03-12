#ifndef PROPERTYWITHVALUETEST_H_
#define PROPERTYWITHVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/BoundedValidator.h"

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
    PropertyWithValue<double> d("Prop2",5.5);
    TS_ASSERT_EQUALS( d = 7.77, 7.77 )
    PropertyWithValue<std::string> s("Prop3", "testing");
    s = "test";
    TS_ASSERT_EQUALS( s.operator()(), "test" )

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
    TS_ASSERT( p->setValue("10") )
    TS_ASSERT( ! p->value().compare("10") )
    TS_ASSERT_EQUALS( i, 10 )

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) )
    PropertyWithValue<double> d("Prop2",5.5);
    Property *pp = dynamic_cast<Property*>(&d);
    TS_ASSERT( ! pp->name().compare("Prop2") )
    TS_ASSERT( ! pp->value().compare("5.5") )
    TS_ASSERT( pp->setValue("7.777") )
    TS_ASSERT( ! pp->value().compare("7.777") )
    TS_ASSERT_EQUALS( d, 7.777 )

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) )
    PropertyWithValue<std::string> s("Prop3", "testing");
    Property *ppp = dynamic_cast<Property*>(&s);
    TS_ASSERT( ! ppp->name().compare("Prop3") )
    TS_ASSERT( ! ppp->value().compare("testing") )
    TS_ASSERT( ppp->setValue("newValue") )
    TS_ASSERT( ! ppp->value().compare("newValue") )
    TS_ASSERT_EQUALS( s.operator()(), "newValue" )
  }

  void testMandatoryValidator()
  {
    PropertyWithValue<std::string> p("test", "", new MandatoryValidator<std::string>());
    TS_ASSERT_EQUALS(p.isValid(),false);
    TS_ASSERT( p.setValue("I'm here"));
    TS_ASSERT_EQUALS(p.isValid(),true);
    TS_ASSERT( ! p.setValue(""));
    TS_ASSERT_EQUALS(p.value(),"I'm here");
  }

  void testIntBoundedValidator()
  {
    PropertyWithValue<int> p("test", 11, new BoundedValidator<int>(1,10));
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( ! p.setValue("0") );
    TS_ASSERT_EQUALS(p.value(),"11");
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( p.setValue("1") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( p.setValue("10") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( ! p.setValue("11") );
    TS_ASSERT_EQUALS(p.value(),"10");
    TS_ASSERT_EQUALS(p.isValid(), true);
  }

  void testDoubleBoundedValidator()
  {
    PropertyWithValue<double> p("test", 11.0, new BoundedValidator<double>(1.0,10.0));
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( ! p.setValue("0.9") );
    TS_ASSERT_EQUALS(p.value(),"11");
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( p.setValue("1") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( p.setValue("10") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( ! p.setValue("10.1") );
    TS_ASSERT_EQUALS(p.value(),"10");
    TS_ASSERT_EQUALS(p.isValid(), true);
  }

  void testStringBoundedValidator()
  {
    PropertyWithValue<std::string> p("test", "", new BoundedValidator<std::string>("B","T"));
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( ! p.setValue("AZ") );
    TS_ASSERT_EQUALS(p.value(),"");
    TS_ASSERT_EQUALS(p.isValid(), false);
    TS_ASSERT( p.setValue("B") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( p.setValue("T") );
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( ! p.setValue("TA") );
    TS_ASSERT_EQUALS(p.value(),"T");
    TS_ASSERT_EQUALS(p.isValid(), true);
    TS_ASSERT( dynamic_cast<const BoundedValidator<std::string>* >(p.getValidator()) )
  }

  void testListValidator()
  {
    std::vector<std::string> vec;
    vec.push_back("one");
    vec.push_back("two");
    PropertyWithValue<std::string> p("test","", new ListValidator(vec));
    TS_ASSERT( ! p.isValid() )
    TS_ASSERT( p.setValue("one") )
    TS_ASSERT( p.isValid() )
    TS_ASSERT( p.setValue("two") )
    TS_ASSERT( p.isValid() )
    TS_ASSERT( ! p.setValue("three") )
    TS_ASSERT_EQUALS( p.value(), "two" )
    TS_ASSERT( p.isValid() )
    std::vector<std::string> vals;
    TS_ASSERT_THROWS_NOTHING( vals = p.allowedValues() )
    TS_ASSERT_EQUALS( vals.size(), 2 )
    TS_ASSERT_EQUALS( vals[0], "one" )
    TS_ASSERT_EQUALS( vals[1], "two" )
    TS_ASSERT( dynamic_cast<const ListValidator*>(p.getValidator()) )
  }
  
private:
  PropertyWithValue<int> *iProp;
  PropertyWithValue<double> *dProp;
  PropertyWithValue<std::string> *sProp;
};

#endif /*PROPERTYWITHVALUETEST_H_*/
