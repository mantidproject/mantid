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
  static PropertyWithValueTest *createSuite() { return new PropertyWithValueTest(); }
  static void destroySuite(PropertyWithValueTest *suite) { delete suite; }

  PropertyWithValueTest()
  {
    iProp = new PropertyWithValue<int>("intProp", 1);
    dProp = new PropertyWithValue<double>("doubleProp", 9.99);
    sProp = new PropertyWithValue<std::string>("stringProp", "theValue");
    lProp = new PropertyWithValue<long long>("int64Prop",-9876543210987654LL);
  }

  ~PropertyWithValueTest()
  {
    delete iProp;
    delete dProp;
    delete sProp;
    delete lProp;
  }

  void testConstructor()
  {
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT( ! iProp->name().compare("intProp") );
    TS_ASSERT( ! iProp->documentation().compare("") );
    TS_ASSERT( typeid( int ) == *iProp->type_info()  );
    TS_ASSERT( iProp->isDefault() );

    TS_ASSERT( ! dProp->name().compare("doubleProp") );
    TS_ASSERT( ! dProp->documentation().compare("") );
    TS_ASSERT( typeid( double ) == *dProp->type_info()  );
    TS_ASSERT( dProp->isDefault() );

    TS_ASSERT( ! sProp->name().compare("stringProp") );
    TS_ASSERT( ! sProp->documentation().compare("") );
    TS_ASSERT( typeid( std::string ) == *sProp->type_info()  );
    TS_ASSERT( sProp->isDefault() );

    TS_ASSERT( ! lProp->name().compare("int64Prop") );
    TS_ASSERT( ! lProp->documentation().compare("") );
    TS_ASSERT( typeid( long long ) == *lProp->type_info()  );
    TS_ASSERT( lProp->isDefault() );
  }

  void testValue()
  {
    TS_ASSERT( ! iProp->value().compare("1") );
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    TS_ASSERT( ! dProp->value().substr(0,4).compare("9.99") );
    TS_ASSERT( ! sProp->value().compare("theValue") );
    TS_ASSERT( ! lProp->value().compare("-9876543210987654") );
  }

  void testSetValue()
  {
    PropertyWithValue<int> i("test", 1);
    TS_ASSERT_EQUALS( i.setValue("10"), "" );
    TS_ASSERT_EQUALS( i, 10 );
    TS_ASSERT_EQUALS( i.setValue("9.99"),
      "Could not set property test. Can not convert \"9.99\" to " + i.type() );
    TS_ASSERT_EQUALS( i.setValue("garbage"),
      "Could not set property test. Can not convert \"garbage\" to " + i.type() );

    PropertyWithValue<double> d("test", 5.55);
    TS_ASSERT_EQUALS( d.setValue("-9.99"), "" );
    TS_ASSERT_EQUALS( d, -9.99 );
    TS_ASSERT_EQUALS( d.setValue("0"), "" );
    TS_ASSERT_EQUALS( d, 0 );
    TS_ASSERT_EQUALS( d.setValue("garbage"),
      "Could not set property test. Can not convert \"garbage\" to " + d.type());

    PropertyWithValue<std::string> s("test", "test");
    TS_ASSERT_EQUALS( s.setValue("-9.99"), "" );
    TS_ASSERT_EQUALS( s.operator()(), "-9.99" );
    TS_ASSERT_EQUALS( s.setValue("0"), "" );
    TS_ASSERT_EQUALS( s.operator()(), "0" );
    TS_ASSERT_EQUALS( s.setValue("it works"), "" );
    TS_ASSERT_EQUALS( s.operator()(), "it works" );

    PropertyWithValue<long long> l("test", 1);
    TS_ASSERT_EQUALS( l.setValue("10"), "" );
    TS_ASSERT_EQUALS( l, 10 );
    TS_ASSERT_EQUALS( l.setValue("1234567890123456"), "" );
    TS_ASSERT_EQUALS( l, 1234567890123456LL );
    TS_ASSERT_EQUALS( l.setValue("9.99"),
      "Could not set property test. Can not convert \"9.99\" to " + l.type() );
    TS_ASSERT_EQUALS( l.setValue("garbage"),
      "Could not set property test. Can not convert \"garbage\" to " + l.type() );
  }

  void testGetDefault()
  {
    PropertyWithValue<std::string> s("defau=theDef", "theDef");
    TS_ASSERT_EQUALS( s.getDefault(), "theDef" );
    TS_ASSERT_EQUALS( s.setValue("somethingElse"), "" );
    TS_ASSERT_EQUALS( s.getDefault(), "theDef" );

    PropertyWithValue<int> i("defau1", 3);
    TS_ASSERT_EQUALS( i.getDefault(), "3" );
    TS_ASSERT_EQUALS( i.setValue("5"), "" );
    TS_ASSERT_EQUALS( i.getDefault(), "3" );
    TS_ASSERT_EQUALS( i.setValue("garbage"),
      "Could not set property defau1. Can not convert \"garbage\" to " + i.type() );
    TS_ASSERT_EQUALS( i.getDefault(), "3" );

    PropertyWithValue<long long> l("defau1", 987987987987LL);
    TS_ASSERT_EQUALS( l.getDefault(), "987987987987" );
    TS_ASSERT_EQUALS( l.setValue("5"), "" );
    TS_ASSERT_EQUALS( l.getDefault(), "987987987987" );
    TS_ASSERT_EQUALS( l.setValue("garbage"),
      "Could not set property defau1. Can not convert \"garbage\" to " + l.type() );
    TS_ASSERT_EQUALS( l.getDefault(), "987987987987" );

    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    PropertyWithValue<double> d("defau3.33", 3.33);
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" );
    TS_ASSERT_EQUALS( d.setValue("1.6"), "" );
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" );
    TS_ASSERT_EQUALS( d.setValue("garbage"),
      "Could not set property defau3.33. Can not convert \"garbage\" to " + d.type() );
    TS_ASSERT_EQUALS( d.getDefault().substr(0,4), "3.33" );
  }

  void testCopyConstructor()
  {
    PropertyWithValue<int> i = *iProp;
    TS_ASSERT( ! i.name().compare("intProp") );
    TS_ASSERT( ! i.documentation().compare("") );
    TS_ASSERT( typeid( int ) == *i.type_info()  );
    TS_ASSERT( i.isDefault() );
    TS_ASSERT_EQUALS( i, 1 );

    PropertyWithValue<double> d = *dProp;
    TS_ASSERT( ! d.name().compare("doubleProp") );
    TS_ASSERT( ! d.documentation().compare("") );
    TS_ASSERT( typeid( double ) == *d.type_info()  );
    TS_ASSERT( d.isDefault() );
    TS_ASSERT_EQUALS( d, 9.99 );

    PropertyWithValue<std::string> s = *sProp;
    TS_ASSERT( ! s.name().compare("stringProp") );
    TS_ASSERT( ! s.documentation().compare("") );
    TS_ASSERT( typeid( std::string ) == *s.type_info()  );
    TS_ASSERT( s.isDefault() );
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" );

    PropertyWithValue<long long> l = *lProp;
    TS_ASSERT( ! lProp->name().compare("int64Prop") );
    TS_ASSERT( ! lProp->documentation().compare("") );
    TS_ASSERT( typeid( long long ) == *lProp->type_info()  );
    TS_ASSERT( lProp->isDefault() );
    TS_ASSERT_EQUALS( l, -9876543210987654LL );

  }

  void testCopyAssignmentOperator()
  {
    PropertyWithValue<int> i("Prop1",5);
    i = *iProp;
    TS_ASSERT( ! i.name().compare("Prop1") );
    TS_ASSERT( ! i.documentation().compare("") );
    TS_ASSERT( ! i.isDefault() );
    TS_ASSERT_EQUALS( i, 1 );

    PropertyWithValue<double> d("Prop2",5.5);
    d = *dProp;
    TS_ASSERT( ! d.name().compare("Prop2") );
    TS_ASSERT( ! d.documentation().compare("") );
    TS_ASSERT( ! d.isDefault() );
    TS_ASSERT_EQUALS( d, 9.99 );

    PropertyWithValue<std::string> s("Prop3","test");
    s = *sProp;
    TS_ASSERT( ! s.name().compare("Prop3") );
    TS_ASSERT( ! s.documentation().compare("") );
    TS_ASSERT( ! s.isDefault() );
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" );

    PropertyWithValue<long long> l("Prop4",5);
    l = *lProp;
    TS_ASSERT( ! l.name().compare("Prop4") );
    TS_ASSERT( ! l.documentation().compare("") );
    TS_ASSERT( ! l.isDefault() );
    TS_ASSERT_EQUALS( l, -9876543210987654LL );

  }

  void testAssignmentOperator()
  {
    PropertyWithValue<int> i("Prop1",5);
    TS_ASSERT_EQUALS( i = 2, 2 );
    TS_ASSERT( !i.isDefault() );
    i = 5;
    TS_ASSERT( i.isDefault() );

    PropertyWithValue<double> d("Prop2",5.5);
    TS_ASSERT_EQUALS( d = 7.77, 7.77 );
    TS_ASSERT( !d.isDefault() );
    d = 5.5;
    TS_ASSERT( d.isDefault() );

    PropertyWithValue<std::string> s("Prop3", "testing");
    s = "test";
    TS_ASSERT_EQUALS( s.operator()(), "test" );
    TS_ASSERT( !s.isDefault() );
    s = "testing";
    TS_ASSERT( i.isDefault() );

    PropertyWithValue<long long> l("Prop4",987987987987LL);
    TS_ASSERT_EQUALS( l = 2, 2 );
    TS_ASSERT( !l.isDefault() );
    l = 987987987987LL;
    TS_ASSERT( l.isDefault() );

    PropertyWithValue<int> ii("Prop1.1",6);
    i = ii = 10;
    TS_ASSERT_EQUALS( ii, 10 );
    TS_ASSERT_EQUALS( i, 10 );

    PropertyWithValue<double> dd("Prop2.2",6.5);
    d = dd = 1.111;
    TS_ASSERT_EQUALS( dd, 1.111 );
    TS_ASSERT_EQUALS( d, 1.111 );

    PropertyWithValue<std::string> ss("Prop3.3", "testing2");
    s = ss = "tested";
    TS_ASSERT_EQUALS( ss.operator()(), "tested" );
    TS_ASSERT_EQUALS( s.operator()(), "tested" );

    PropertyWithValue<long long> ll("Prop4.4",6);
    l = ll = 789789789789LL;
    TS_ASSERT_EQUALS( ll, 789789789789LL );
    TS_ASSERT_EQUALS( l, 789789789789LL );
  }

  void testOperatorBrackets()
  {
    TS_ASSERT_EQUALS( iProp->operator()(), 1 );
    TS_ASSERT_EQUALS( dProp->operator()(), 9.99 );
    TS_ASSERT_EQUALS( sProp->operator()(), "theValue" );
    TS_ASSERT_EQUALS( lProp->operator()(), -9876543210987654LL );
  }

  void testPlusEqualOperator()
  {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    PropertyWithValue< std::vector<int> > * pv = new PropertyWithValue< std::vector<int> >("some_array", v);
    PropertyWithValue< std::vector<int> > * pv2 = new PropertyWithValue< std::vector<int> >("some_array", v);
    (*pv) += pv2;
    TS_ASSERT_EQUALS(pv->value(), "1,2,3,1,2,3")
  }

  void testPlusEqualOperatorOnYourself()
  {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    PropertyWithValue< std::vector<int> > * pv = new PropertyWithValue< std::vector<int> >("some_array", v);
    (*pv) += pv;
    TS_ASSERT_EQUALS(pv->value(), "1,2,3,1,2,3")
  }

  void testOperatorNothing()
  {
    int i = *iProp;
    TS_ASSERT_EQUALS( i, 1 );
    double d = *dProp;
    TS_ASSERT_EQUALS( d, 9.99 );
    std::string str(*sProp);
    TS_ASSERT( ! str.compare("theValue") );
    long long l = *lProp;
    TS_ASSERT_EQUALS( l, -9876543210987654LL );
  }

  void testAllowedValues()
  {
    TS_ASSERT( iProp->allowedValues().empty() );
    TS_ASSERT( dProp->allowedValues().empty() );
    TS_ASSERT( sProp->allowedValues().empty() );
    TS_ASSERT( lProp->allowedValues().empty() );
    // Tests using a ListValidator are below
  }

  void testCasting()
  {
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) );
    PropertyWithValue<int> i("Prop1",5);
    Property *p = dynamic_cast<Property*>(&i);
    TS_ASSERT( ! p->name().compare("Prop1") );
    TS_ASSERT( ! p->value().compare("5") );
    TS_ASSERT_EQUALS( p->setValue("10"), "" );
    TS_ASSERT( ! p->value().compare("10") );
    TS_ASSERT_EQUALS( i, 10 );

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) );
    PropertyWithValue<double> d("Prop2",5.5);
    Property *pp = dynamic_cast<Property*>(&d);
    TS_ASSERT( ! pp->name().compare("Prop2") );
    TS_ASSERT( ! pp->value().compare("5.5") );
    TS_ASSERT_EQUALS( pp->setValue("7.777"), "" );
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    TS_ASSERT( ! pp->value().substr(0,5).compare("7.777") );
    TS_ASSERT_EQUALS( d, 7.777 );

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) );
    PropertyWithValue<std::string> s("Prop3", "testing");
    Property *ppp = dynamic_cast<Property*>(&s);
    TS_ASSERT( ! ppp->name().compare("Prop3") );
    TS_ASSERT( ! ppp->value().compare("testing") );
    TS_ASSERT_EQUALS( ppp->setValue("newValue"), "" );
    TS_ASSERT( ! ppp->value().compare("newValue") );
    TS_ASSERT_EQUALS( s.operator()(), "newValue" );

    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(lProp), static_cast<Property*>(0) );
    PropertyWithValue<long long> l("Prop4",789789789789LL);
    Property *pppp = dynamic_cast<Property*>(&l);
    TS_ASSERT( ! pppp->name().compare("Prop4") );
    TS_ASSERT( ! pppp->value().compare("789789789789") );
    TS_ASSERT_EQUALS( pppp->setValue("10"), "" );
    TS_ASSERT( ! pppp->value().compare("10") );
    TS_ASSERT_EQUALS( l, 10 );
  }

  void testMandatoryValidator()
  {
    PropertyWithValue<std::string> p("test", "", new MandatoryValidator<std::string>());
    TS_ASSERT_EQUALS( p.isValid(), "A value must be entered for this parameter");
    TS_ASSERT_EQUALS( p.setValue("I'm here"), "" );
    TS_ASSERT_EQUALS(p.isValid(), "" );
    TS_ASSERT_EQUALS( p.setValue(""), "A value must be entered for this parameter" );
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
    TS_ASSERT_EQUALS( pi.setValue("11"), start + "11" + greaterThan + "10" + end );
    TS_ASSERT_EQUALS(pi.value(),"10");
    TS_ASSERT_EQUALS(pi.isValid(), "");
    std::string errorMsg = pi.setValue("");
    //when the string can't be converted to the correct type we get a system dependent meassage that in this case should look like the string below
    TS_ASSERT_EQUALS( errorMsg.find("Could not set property test. Can not convert \"\" to ",0), 0);
  
    //double tests
    PropertyWithValue<double> pd("test", 11.0, new BoundedValidator<double>(1.0,10.0));
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end);
    TS_ASSERT_EQUALS( pd.setValue("0.9"), start + "0.9" + lessThan + "1" + end );
    TS_ASSERT_EQUALS(pd.value(),"11");
    TS_ASSERT_EQUALS(pd.isValid(), start + "11" + greaterThan + "10" + end );
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
    TS_ASSERT_EQUALS( ps.setValue("AZ"), start + "AZ" + lessThan + "B" + end );
    TS_ASSERT_EQUALS(ps.value(),"");
    TS_ASSERT_EQUALS(ps.isValid(), start + "" + lessThan + "B" + end);
    TS_ASSERT_EQUALS( ps.setValue("B"), "" );
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS( ps.setValue("T"), "" );
    TS_ASSERT_EQUALS(ps.isValid(), "");
    TS_ASSERT_EQUALS( ps.setValue("TA"), start + "TA" + greaterThan + "T" + end);
    TS_ASSERT_EQUALS(ps.value(),"T");
    TS_ASSERT_EQUALS(ps.isValid(), "");

    //int64 tests
    PropertyWithValue<long long> pl("test", 987987987987LL, new BoundedValidator<long long>(0,789789789789LL));
    TS_ASSERT_EQUALS( pl.isValid(), start + "987987987987" + greaterThan + "789789789789" + end);
    TS_ASSERT_EQUALS( pl.setValue("-1"), start + "-1" + lessThan + "0" + end );
    TS_ASSERT_EQUALS( pl.value(),"987987987987");
    TS_ASSERT_EQUALS( pl.setValue("0"), "" );
    TS_ASSERT_EQUALS( pl.isValid(), "");
    TS_ASSERT_EQUALS( pl.setValue("789789789789"), "" );
    TS_ASSERT_EQUALS( pl.isValid(), "");
    TS_ASSERT_EQUALS( pl.setValue("789789789790"), start + "789789789790" + greaterThan + "789789789789" + end );
    TS_ASSERT_EQUALS( pl.value(),"789789789789");
}

  void testListValidator()
  {
    std::string start("The value '"),
      end("' is not in the list of allowed values");

    std::vector<std::string> empt, vec;
    PropertyWithValue<std::string> empty("test","", new ListValidator(empt));
    TS_ASSERT_EQUALS( empty.isValid(), "Select a value" );
    vec.push_back("one");
    vec.push_back("two");
    PropertyWithValue<std::string> p("test","", new ListValidator(vec));
    TS_ASSERT_EQUALS( p.isValid(), "Select a value" );
    TS_ASSERT_EQUALS( p.setValue("one"), "" );
    TS_ASSERT_EQUALS( p.isValid(), "" );
    TS_ASSERT_EQUALS( p.setValue("two"), "" );
    TS_ASSERT_EQUALS( p.isValid(), "" );
    TS_ASSERT_EQUALS( p.setValue("three"), "The value \"three\" is not in the list of allowed values" );
    TS_ASSERT_EQUALS( p.value(), "two" );
    TS_ASSERT_EQUALS( p.isValid(), "" );
    std::set<std::string> vals;
    TS_ASSERT_THROWS_NOTHING( vals = p.allowedValues() );
    TS_ASSERT_EQUALS( vals.size(), 2 );
    TS_ASSERT( vals.count("one") );
    TS_ASSERT( vals.count("two") );
  }
  
  void testIsDefault()
  {
    TS_ASSERT_EQUALS( iProp->setValue("1"), "" );
    //1 is was the initial value and so the next test should pass
    TS_ASSERT( iProp->isDefault() );
    TS_ASSERT_EQUALS( iProp->setValue("2"), "" );
    TS_ASSERT( !iProp->isDefault() ) ;
  }



//class A
//{
//public:
//  virtual A& fun(const A& rhs)
//  {
//    std::cout << "fun() called from class A\n";
//  }
//};
//
//template <typename T>
//class B : public A
//{
//public:
//  B(T value) : m_value(value)
//  {
//
//  }
//
//  virtual A& fun(const A& rhs)
//  {
//    std::cout << "fun() called from class B<T>. I contain " << m_value << " and the parameter contains " << rhs.m_value << "\n";
//  }
//  T m_value;
//};
//
//  void testTemplates()
//  {
//    std::cout << "\n\n";
//    A myA;
//    B<int> myB(12);
//    myA.fun(myA);
//    myB.fun(myB);
//
//    A * myAptr = new A();
//    A * myBptr = new B<int>(23);
//
//    myAptr->fun(*myAptr);
//    myBptr->fun(*myBptr);
//
//  }





  void testAdditionOperator()
  {
    int i; double d;
    Property * p1;
    Property * p2;

    // --- Numbers are added together ----
    p1 = new PropertyWithValue<double>("Prop1", 12.0);
    p2 = new PropertyWithValue<double>("Prop1", 23.0);
    (*p1) += p2;
    PropertyWithValue<double> * pd = dynamic_cast< PropertyWithValue<double> * >(p1);
    d = *pd;
    TS_ASSERT_EQUALS( d, 35.0 );

    p1 = new PropertyWithValue<int>("Prop1", 34);
    p2 = new PropertyWithValue<int>("Prop1", 62);
    (*p1) += p2;
    PropertyWithValue<int> * pi = dynamic_cast< PropertyWithValue<int> * >(p1);
    i = *pi;
    TS_ASSERT_EQUALS( i, 96 );

    // --- Vectors are appennded together ----
    std::vector<int> v1, v2;
    v1.push_back(1); v1.push_back(2); v1.push_back(3);
    v1.push_back(4); v1.push_back(5); v1.push_back(6);
    p1 = new PropertyWithValue< std::vector<int> >("Prop1", v1);
    p2 = new PropertyWithValue< std::vector<int> >("Prop1", v2);
    (*p1) += p2;
    PropertyWithValue< std::vector<int> > * pvi = dynamic_cast< PropertyWithValue< std::vector<int> > * >(p1);
    std::vector<int> v3 = *pvi;
    TS_ASSERT_EQUALS( v3.size(), 6 );

  }
  
private:
  PropertyWithValue<int> *iProp;
  PropertyWithValue<double> *dProp;
  PropertyWithValue<std::string> *sProp;
  PropertyWithValue<long long> *lProp;
};

#endif /*PROPERTYWITHVALUETEST_H_*/
