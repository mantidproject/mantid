#ifndef ARRAYPROPERTYTEST_H_
#define ARRAYPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ArrayProperty.h"

using namespace Mantid::Kernel;

class ArrayPropertyTest : public CxxTest::TestSuite
{
public:
  void setUp()
  {
    iProp = new ArrayProperty<int>("intProp");
    dProp = new ArrayProperty<double>("doubleProp");
    sProp = new ArrayProperty<std::string>("stringProp");    
  }
  
  void tearDown()
  {
    delete iProp;
    delete dProp;
    delete sProp;
  }
  
  void testConstructor()
  {
    TS_ASSERT( ! iProp->name().compare("intProp") )
    TS_ASSERT( ! iProp->documentation().compare("") )
    TS_ASSERT( typeid( std::vector<int> ) == *iProp->type_info()  )
    TS_ASSERT( iProp->isDefault() )
    TS_ASSERT( iProp->operator()().empty() )
    
    TS_ASSERT( ! dProp->name().compare("doubleProp") )
    TS_ASSERT( ! dProp->documentation().compare("") )
    TS_ASSERT( typeid( std::vector<double> ) == *dProp->type_info()  )
    TS_ASSERT( dProp->isDefault() )
    TS_ASSERT( dProp->operator()().empty() )
    
    TS_ASSERT( ! sProp->name().compare("stringProp") )
    TS_ASSERT( ! sProp->documentation().compare("") )
    TS_ASSERT( typeid( std::vector<std::string> ) == *sProp->type_info()  )
    TS_ASSERT( sProp->isDefault() )
    TS_ASSERT( sProp->operator()().empty() )
    
    std::vector<int> i(5,2);
    ArrayProperty<int> ip("ip",i);
    TS_ASSERT_EQUALS( ip.operator()().size(), 5 )
    TS_ASSERT_EQUALS( ip.operator()()[3], 2 )

    std::vector<double> d(4,6.66);
    ArrayProperty<double> dp("dp",d);
    TS_ASSERT_EQUALS( dp.operator()().size(), 4 )
    TS_ASSERT_EQUALS( dp.operator()()[1], 6.66 )

    std::vector<std::string> s(3,"yyy");
    ArrayProperty<std::string> sp("sp",s);
    TS_ASSERT_EQUALS( sp.operator()().size(), 3 )
    TS_ASSERT( ! sp.operator()()[2].compare("yyy") )
  }

  void testConstructorByString()
  {
	  ArrayProperty<int> i("i","1,2,3");
	  TS_ASSERT_EQUALS( i.operator()()[0], 1 )
    TS_ASSERT_EQUALS( i.operator()()[1], 2 )
    TS_ASSERT_EQUALS( i.operator()()[2], 3 )

    ArrayProperty<int> i2("i", "-1-1");
    TS_ASSERT_EQUALS( i2.operator()()[0], -1);
    TS_ASSERT_EQUALS( i2.operator()()[1], 0);
    TS_ASSERT_EQUALS( i2.operator()()[2], 1);

    ArrayProperty<int> i4("i", "-1:1");
    TS_ASSERT_EQUALS( i4.operator()()[0], -1);
    TS_ASSERT_EQUALS( i4.operator()()[1], 0);
    TS_ASSERT_EQUALS( i4.operator()()[2], 1);

    ArrayProperty<int> i5("i", "-3--1");
    TS_ASSERT_EQUALS( i5.operator()()[0], -3);
    TS_ASSERT_EQUALS( i5.operator()()[1], -2);
    TS_ASSERT_EQUALS( i5.operator()()[2], -1);

    ArrayProperty<int> i7("i", "-3:-1");
    TS_ASSERT_EQUALS( i7.operator()()[0], -3);
    TS_ASSERT_EQUALS( i7.operator()()[1], -2);
    TS_ASSERT_EQUALS( i7.operator()()[2], -1);

    ArrayProperty<unsigned int> i3("i", "0:2,5");
    TS_ASSERT_EQUALS( i3.operator()()[0], 0);
    TS_ASSERT_EQUALS( i3.operator()()[1], 1);
    TS_ASSERT_EQUALS( i3.operator()()[2], 2);
    TS_ASSERT_EQUALS( i3.operator()()[3], 5);

    ArrayProperty<unsigned int> i6("i", "5,0-2,5");
    TS_ASSERT_EQUALS( i6.operator()()[0], 5);
    TS_ASSERT_EQUALS( i6.operator()()[1], 0);
    TS_ASSERT_EQUALS( i6.operator()()[2], 1);
    TS_ASSERT_EQUALS( i6.operator()()[3], 2);
    TS_ASSERT_EQUALS( i6.operator()()[4], 5);

    ArrayProperty<double> d("d","7.77,8.88,9.99");
    TS_ASSERT_EQUALS( d.operator()()[0], 7.77 )
    TS_ASSERT_EQUALS( d.operator()()[1], 8.88 )
    TS_ASSERT_EQUALS( d.operator()()[2], 9.99 )

    ArrayProperty<std::string> s("d","a,b,c");
    TS_ASSERT( ! s.operator()()[0].compare("a") )
    TS_ASSERT( ! s.operator()()[1].compare("b") )
    TS_ASSERT( ! s.operator()()[2].compare("c") )

    TS_ASSERT_THROWS( ArrayProperty<int> ii("ii","aa,bb"), std::invalid_argument )
    TS_ASSERT_THROWS( ArrayProperty<int> ii("ii","5.5,6.6"), std::invalid_argument )
    TS_ASSERT_THROWS( ArrayProperty<double> dd("dd","aa,bb"), std::invalid_argument )
  }
	
  void testCopyConstructor()
  {
    ArrayProperty<int> i = *iProp;
    TS_ASSERT( ! i.name().compare("intProp") )
    TS_ASSERT( ! i.documentation().compare("") )
    TS_ASSERT( typeid( std::vector<int> ) == *i.type_info()  )
    TS_ASSERT( i.isDefault() )
    TS_ASSERT( i.operator()().empty() )
	    
    ArrayProperty<double> d = *dProp;
    TS_ASSERT( ! d.name().compare("doubleProp") )
    TS_ASSERT( ! d.documentation().compare("") )
    TS_ASSERT( typeid( std::vector<double> ) == *d.type_info()  )
    TS_ASSERT( d.isDefault() )
    TS_ASSERT( d.operator()().empty() )
	    
    ArrayProperty<std::string> s = *sProp;
    TS_ASSERT( ! s.name().compare("stringProp") )
    TS_ASSERT( ! s.documentation().compare("") )
    TS_ASSERT( typeid( std::vector<std::string> ) == *s.type_info()  )
    TS_ASSERT( s.isDefault() )
    TS_ASSERT( s.operator()().empty() )
  }

  void testValue()
  {
    std::vector<int> i(3,3);
    ArrayProperty<int> ip("ip",i);
    TS_ASSERT( ! ip.value().compare("3,3,3") )

    std::vector<double> d(4,1.23);
    ArrayProperty<double> dp("dp",d);
    TS_ASSERT( ! dp.value().compare("1.23,1.23,1.23,1.23") )
  
    std::vector<std::string> s(2,"yyy");
    ArrayProperty<std::string> sp("sp",s);
    TS_ASSERT( ! sp.value().compare("yyy,yyy") )
  }

  void testSetValueAndIsDefault()
  {
    std::string couldnt = "Could not set property ", cant = ". Can not convert \"";

    TS_ASSERT_EQUALS( iProp->setValue("1.1,2,2"),
      couldnt + iProp->name() + cant + "1.1,2,2\" to " + iProp->type() )
    TS_ASSERT( iProp->operator()().empty() )
    TS_ASSERT( iProp->isDefault() )
    TS_ASSERT_EQUALS( iProp->setValue("aaa,bbb"), 
      couldnt + iProp->name() + cant + "aaa,bbb\" to " + iProp->type() )
    TS_ASSERT( iProp->operator()().empty() )
    TS_ASSERT( iProp->isDefault() )
    TS_ASSERT_EQUALS( iProp->setValue("1,2,3,4"), "" )
    TS_ASSERT_EQUALS( iProp->operator()().size(), 4 )
    for ( std::size_t i=0; i < 4; ++i )
    {
      TS_ASSERT_EQUALS( iProp->operator()()[i], i+1 )
    }
    TS_ASSERT( !iProp->isDefault() )
    TS_ASSERT_EQUALS( iProp->setValue(""), "" )
    TS_ASSERT( iProp->operator()().empty() )
    TS_ASSERT( iProp->isDefault() )
    
    TS_ASSERT_EQUALS( dProp->setValue("aaa,bbb"),
      couldnt + dProp->name() + cant + "aaa,bbb\" to " + dProp->type() )
    TS_ASSERT( dProp->operator()().empty() )
    TS_ASSERT( dProp->isDefault() )
    TS_ASSERT_EQUALS( dProp->setValue("1,2"), "" )
    TS_ASSERT_EQUALS( dProp->operator()()[1], 2 )
    TS_ASSERT( !dProp->isDefault() )
    TS_ASSERT_EQUALS( dProp->setValue("1.11,2.22,3.33,4.44"), "" )
    TS_ASSERT_EQUALS( dProp->operator()()[0], 1.11 )
    TS_ASSERT( !dProp->isDefault() )
    TS_ASSERT_EQUALS( dProp->setValue(""), "" )
    TS_ASSERT( dProp->operator()().empty() )
    TS_ASSERT( dProp->isDefault() )
    
    TS_ASSERT_EQUALS( sProp->setValue("This,is,a,test"), "" )
    TS_ASSERT_EQUALS( sProp->operator()()[2], "a" )
    TS_ASSERT( !sProp->isDefault() )
    TS_ASSERT_EQUALS( sProp->setValue(""), "" )
    TS_ASSERT( sProp->operator()().empty() )
    TS_ASSERT( sProp->isDefault() )    
  }

  void testAssignmentOperator()
  {
    ArrayProperty<int> i("i");
    TS_ASSERT( i.isDefault() )
    std::vector<int> ii(3,4);
    TS_ASSERT_EQUALS( i = ii, ii )
    TS_ASSERT_EQUALS( i.operator()()[1], 4 )
    TS_ASSERT( !i.isDefault() )

    ArrayProperty<double> d("d");
    TS_ASSERT( d.isDefault() )
    std::vector<double> dd(5,9.99);
    TS_ASSERT_EQUALS( d = dd, dd )
    TS_ASSERT_EQUALS( d.operator()()[3], 9.99 )
    TS_ASSERT( !d.isDefault() )

    ArrayProperty<std::string> s("s");
    TS_ASSERT( s.isDefault() )
    std::vector<std::string> ss(2,"zzz");
    TS_ASSERT_EQUALS( s = ss, ss )
    TS_ASSERT_EQUALS( s.operator()()[0], "zzz" )
    TS_ASSERT( !s.isDefault() )
  }
  
  void testOperatorBrackets()
  {
    TS_ASSERT( iProp->operator()().empty() )
    TS_ASSERT( dProp->operator()().empty() )
    TS_ASSERT( sProp->operator()().empty() )    
  }
  
  void testOperatorNothing()
  {
    std::vector<int> i = *iProp;
    TS_ASSERT( i.empty() )
    
    std::vector<double> d(3,8.8);
    *dProp = d;
    std::vector<double> dd = *dProp;
    for(std::size_t i = 0; i<3; ++i)
    {
      TS_ASSERT_EQUALS( dProp->operator()()[i], 8.8 )
    }
    
    std::vector<std::string> s = *sProp;
    TS_ASSERT( s.empty() )
  }
  
  void testCasting()
  {
    TS_ASSERT_DIFFERS( dynamic_cast<PropertyWithValue<std::vector<int> >*>(iProp), 
                            static_cast<PropertyWithValue<std::vector<int> >*>(0) )
    TS_ASSERT_DIFFERS( dynamic_cast<PropertyWithValue<std::vector<double> >*>(dProp), 
                            static_cast<PropertyWithValue<std::vector<double> >*>(0) )
    TS_ASSERT_DIFFERS( dynamic_cast<PropertyWithValue<std::vector<std::string> >*>(sProp), 
                            static_cast<PropertyWithValue<std::vector<std::string> >*>(0) )
    
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) )
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) )
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) )
  }
  
private:
  ArrayProperty<int> *iProp;
  ArrayProperty<double> *dProp;
  ArrayProperty<std::string> *sProp;
};

#endif /*ARRAYPROPERTYTEST_H_*/
