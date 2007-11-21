#ifndef PROPERTYMANAGERTEST_H_
#define PROPERTYMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "../inc/PropertyManager.h"
#include "../inc/PropertyWithValue.h"
#include <iostream>

using namespace Mantid::Kernel;

class PropertyManagerTest : public CxxTest::TestSuite
{
public:
  PropertyManagerTest()
  {
    Property *p = new PropertyWithValue<int>("aProp",1);
    manager.declareProperty(p);
  }
  
	void testConstructor()
	{
	  PropertyManager mgr;
	  std::vector<Property*> props = mgr.getProperties();
	  TS_ASSERT( props.empty() )
	}

	void testdeclareProperty_pointer()
	{
		PropertyManager mgr;
		Property *p = new PropertyWithValue<double>("myProp", 9.99);
		TS_ASSERT_THROWS_NOTHING( mgr.declareProperty(p) )
		TS_ASSERT( mgr.checkProperty(p) )
		TS_ASSERT( ! mgr.getPropertyValue("myProp").compare("9.99") )
		
		TS_ASSERT_THROWS( mgr.declareProperty(p), Exception::ExistsError )
	}

	void testdeclareProperty_int()
	{
		PropertyManager mgr;
		TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", 1, "hello") )
		TS_ASSERT( ! mgr.getPropertyValue("myProp").compare("1") )
		Property *p = mgr.getProperty("myProp");
		TS_ASSERT( ! p->documentation().compare("hello") )
		
		TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", 5), Exception::ExistsError )
	}

	void testdeclareProperty_double()
	{
    PropertyManager mgr;
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", 9.99, "hello") )
    TS_ASSERT( ! mgr.getPropertyValue("myProp").compare("9.99") )
    Property *p = mgr.getProperty("myProp");
    TS_ASSERT( ! p->documentation().compare("hello") )
    
    TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", 5), Exception::ExistsError )
	}

	void testdeclareProperty_string()
	{
    PropertyManager mgr;
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", "theValue", "hello") )
    TS_ASSERT( ! mgr.getPropertyValue("myProp").compare("theValue") )
    Property *p = mgr.getProperty("myProp");
    TS_ASSERT( ! p->documentation().compare("hello") )
    
    TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", 5), Exception::ExistsError )
	}

	void testSetProperty()
	{
		manager.setProperty("APROP","10");
		TS_ASSERT( ! manager.getPropertyValue("aProp").compare("10") )
    manager.setProperty("aProp","1");
		TS_ASSERT_THROWS( manager.setProperty("fhfjsdf","0"), Exception::NotFoundError )
	}

	void testCheckProperty()
	{
	  Property *p = new PropertyWithValue<int>("sjfudh",0);
	  TS_ASSERT( ! manager.checkProperty(p) )
		Property *pp = new PropertyWithValue<double>("APROP",9.99);
    // Note that although the name of the property is the same, the type is different - yet it passes
		TS_ASSERT( manager.checkProperty(pp) )
	}

	void testGetPropertyValue()
	{
		TS_ASSERT( ! manager.getPropertyValue("APROP").compare("1") )
		TS_ASSERT_THROWS( manager.getPropertyValue("sdfshdu"), Exception::NotFoundError )
	}

	void testGetProperty()
	{
		Property *p = manager.getProperty("APROP");
		TS_ASSERT( p )
		TS_ASSERT( ! p->name().compare("aProp") )
		TS_ASSERT( ! p->value().compare("1") )
		TS_ASSERT( ! p->documentation().compare("") )
		TS_ASSERT( typeid(int) == *p->type_info() )
		
		TS_ASSERT_THROWS( manager.getProperty("werhui"), Exception::NotFoundError )

	}

	void testGetProperties()
	{
	  std::vector<Property*> props = manager.getProperties();
	  TS_ASSERT( props.size() == 1 )
	  Property *p = props[0];
    TS_ASSERT( ! p->name().compare("aProp") )
    TS_ASSERT( ! p->value().compare("1") )
	}

private:
  PropertyManager manager;
	
};

#endif /*PROPERTYMANAGERTEST_H_*/
