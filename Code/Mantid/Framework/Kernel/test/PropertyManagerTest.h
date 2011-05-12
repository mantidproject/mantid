#ifndef PROPERTYMANAGERTEST_H_
#define PROPERTYMANAGERTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Kernel;

class PropertyManagerHelper : public PropertyManager
{
public:
  PropertyManagerHelper() : PropertyManager() {}

  using PropertyManager::declareProperty;
  using PropertyManager::setProperty;
  using PropertyManager::getPointerToProperty;
};

class PropertyManagerTest : public CxxTest::TestSuite
{
public:
  void setUp()
  {
    manager = new PropertyManagerHelper;
    Property *p = new PropertyWithValue<int>("aProp",1);
    manager->declareProperty(p);
    manager->declareProperty("anotherProp",1.11);
    manager->declareProperty("yetAnotherProp","itsValue");
  }

  void tearDown()
  {
    delete manager;
  }

  void testConstructor()
  {
    PropertyManagerHelper mgr;
    std::vector<Property*> props = mgr.getProperties();
    TS_ASSERT( props.empty() );
  }
  
  void testCopyConstructor()
  {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp",10);
    PropertyManagerHelper mgr2 = mgr1;
    const std::vector<Property*>& props1 = mgr1.getProperties();
    const std::vector<Property*>& props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS( props1.size(), props2.size() );
    TS_ASSERT_DIFFERS( &props1[0], &props2[0] );
    TS_ASSERT_EQUALS( props1[0]->name(), props2[0]->name() );
    TS_ASSERT_EQUALS( props1[0]->value(), props2[0]->value() );
  }
  
  void testCopyAssignment()
  {
    PropertyManagerHelper mgr1;
    mgr1.declareProperty("aProp",10);
    PropertyManagerHelper mgr2;
    mgr2 = mgr1;
    const std::vector<Property*>& props1 = mgr1.getProperties();
    const std::vector<Property*>& props2 = mgr2.getProperties();
    TS_ASSERT_EQUALS( props1.size(), props2.size() );
    TS_ASSERT_DIFFERS( &props1[0], &props2[0] );
    TS_ASSERT_EQUALS( props1[0]->name(), props2[0]->name() );
    TS_ASSERT_EQUALS( props1[0]->value(), props2[0]->value() );
  }

  void testdeclareProperty_pointer()
  {
    PropertyManagerHelper mgr;
    Property *p = new PropertyWithValue<double>("myProp", 9.99);
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty(p) );
    TS_ASSERT( mgr.existsProperty(p->name()) );
    // Confirm that the first 4 characters of the string are the same
    
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    
    TS_ASSERT_EQUALS( mgr.getPropertyValue("myProp").substr(0,4),  std::string("9.99") );
      
    TS_ASSERT_THROWS( mgr.declareProperty(p), Exception::ExistsError );
    TS_ASSERT_THROWS( mgr.declareProperty(new PropertyWithValue<int>("",0)), std::invalid_argument );
    mgr.declareProperty(new PropertyWithValue<int>("GoodIntProp",1), "Test doc"); 
    TS_ASSERT_EQUALS( mgr.getPointerToProperty("GoodIntProp")->documentation(), "Test doc" );
  }

  void testdeclareProperty_int()
  {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", 1) );
    TS_ASSERT( ! mgr.getPropertyValue("myProp").compare("1") );
    TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", 5), Exception::ExistsError );
    TS_ASSERT_THROWS( mgr.declareProperty("", 5), std::invalid_argument );
  }

  void testdeclareProperty_double()
  {
    PropertyManagerHelper mgr;
    BoundedValidator<double> *v = new BoundedValidator<double>(1,5);
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", 9.99, v) );
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.
    TS_ASSERT_EQUALS( mgr.getPropertyValue("myProp").substr(0,4), std::string("9.99") );
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("withDoc", 4.4, v->clone(), "Test doc doub") );
    TS_ASSERT_EQUALS( mgr.getPointerToProperty("withDoc")->documentation(), "Test doc doub" );
    TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", 5.5), Exception::ExistsError );
    TS_ASSERT_THROWS( mgr.declareProperty("", 5.5), std::invalid_argument );
  }

  void testdeclareProperty_string()
  {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("myProp", "theValue", new MandatoryValidator<std::string>, "hello") );
    TS_ASSERT_EQUALS( mgr.getPropertyValue("myProp"), "theValue" );
      Property *p;
    TS_ASSERT_THROWS_NOTHING( p = mgr.getProperty("myProp") );
    TS_ASSERT_EQUALS(p->documentation(),"hello");

    TS_ASSERT_THROWS( mgr.declareProperty("MYPROP", "aValue"), Exception::ExistsError );
    TS_ASSERT_THROWS( mgr.declareProperty("", "aValue"), std::invalid_argument );
  }

  void testSetProperties()
  {
    PropertyManagerHelper mgr;
    mgr.declareProperty("APROP", 1);
    mgr.declareProperty("anotherProp", 1.0);
    TS_ASSERT_THROWS_NOTHING( mgr.setProperties("APROP=15;anotherProp=1.3") );
    TS_ASSERT( ! mgr.getPropertyValue("APROP").compare("15") );
    TS_ASSERT( ! mgr.getPropertyValue("anotherProp").compare("1.3") );
  }

  void testSetPropertyValue()
  {
    manager->setPropertyValue("APROP","10");
    TS_ASSERT( ! manager->getPropertyValue("aProp").compare("10") );
    manager->setPropertyValue("aProp","1");
    TS_ASSERT_THROWS( manager->setPropertyValue("fhfjsdf","0"), Exception::NotFoundError );
  }

  void testSetProperty()
  {
    TS_ASSERT_THROWS_NOTHING( manager->setProperty("AProp",5) );
    TS_ASSERT_THROWS( manager->setProperty("wefhui",5), Exception::NotFoundError );
    TS_ASSERT_THROWS( manager->setProperty("APROP",5.55), std::invalid_argument );
    TS_ASSERT_THROWS( manager->setProperty("APROP","value"), std::invalid_argument );
    TS_ASSERT_THROWS_NOTHING( manager->setProperty("AProp",1) );
  }

  void testExistsProperty()
  {
    Property *p = new PropertyWithValue<int>("sjfudh",0);
    TS_ASSERT( ! manager->existsProperty(p->name()) );
    Property *pp = new PropertyWithValue<double>("APROP",9.99);
    // Note that although the name of the property is the same, the type is different - yet it passes
    TS_ASSERT( manager->existsProperty(pp->name()) );
    delete p;
    delete pp;
  }

  void testValidateProperties()
  {
    TS_ASSERT( manager->validateProperties() );
    PropertyManagerHelper mgr;
    mgr.declareProperty("someProp","", new MandatoryValidator<std::string>);
    TS_ASSERT( ! mgr.validateProperties() );
  }

  void testPropertyCount()
  {
    PropertyManagerHelper mgr;
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);
    const std::string name("TestProperty");
    mgr.declareProperty(name, 10.0);
    TS_ASSERT_EQUALS(mgr.propertyCount(), 1);
    mgr.removeProperty(name);
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);
  }

  void testGetPropertyValue()
  {
    TS_ASSERT( ! manager->getPropertyValue("APROP").compare("1") );
    TS_ASSERT_THROWS( manager->getPropertyValue("sdfshdu"), Exception::NotFoundError );
  }

  void testGetProperty()
  {
    Property *p = manager->getProperty("APROP");
    TS_ASSERT( p );
    TS_ASSERT( ! p->name().compare("aProp") );
    TS_ASSERT( ! p->value().compare("1") );
    TS_ASSERT( ! p->documentation().compare("") );
    TS_ASSERT( typeid(int) == *p->type_info() );

    TS_ASSERT_THROWS( p = manager->getProperty("werhui"), Exception::NotFoundError );

    int i;
    TS_ASSERT_THROWS_NOTHING( i = manager->getProperty("aprop") );
    TS_ASSERT_EQUALS( i, 1 );
    double dd;
    TS_ASSERT_THROWS( dd= manager->getProperty("aprop"), std::runtime_error );
    std::string s = manager->getProperty("aprop");
    TS_ASSERT( ! s.compare("1") );
    double d;
    TS_ASSERT_THROWS_NOTHING( d = manager->getProperty("anotherProp") );
    TS_ASSERT_EQUALS( d, 1.11 );
    int ii;
    TS_ASSERT_THROWS( ii = manager->getProperty("anotherprop"), std::runtime_error );
    std::string ss = manager->getProperty("anotherprop");
    // Note that some versions of boost::lexical_cast > 1.34 give a string such as
    // 9.9900000000000002 rather than 9.99. Converting back to a double however does
    // still give the correct 9.99.

    TS_ASSERT_EQUALS( ss.substr(0,4), std::string("1.11") );

    // This works, but CANNOT at present declare the string on a separate line and then assign
    //               (as I did for the int & double above)
    std::string sss = manager->getProperty("yetanotherprop");
    TS_ASSERT( ! sss.compare("itsValue") );
  }

  void testGetProperties()
  {
    std::vector<Property*> props = manager->getProperties();
    TS_ASSERT( props.size() == 3 );
    Property *p = props[0];
    TS_ASSERT( ! p->name().compare("aProp") );
    TS_ASSERT( ! p->value().compare("1") );
  }

  void testLongLongProperty()
  {
    PropertyManagerHelper mgr;
    TS_ASSERT_THROWS_NOTHING( mgr.declareProperty("llprop",static_cast<int64_t>(0)) );
    TS_ASSERT_THROWS_NOTHING( mgr.setProperty("llprop",static_cast<int64_t>(52147900000)) );
    TS_ASSERT_EQUALS( mgr.getPropertyValue("llprop"), "52147900000" );
    TS_ASSERT_THROWS_NOTHING( mgr.setPropertyValue("llprop","1234567890123456789") );
    int64_t retrieved;
    TS_ASSERT_THROWS_NOTHING( retrieved = mgr.getProperty("llprop") );
    TS_ASSERT_EQUALS( retrieved, static_cast<int64_t>(1234567890123456789) );
  }

  void testRemoveProperty()
  {
    PropertyManagerHelper mgr;
    const std::string name("TestProperty");
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name, 10.0));
    TS_ASSERT_THROWS_NOTHING(mgr.removeProperty(name));
    TS_ASSERT_EQUALS(mgr.getProperties().size(), 0 );
    
  }

  void testClear()
  {
    PropertyManagerHelper mgr;
    const std::string name("TestProperty");
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "1", 10.0));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "2", 15.0));
    TS_ASSERT_THROWS_NOTHING(mgr.declareProperty(name + "3", 14.0));

    TS_ASSERT_EQUALS(mgr.propertyCount(), 3);
    TS_ASSERT_THROWS_NOTHING(mgr.clear());
    TS_ASSERT_EQUALS(mgr.propertyCount(), 0);

  }

  //-----------------------------------------------------------------------------------------------------------
  /** Test of adding managers together (this will be used when
   * concatenating runs together).
   */
  void testAdditionOperator()
  {
    PropertyManager mgr1;
    Property *p;
    p = new PropertyWithValue<double>("double", 12.0);    mgr1.declareProperty(p, "docs");
    p = new PropertyWithValue<int>("int", 23);            mgr1.declareProperty(p, "docs");
    p = new PropertyWithValue<double>("double_only_in_mgr1", 456.0);  mgr1.declareProperty(p, "docs");

    PropertyManager mgr2;
    p = new PropertyWithValue<double>("double", 23.6);      mgr2.declareProperty(p, "docs");
    p = new PropertyWithValue<int>("int", 34);              mgr2.declareProperty(p, "docs");
    p = new PropertyWithValue<double>("new_double_in_mgr2", 321.0);  mgr2.declareProperty(p, "docs");
    p = new PropertyWithValue<int>("new_int", 655);              mgr2.declareProperty(p, "docs");

    //Add em together
    mgr1 += mgr2;

    double d;
    d = mgr1.getProperty("double");
    TS_ASSERT_DELTA( d, 35.6, 1e-4);
    d = mgr1.getProperty("double_only_in_mgr1");
    TS_ASSERT_DELTA( d, 456.0, 1e-4);
    d = mgr1.getProperty("new_double_in_mgr2");
    TS_ASSERT_DELTA( d, 321.0, 1e-4);

    int i;
    i = mgr1.getProperty("int");
    TS_ASSERT_EQUALS( i, 57);
    i = mgr1.getProperty("new_int");
    TS_ASSERT_EQUALS( i, 655);

  }



private:
  PropertyManagerHelper * manager;

};

#endif /*PROPERTYMANAGERTEST_H_*/
