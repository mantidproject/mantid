#ifndef PARAMETERMAPTEST_H_
#define PARAMETERMAPTEST_H_

#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::ParameterMap;
using Mantid::Geometry::ParameterMap_sptr;
using Mantid::Geometry::Parameter_sptr;
using Mantid::Geometry::Instrument_sptr;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::IComponent_sptr;

class ParameterMapTest : public CxxTest::TestSuite
{
public:

  static ParameterMapTest *createSuite() { return new ParameterMapTest(); }
  static void destroySuite( ParameterMapTest *suite ) { delete suite; }

  ParameterMapTest()
  {
    m_testInstrument = ComponentCreationHelper::createTestInstrumentCylindrical(1);
  }

  void testConstructor_Does_Not_Throw()
  {
    TS_ASSERT_THROWS_NOTHING(ParameterMap());
  }

  void testParameter_Name_Functions()
  {
     // This should be the only test to explicitly use the string values
     // Other tests can use the functions tested here.
     TS_ASSERT_EQUALS(ParameterMap::pDouble(),"double");
     TS_ASSERT_EQUALS(ParameterMap::pInt(),"int");
     TS_ASSERT_EQUALS(ParameterMap::pBool(),"bool");
     TS_ASSERT_EQUALS(ParameterMap::pString(),"string");
     TS_ASSERT_EQUALS(ParameterMap::pV3D(),"V3D");
     TS_ASSERT_EQUALS(ParameterMap::pQuat(),"Quat");

     TS_ASSERT_EQUALS(ParameterMap::pos(),"pos");
     TS_ASSERT_EQUALS(ParameterMap::posx(),"x");
     TS_ASSERT_EQUALS(ParameterMap::posy(),"y");
     TS_ASSERT_EQUALS(ParameterMap::posz(),"z");

     TS_ASSERT_EQUALS(ParameterMap::rot(),"rot");
     TS_ASSERT_EQUALS(ParameterMap::rotx(),"rotx");
     TS_ASSERT_EQUALS(ParameterMap::roty(),"roty");
     TS_ASSERT_EQUALS(ParameterMap::rotz(),"rotz");
  }

  void test_Equality_Operator()
  {
    const std::string name("TestName");
    const double value(5.1);
   
    ParameterMap pmapA;
    ParameterMap pmapB;
    // Empty
    TS_ASSERT_EQUALS(pmapA,pmapB);
    
    pmapA.addDouble(m_testInstrument.get(), name, value);
    // Map equals itself
    TS_ASSERT_EQUALS(pmapA,pmapA);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapB);

    // Same name/value/component
    pmapB.addDouble(m_testInstrument.get(), name, value);
    // Now equal
    TS_ASSERT_EQUALS(pmapA,pmapB);

    ParameterMap pmapC;
    // Same name/value different component
    IComponent_sptr comp = m_testInstrument->getChild(0);
    pmapC.addDouble(comp.get(), name, value);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapC);

    // Same name/component different value
    ParameterMap pmapD;
    pmapD.addDouble(m_testInstrument.get(), name, value + 1.0);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapD);

    // Same value/component different name
    ParameterMap pmapE;
    pmapE.addDouble(m_testInstrument.get(), name + "_differ", value);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapE);

    //Different type
    ParameterMap pmapF;
    pmapF.addInt(m_testInstrument.get(), name, 5);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapF);
  }

  void testAdding_A_Parameter_That_Is_Not_Present_Puts_The_Parameter_In()
  {
    // Add a parameter for the first component of the instrument
    IComponent_sptr comp = m_testInstrument->getChild(0);
    const std::string name("TestName");
    const double value(5.1);
    ParameterMap pmap;
    TS_ASSERT_EQUALS(pmap.size(), 0);
    TSM_ASSERT("Newly created parameter map should be empty",pmap.empty())
    pmap.addDouble(comp.get(), name, value);
    TS_ASSERT_EQUALS(pmap.size(), 1);
    TSM_ASSERT("Populated parameter map should not be empty",!pmap.empty())
    // Check that the correct one went in
    Parameter_sptr fetchedValue = pmap.get(comp.get(), name);
    TS_ASSERT(fetchedValue);
    TS_ASSERT_DELTA(value, fetchedValue->value<double>(), DBL_EPSILON);
  }

  void testAdding_A_Parameter_Of_Same_Name_Overwrites_The_First()
  {
    // Add a parameter for the first component of the instrument
    IComponent_sptr comp = m_testInstrument->getChild(0);
    const std::string name("TestName");
    ParameterMap pmap;
    pmap.addDouble(comp.get(), name, 5.1);
    TS_ASSERT_EQUALS(pmap.size(), 1);
    const double finalValue(10.1);
    pmap.addDouble(comp.get(), name, finalValue);
    // Should have overwritten
    TS_ASSERT_EQUALS(pmap.size(), 1);
    Parameter_sptr stored = pmap.get(comp.get(), name);
    TS_ASSERT(stored);
    TS_ASSERT_DELTA(finalValue, stored->value<double>(), DBL_EPSILON);
  }

  void testMap_Contains_Newly_Added_Value_For_Correct_Component()
  {
    ParameterMap pmap;
    const std::string name("NewValue");
    pmap.addInt(m_testInstrument.get(), name, 1);
    TS_ASSERT_EQUALS(pmap.contains(m_testInstrument.get(), name), true);
    IComponent_sptr parametrized = m_testInstrument->getChild(0);
    TS_ASSERT_EQUALS(pmap.contains(parametrized.get(), name), false);
  }

  void testMap_Contains_Newly_Added_Value_For_Correct_Component_Of_Correct_Type()
  {
    ParameterMap pmap;
    const std::string name("MyValue");
    const std::string type(ParameterMap::pInt());
    const int value(1);
    pmap.add<int>(type, m_testInstrument.get(),name,value);
    TS_ASSERT_EQUALS(pmap.contains(m_testInstrument.get(), name, ParameterMap::pInt()), true);
    TS_ASSERT_EQUALS(pmap.contains(m_testInstrument.get(), name, ParameterMap::pDouble()), false);
  }

  void testMap_Contains_Parameter()
  {
    ParameterMap pmap;
    const std::string name("NewValue");
    pmap.addInt(m_testInstrument.get(), name, 1);
    auto param = pmap.get(m_testInstrument.get(), name);
    
    TS_ASSERT(pmap.contains(m_testInstrument.get(), *param));
    auto empty = Mantid::Geometry::ParameterFactory::create("int","testparam");
    TS_ASSERT(!pmap.contains(m_testInstrument.get(), *empty));
  }

  void testParameter_Name_Matching_Is_Case_Insensitive()
  {
    IComponent_sptr parametrized = m_testInstrument->getChild(0);
    const std::string camelCase("TestCase");
    const double value(10.01);
    ParameterMap pmap;
    pmap.addDouble(parametrized.get(), camelCase, value);
    Parameter_sptr fetched = pmap.get(parametrized.get(), "TESTCASE");
    TSM_ASSERT("The parameter should be found by a case insensitive search",fetched);
  }

  void testRecursive_Parameter_Search_Moves_Up_The_Instrument_Tree()
  {
    // Attach a parameter to the instrument
    const std::string topLevel("TopLevelParameter");
    const int value(2);
    ParameterMap pmap;
    pmap.addInt(m_testInstrument.get(), topLevel, value);
    //Ask for the parameter on a child
    IComponent_sptr comp = m_testInstrument->getChild(0);
    // Non-recursive should not find the parameter
    Parameter_sptr fetched = pmap.get(comp.get(), topLevel);
    TS_ASSERT_EQUALS(fetched, Parameter_sptr());
    fetched = pmap.getRecursive(comp.get(), topLevel);
    TS_ASSERT(fetched);
    TS_ASSERT_EQUALS(fetched->value<int>(), value);
  }

  void testClearByName_Only_Removes_Named_Parameter()
  {
    ParameterMap pmap;
    pmap.addDouble(m_testInstrument.get(), "first", 5.4);
    pmap.addDouble(m_testInstrument.get(), "second", 10.3);
    TS_ASSERT_EQUALS(pmap.size(), 2);
    pmap.clearParametersByName("first");
    TS_ASSERT_EQUALS(pmap.size(), 1);
    // Has the correct one gone?
    Parameter_sptr stored = pmap.get(m_testInstrument.get(), "second");
    TSM_ASSERT("Parameter called second should still exist", stored);
    stored = pmap.get(m_testInstrument.get(), "first");
    TSM_ASSERT_EQUALS("Parameter called first should not exist", stored, Parameter_sptr());
  }

  void testClear_Results_In_Empty_Map()
  {
    ParameterMap pmap;
    pmap.addInt(m_testInstrument.get(), "P1", 1);
    pmap.addInt(m_testInstrument.get(), "P2", 2);
    TS_ASSERT_EQUALS(pmap.size(), 2);
    pmap.clear();
    TS_ASSERT_EQUALS(pmap.size(), 0);
    TSM_ASSERT("Cleared parameter map should be empty",pmap.empty())
  }

    void test_lookup_via_type_returns_null_if_fails()
  {
    // Add a parameter for the first component of the instrument
    IComponent_sptr comp = m_testInstrument->getChild(0);
    // Create the parameter map with a single boolean type.
    ParameterMap pmap;
    TS_ASSERT_EQUALS(pmap.size(), 0);
    pmap.addBool(comp.get(), "A", true);
    TS_ASSERT_EQUALS(pmap.size(), 1);
    // Try to find double type parameters, of which there should be none.
    Parameter_sptr fetchedValue = pmap.getByType(comp.get(), ParameterMap::pDouble());
    TSM_ASSERT("Should not be able to find a double type parameter", fetchedValue == NULL);
  }

  void test_lookup_via_type()
  {
    // Add a parameter for the first component of the instrument
    IComponent_sptr comp = m_testInstrument->getChild(0);
    // Create the parameter map and add some new parameters.
    ParameterMap pmap;
    TS_ASSERT_EQUALS(pmap.size(), 0);
    pmap.addDouble(comp.get(), "A", 1.2);
    pmap.addBool(comp.get(), "B", true);
    TS_ASSERT_EQUALS(pmap.size(), 2);
   
    // Test the ability to correctly fetch the double argument by type.
    Parameter_sptr fetchedValue1 = pmap.getByType(comp.get(), ParameterMap::pDouble());
    TS_ASSERT(fetchedValue1);
    TS_ASSERT_EQUALS("A", fetchedValue1->name());
    TS_ASSERT_DELTA(1.2, fetchedValue1->value<double>(), DBL_EPSILON);

    // Test the ability to correctly fetch the bool argument by type.
    Parameter_sptr fetchedValue2 = pmap.getByType(comp.get(), ParameterMap::pBool());
    TS_ASSERT(fetchedValue2);
    TS_ASSERT_EQUALS("B", fetchedValue2->name());
    TS_ASSERT_EQUALS(true, fetchedValue2->value<bool>());
  }

    void test_lookup_recursive_by_type_finds_on_current()
  {
    IComponent_sptr component = m_testInstrument;

    //Add something to the parent component ONLY.
    ParameterMap pmap;
    pmap.addBool(component.get(), "A", true);

    //Find it via the component
    Parameter_sptr fetchedValue = pmap.getRecursiveByType(component.get(), ParameterMap::pBool());
    TS_ASSERT(fetchedValue != NULL);
    TS_ASSERT_EQUALS("A", fetchedValue->name());
    TS_ASSERT_EQUALS(ParameterMap::pBool(), fetchedValue->type());
    TS_ASSERT_EQUALS(true, fetchedValue->value<bool>());
  }

  void test_lookup_recursive_by_type_finds_on_parent_if_not_on_current()
  {
    IComponent_sptr childComponent = m_testInstrument->getChild(0);
    IComponent_sptr parentComponent = m_testInstrument;

    //Add something to the parent component ONLY.
    ParameterMap pmap;
    pmap.addBool(parentComponent.get(), "A", true);

    //Find it via the child 
    Parameter_sptr fetchedValue = pmap.getRecursiveByType(childComponent.get(), ParameterMap::pBool());
    TS_ASSERT(fetchedValue != NULL);
    TS_ASSERT_EQUALS("A", fetchedValue->name());
    TS_ASSERT_EQUALS(ParameterMap::pBool(), fetchedValue->type());
    TS_ASSERT_EQUALS(true, fetchedValue->value<bool>());
  }

  void test_lookup_recursive_by_type_finds_on_current_in_preference_to_parent()
  {
    IComponent_sptr childComponent = m_testInstrument->getChild(0);
    IComponent_sptr parentComponent = m_testInstrument;

    //Add something to the child component.
    ParameterMap pmap;
    pmap.addBool(childComponent.get(), "A", false);

    //Add something with the SAME TYPE TO THE PARENT TOO.
    pmap.addBool(parentComponent.get(), "B", true);

    //Find it via the child 
    Parameter_sptr fetchedValue = pmap.getRecursiveByType(childComponent.get(), ParameterMap::pBool());
    TS_ASSERT(fetchedValue != NULL);
    TSM_ASSERT_EQUALS("Has not searched through parameters with the correct priority", "A", fetchedValue->name());
    TSM_ASSERT_EQUALS("Has not searched through parameters with the correct priority",ParameterMap::pBool(), fetchedValue->type());
    TSM_ASSERT_EQUALS("Has not searched through parameters with the correct priority",false, fetchedValue->value<bool>());
  }

private:
  Instrument_sptr m_testInstrument;
};


#endif /* PARAMETERMAPTEST_H_ */
