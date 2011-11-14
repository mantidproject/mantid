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

  void testAdding_A_Parameter_That_Is_Not_Present_Puts_The_Parameter_In()
  {
    // Add a parameter for the first component of the instrument
    IComponent_sptr comp = m_testInstrument->getChild(0);
    const std::string name("TestName");
    const double value(5.1);
    ParameterMap pmap;
    TS_ASSERT_EQUALS(pmap.size(), 0);
    pmap.addDouble(comp.get(), name, value);
    TS_ASSERT_EQUALS(pmap.size(), 1);
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
    const std::string type("int");
    const int value(1);
    pmap.add<int>("int", m_testInstrument.get(),name,value);
    TS_ASSERT_EQUALS(pmap.contains(m_testInstrument.get(), name, "int"), true);
    TS_ASSERT_EQUALS(pmap.contains(m_testInstrument.get(), name, "double"), false);
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
  }

private:
  Instrument_sptr m_testInstrument;
};


#endif /* PARAMETERMAPTEST_H_ */
