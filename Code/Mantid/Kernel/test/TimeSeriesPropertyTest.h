#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;

class TimeSeriesPropertyTest : public CxxTest::TestSuite
{
public:
  TimeSeriesPropertyTest()
  {
    iProp = new TimeSeriesProperty<int>("intProp");
    dProp = new TimeSeriesProperty<double>("doubleProp");
    sProp = new TimeSeriesProperty<std::string>("stringProp");
  }
  
	void testConstructor()
	{
    // Test that all the base class member variables are correctly assigned to
    TS_ASSERT( ! iProp->name().compare("intProp") )
    TS_ASSERT( ! iProp->documentation().compare("") )
    TS_ASSERT( typeid( std::map<boost::posix_time::ptime, int> ) == *iProp->type_info()  )
    TS_ASSERT( iProp->isDefault() )
    
    TS_ASSERT( ! dProp->name().compare("doubleProp") )
    TS_ASSERT( ! dProp->documentation().compare("") )
    TS_ASSERT( typeid( std::map<boost::posix_time::ptime, double> ) == *dProp->type_info()  )
    TS_ASSERT( dProp->isDefault() )
    
    TS_ASSERT( ! sProp->name().compare("stringProp") )
    TS_ASSERT( ! sProp->documentation().compare("") )
    TS_ASSERT( typeid( std::map<boost::posix_time::ptime, std::string> ) == *sProp->type_info()  )
    TS_ASSERT( sProp->isDefault() )
	}

	void testValue()
	{
		TS_ASSERT_THROWS( iProp->value(), Exception::NotImplementedError )
    TS_ASSERT_THROWS( dProp->value(), Exception::NotImplementedError )
    TS_ASSERT_THROWS( sProp->value(), Exception::NotImplementedError )
	}

	void testSetValue()
	{
    TS_ASSERT_THROWS( iProp->setValue("1"), Exception::NotImplementedError )
    TS_ASSERT_THROWS( dProp->setValue("5.5"), Exception::NotImplementedError )
    TS_ASSERT_THROWS( sProp->setValue("aValue"), Exception::NotImplementedError )
	}

	void testAddValue()
	{
		TS_ASSERT( iProp->addValue("20071130T161700",1) )
    TS_ASSERT( iProp->addValue("20071130T161710",1) )
    TS_ASSERT( ! iProp->addValue("20071130T161710",2) )
    TS_ASSERT( ! iProp->addValue("NotaTime",3) )
    
    TS_ASSERT( dProp->addValue("20071130T161700",9.99) )
    TS_ASSERT( dProp->addValue("20071130T161710",5.55) )
    TS_ASSERT( ! dProp->addValue("20071130T161710",8.88) )
    
    TS_ASSERT( sProp->addValue("20071130T161700","test") )
    TS_ASSERT( sProp->addValue("20071130T161710","test2") )
    TS_ASSERT( ! sProp->addValue("20071130T161710","test3") )
 	}

	void testCasting()
	{
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(iProp), static_cast<Property*>(0) )
	  TS_ASSERT_DIFFERS( dynamic_cast<Property*>(dProp), static_cast<Property*>(0) )
    TS_ASSERT_DIFFERS( dynamic_cast<Property*>(sProp), static_cast<Property*>(0) )
	}
	
private:
  TimeSeriesProperty<int> *iProp;
  TimeSeriesProperty<double> *dProp;
  TimeSeriesProperty<std::string> *sProp;
};

#endif /*TIMESERIESPROPERTYTEST_H_*/
