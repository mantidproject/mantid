#ifndef TIMESERIESPROPERTYTEST_H_
#define TIMESERIESPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>
#include <ctime>
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
  
  ~TimeSeriesPropertyTest()
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
    TS_ASSERT( typeid( std::multimap<std::time_t, int> ) == *iProp->type_info()  )
    //TS_ASSERT( iProp->isDefault() )
    
    TS_ASSERT( ! dProp->name().compare("doubleProp") )
    TS_ASSERT( ! dProp->documentation().compare("") )
    TS_ASSERT( typeid( std::multimap<std::time_t, double> ) == *dProp->type_info()  )
    ///TS_ASSERT( dProp->isDefault() )
    
    TS_ASSERT( ! sProp->name().compare("stringProp") )
    TS_ASSERT( ! sProp->documentation().compare("") )
    TS_ASSERT( typeid( std::multimap<std::time_t, std::string> ) == *sProp->type_info()  )
    //TS_ASSERT( sProp->isDefault() )
	}
  
	void testSetValue()
	{
    TS_ASSERT_THROWS( iProp->setValue("1"), Exception::NotImplementedError )
    TS_ASSERT_THROWS( dProp->setValue("5.5"), Exception::NotImplementedError )
    TS_ASSERT_THROWS( sProp->setValue("aValue"), Exception::NotImplementedError )
	}

	void testAddValue()
	{
    TS_ASSERT( iProp->addValue("2007-11-30T16:17:00",1) )
    TS_ASSERT( iProp->addValue("2007-11-30T16:17:10",1) )
    //TS_ASSERT( ! iProp->addValue("2007-11-30T16:17:10",2) )
    //TS_ASSERT( ! iProp->addValue("NotaTime",3) )
    
    TS_ASSERT( dProp->addValue("2007-11-30T16:17:00",9.99) )
    TS_ASSERT( dProp->addValue("2007-11-30T16:17:10",5.55) )
    //TS_ASSERT( ! dProp->addValue("2007-11-30T16:17:10",8.88) )
    
    TS_ASSERT( sProp->addValue("2007-11-30T16:17:00","test") )
    TS_ASSERT( sProp->addValue("2007-11-30T16:17:10","test2") )
    //TS_ASSERT( ! sProp->addValue("2007-11-30T16:17:10","test3") )
 	}

	void testValue()
	{
    const std::string dString = dProp->value();
    TS_ASSERT_EQUALS( dString.substr(0,27), "2007-Nov-30 16:17:00  9.99\n" );
    const std::string iString = iProp->value();
    TS_ASSERT_EQUALS( iString.substr(0,24), "2007-Nov-30 16:17:00  1\n" );
    const std::string sString = sProp->value();
    TS_ASSERT_EQUALS( sString.substr(0,27), "2007-Nov-30 16:17:00  test\n" );
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
