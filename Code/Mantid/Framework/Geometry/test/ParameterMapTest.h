#ifndef PARAMETERMAPTEST_H_
#define PARAMETERMAPTEST_H_

#include "MantidGeometry/Instrument/Parameter.h"
#include "MantidGeometry/Instrument/ParameterMap.h"
#include "MantidGeometry/Instrument/Detector.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

#include <boost/function.hpp>
#include <boost/make_shared.hpp>

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
    ParameterMap pmapG;
    // Empty
    TS_ASSERT_EQUALS(pmapA,pmapB);
    
    pmapA.addDouble(m_testInstrument.get(), name, value);
    // Map equals itself
    TS_ASSERT_EQUALS(pmapA,pmapA);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapB);
    TS_ASSERT_DIFFERS(pmapA,pmapG);

    auto par= pmapA.getRecursive(m_testInstrument.get(),name);
    pmapG.add(m_testInstrument.get(),par);
    TS_ASSERT_EQUALS(pmapA,pmapG);

    // Same name/value/component
    pmapB.addDouble(m_testInstrument.get(), name, value);
    // Now equal
    TS_ASSERT_EQUALS(pmapA,pmapB);
    TS_ASSERT_EQUALS(pmapA,pmapG);

    //---  C
    ParameterMap pmapC,pmapC1;
    // Same name/value different component
    IComponent_sptr comp = m_testInstrument->getChild(0);
    pmapC.addDouble(comp.get(), name, value);
    auto par1=pmapC.getRecursive(comp.get(),name);
    pmapC1.add(comp.get(), par1);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapC);
    // Equal
    TS_ASSERT_EQUALS(pmapC,pmapC1);


    //---  D
    // Same name/component different value
    ParameterMap pmapD;
    pmapD.addDouble(m_testInstrument.get(), name, value + 1.0);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapD);
    // Adding the same replaces the same par
    pmapD.add(m_testInstrument.get(), par1);
    // Equal
    TS_ASSERT_EQUALS(pmapA,pmapD);


    //---  E
    // Same value/component different name
    ParameterMap pmapE;
    pmapE.addDouble(m_testInstrument.get(), name + "_differ", value);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapE);

    //---  F
    //Different type
    ParameterMap pmapF;
    pmapF.addInt(m_testInstrument.get(), name, 5);
    // Differs from other
    TS_ASSERT_DIFFERS(pmapA,pmapF);
    // Adding the same replaces the same par regardless of type
    pmapF.add(m_testInstrument.get(), par1);
    // Equal
    TS_ASSERT_EQUALS(pmapA,pmapF);

  }

  void test_Diff_Method()
  {
    const std::string name("TestName");
    const double value(5.1);
   
    ParameterMap pmapA;
    ParameterMap pmapB;
    // Empty
    TS_ASSERT_EQUALS(pmapA.diff(pmapB), "");
    TS_ASSERT_EQUALS(pmapA.diff(pmapA), "");
    
    pmapA.addDouble(m_testInstrument.get(), name, value);
    TS_ASSERT_DIFFERS(pmapA.diff(pmapB), "");

    pmapB.addDouble(m_testInstrument.get(), name, value);
    TS_ASSERT_EQUALS(pmapA.diff(pmapB), "");

    pmapA.addDouble(m_testInstrument.get(), name, value);
    pmapB.addDouble(m_testInstrument.get(), name, 5.2);
    pmapA.addDouble(m_testInstrument.get(), name+"2", value);
    pmapB.addDouble(m_testInstrument.get(), name+"2", 5.3);
    TS_ASSERT_DIFFERS(pmapA.diff(pmapB), "");
    TS_ASSERT_DIFFERS(pmapA.diff(pmapB), "");
  }

  void testClone()
  {
    const double value(5.1);
   
    ParameterMap pmapA,pmapB;
   
    pmapA.addDouble(m_testInstrument.get(), "testDouble", value);
    pmapA.addV3D(m_testInstrument.get(), "testV3D", Mantid::Kernel::V3D(1,2,3));

    auto parD= pmapA.getRecursive(m_testInstrument.get(),"testDouble");
    auto parV3= pmapA.getRecursive(m_testInstrument.get(),"testV3D");

    Mantid::Geometry::Parameter *pParD = parD->clone();
    Mantid::Geometry::Parameter *pParV = parV3->clone();

    TS_ASSERT_EQUALS(pParD->asString(),parD->asString())
    TS_ASSERT_EQUALS(pParV->asString(),parV3->asString())

    pmapB.add(m_testInstrument.get(),Parameter_sptr(pParD));
    pmapB.add(m_testInstrument.get(),Parameter_sptr(pParV));

    TS_ASSERT_EQUALS(pmapA,pmapB);
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

  void test_Replacing_Existing_Parameter_On_A_Copy_Does_Not_Update_Original_Value_Using_Generic_Add()
  {
    using namespace Mantid::Kernel;

    // -- General templated function --
    doCopyAndUpdateTestUsingGenericAdd<double>("double", 5.0, 3.5); // no need to check other types
  }

  void test_Replacing_Existing_Parameter_On_A_Copy_Does_Not_Update_Original_Value_Using_AddHelpers()
  {
    using namespace Mantid::Kernel;
    // -- Specialized Helper Functions --

    // double
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,double, const std::string *const)> faddDouble;
    faddDouble = (void (ParameterMap::*)(const IComponent*,const std::string&,double, const std::string *const ))&ParameterMap::addDouble;
    doCopyAndUpdateTestUsingAddHelpers(faddDouble, "name", 5.0, 4.0);

    // int
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,int, const std::string *const)> faddInt;
    faddInt = (void (ParameterMap::*)(const IComponent*,const std::string&,int, const std::string *const))&ParameterMap::addInt;
    doCopyAndUpdateTestUsingAddHelpers(faddInt, "name", 3, 5);

    // bool
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,bool, const std::string *const)> faddBool;
    faddBool = (void (ParameterMap::*)(const IComponent*,const std::string&,bool, const std::string  *const))&ParameterMap::addBool;
    doCopyAndUpdateTestUsingAddHelpers(faddBool, "name", true, false);

    // string
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,const std::string&, const std::string *const)> faddStr;
    faddStr = (void (ParameterMap::*)(const IComponent*,const std::string&,const std::string&, const std::string *const))&ParameterMap::addString;
    doCopyAndUpdateTestUsingAddHelpers(faddStr, "name", std::string("first"), std::string("second"));

    // V3D
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,const V3D&, const std::string *const )> faddV3D;
    faddV3D = (void (ParameterMap::*)(const IComponent*,const std::string&,const V3D&, const std::string* const ))&ParameterMap::addV3D;
    doCopyAndUpdateTestUsingAddHelpers(faddV3D, "name", V3D(1,2,3), V3D(4,5,6));

    // Quat
    boost::function<void (ParameterMap*, const IComponent*,const std::string&,const Quat&, const std::string *const )> faddQuat;
    faddQuat = (void (ParameterMap::*)(const IComponent*,const std::string&,const Quat&, const std::string *const))&ParameterMap::addQuat;
    doCopyAndUpdateTestUsingAddHelpers(faddQuat, "name", Quat(), Quat(45.0,V3D(0,0,1)));
  }

  void test_Replacing_Existing_Parameter_On_A_Copy_Does_Not_Update_Original_Value_Using_AddHelpers_As_Strings()
  {
    // -- Specialized Helper Functions --

    typedef boost::function<void (ParameterMap*, const IComponent*,const std::string&,const std::string &,const std::string *const)> AddFuncHelper;

    // double
    AddFuncHelper faddDouble;
    faddDouble = (void (ParameterMap::*)(const IComponent*,const std::string&,const std::string &, const std::string *const))&ParameterMap::addDouble;
    doCopyAndUpdateTestUsingAddHelpersAsStrings<AddFuncHelper, double>(faddDouble, "name", 5.0, 4.0);

    // int
    AddFuncHelper faddInt;
    faddInt = (void (ParameterMap::*)(const IComponent*,const std::string&,const std::string &, const std::string *const))&ParameterMap::addInt;
    doCopyAndUpdateTestUsingAddHelpersAsStrings<AddFuncHelper, int>(faddInt, "name", 3, 5);

    // bool
    AddFuncHelper faddBool;
    faddBool = (void (ParameterMap::*)(const IComponent*,const std::string&,const std::string&, const std::string *const))&ParameterMap::addBool;
    doCopyAndUpdateTestUsingAddHelpersAsStrings<AddFuncHelper, bool>(faddBool, "name", true, false);
  }

  void test_Replacing_Existing_Parameter_On_A_Copy_Does_Not_Update_Original_Value_Using_AddPosition_Helper()
  {
    using namespace Mantid::Kernel;

    ParameterMap pmap;
    V3D origValue(1,2,3);
    pmap.addV3D(m_testInstrument.get(), ParameterMap::pos(), origValue);

    ParameterMap copy(pmap); // invoke copy constructor

    TS_ASSERT_EQUALS(1, copy.size());
    auto parameter = copy.get(m_testInstrument.get(), ParameterMap::pos());
    TS_ASSERT_EQUALS(origValue, parameter->value<V3D>());
    //change the value on the copy and it should NOT update on the original
    copy.addPositionCoordinate(m_testInstrument.get(), ParameterMap::posy() , 5.0);

    V3D newValue(1,5,3);
    auto copyParameter = copy.get(m_testInstrument.get(), ParameterMap::pos());
    TS_ASSERT_EQUALS(newValue, copyParameter->value<V3D>());
    auto origParameter = pmap.get(m_testInstrument.get(), ParameterMap::pos());
    TS_ASSERT_EQUALS(origValue, origParameter->value<V3D>());
  }

  void test_Replacing_Existing_Parameter_On_A_Copy_Does_Not_Update_Original_Value_Using_AddRotation_Helper()
  {
    using namespace Mantid::Kernel;

    ParameterMap pmap;
    Quat origValue(45.0, V3D(0,0,1));
    pmap.addQuat(m_testInstrument.get(), ParameterMap::rot(), origValue);

    ParameterMap copy(pmap); // invoke copy constructor

    TS_ASSERT_EQUALS(1, copy.size());
    auto parameter = copy.get(m_testInstrument.get(), ParameterMap::rot());
    TS_ASSERT_EQUALS(origValue, parameter->value<Quat>());
    //change the value on the copy and it should NOT update on the original
    copy.addRotationParam(m_testInstrument.get(), ParameterMap::roty(), 30.0);

    Quat newValue = origValue;
    newValue.setAngleAxis(30.0, V3D(0,1,0));

    auto copyParameter = copy.get(m_testInstrument.get(), ParameterMap::rot());
    TS_ASSERT_EQUALS(newValue, copyParameter->value<Quat>());
    auto origParameter = pmap.get(m_testInstrument.get(), ParameterMap::rot());
    TS_ASSERT_EQUALS(origValue, origParameter->value<Quat>());
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
    // Attach 2 parameters to the instrument
    const std::string topLevel1("top1"), topLevel2("top2");
    const int value1(2), value2(3);
    ParameterMap pmap;
    pmap.addInt(m_testInstrument.get(), topLevel1, value1);
    pmap.addInt(m_testInstrument.get(), topLevel2, value2);
    //Ask for the parameter on a child
    IComponent_sptr comp = m_testInstrument->getChild(0);
    // Non-recursive should not find the parameter
    Parameter_sptr fetched = pmap.get(comp.get(), topLevel1);
    TS_ASSERT_EQUALS(fetched, Parameter_sptr());

    fetched = pmap.getRecursive(comp.get(), topLevel1);
    TS_ASSERT(fetched);
    TS_ASSERT_EQUALS(fetched->value<int>(), value1);

    // Check that the correct parameter name is found even after a first call that
    // would be cache the previous one
    fetched = pmap.getRecursive(comp.get(), topLevel2);
    TS_ASSERT(fetched);
    TS_ASSERT_EQUALS(fetched->value<int>(), value2);
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

  void testClearByName_Only_Removes_Named_Parameter_for_Cmpt()
  {
    ParameterMap pmap;
    pmap.addDouble(m_testInstrument.get(), "first", 5.4);
    pmap.addDouble(m_testInstrument.get(), "second", 10.3);
    IComponent_sptr comp = m_testInstrument->getChild(0);
    pmap.addDouble(comp.get(), "first", 5.4);
    TS_ASSERT_EQUALS(pmap.size(), 3);
    pmap.clearParametersByName("first",m_testInstrument.get());
    TS_ASSERT_EQUALS(pmap.size(), 2);
    // Has the correct one gone?
    Parameter_sptr stored = pmap.get(m_testInstrument.get(), "second");
    TSM_ASSERT("Parameter called second should still exist", stored);
    stored = pmap.get(comp.get(), "first");
    TSM_ASSERT("Parameter called first for child should still exist", stored);
    stored = pmap.get(m_testInstrument.get(), "first");
    TSM_ASSERT_EQUALS("Parameter called first for inst should not exist", stored, Parameter_sptr());
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

  void test_copy_from_old_pmap_to_new_pmap_with_new_component(){

    IComponent_sptr oldComp = m_testInstrument->getChild(0);
    IComponent_sptr newComp = m_testInstrument->getChild(1);

    ParameterMap oldPMap;
    oldPMap.addBool(oldComp.get(), "A", false);
    oldPMap.addDouble(oldComp.get(), "B", 1.2);

    ParameterMap newPMap;

    TS_ASSERT_DIFFERS(oldPMap,newPMap);

    newPMap.copyFromParameterMap(oldComp.get(),newComp.get(), &oldPMap);

    TS_ASSERT_EQUALS(newPMap.contains(newComp.get(), "A", ParameterMap::pBool()), true);
    TS_ASSERT_EQUALS(newPMap.contains(newComp.get(), "B", ParameterMap::pDouble()), true);

    Parameter_sptr a = newPMap.get(newComp.get(), "A");
    TS_ASSERT_EQUALS( a->value<bool>(), false);

    // change value on new and ensure it is not changed on the old
    newPMap.addBool(oldComp.get(), "A", true);
    a = newPMap.get(oldComp.get(), "A");
    TS_ASSERT_EQUALS( a->value<bool>(), true);
    auto oldA = oldPMap.get(oldComp.get(), "A");
    TS_ASSERT_EQUALS( oldA->value<bool>(), false);
  }

private:

  template<typename ValueType>
  void doCopyAndUpdateTestUsingGenericAdd(const std::string & type, const ValueType & origValue, const ValueType & newValue)
  {
    ParameterMap pmap;
    const std::string name = "Parameter";
    pmap.add<ValueType>(type, m_testInstrument.get(), name, origValue,NULL);

    ParameterMap copy(pmap); // invoke copy constructor

    TS_ASSERT_EQUALS(1, copy.size());
    auto parameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origValue, parameter->value<ValueType>());
    //change the value on the copy and it should NOT update on the original
    copy.add<ValueType>(type, m_testInstrument.get(), name, newValue);

    auto copyParameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(newValue, copyParameter->value<ValueType>());
    auto origParameter = pmap.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origValue, origParameter->value<ValueType>());
  }

  template<typename FuncType, typename ValueType>
  void doCopyAndUpdateTestUsingAddHelpers(const FuncType & addFunc,
                                          const std::string &name,
                                          const ValueType & origValue, const ValueType & newValue)
  {
    ParameterMap pmap;
    addFunc(&pmap, m_testInstrument.get(), name, origValue,NULL);

    ParameterMap copy(pmap); // invoke copy constructor

    TS_ASSERT_EQUALS(1, copy.size());
    auto parameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origValue, parameter->value<ValueType>());
    //change the value on the copy and it should NOT update on the original
    addFunc(&copy, m_testInstrument.get(), name, newValue,NULL);

    auto copyParameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(newValue, copyParameter->value<ValueType>());
    auto origParameter = pmap.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origValue, origParameter->value<ValueType>());
  }

  template<typename FuncType, typename ValueType>
  void doCopyAndUpdateTestUsingAddHelpersAsStrings(const FuncType & addFunc,
                                                   const std::string &name,
                                                   const ValueType & origTypedValue,
                                                   const ValueType & newTypedValue)
  {
    std::string origValue = boost::lexical_cast<std::string>(origTypedValue);
    std::string newValue = boost::lexical_cast<std::string>(newTypedValue);

    ParameterMap pmap;
    addFunc(&pmap, m_testInstrument.get(), name, origValue,NULL);

    ParameterMap copy(pmap); // invoke copy constructor

    TS_ASSERT_EQUALS(1, copy.size());
    auto parameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origTypedValue, parameter->value<ValueType>());
    //change the value on the copy and it should NOT update on the original
    addFunc(&copy, m_testInstrument.get(), name, newValue,NULL);

    auto copyParameter = copy.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(newTypedValue, copyParameter->value<ValueType>());
    auto origParameter = pmap.get(m_testInstrument.get(), name);
    TS_ASSERT_EQUALS(origTypedValue, origParameter->value<ValueType>());
  }

  // private instrument
  Instrument_sptr m_testInstrument;
};



//---------------------------------- Performance Tests ----------------------------------------
class ParameterMapTestPerformance : public CxxTest::TestSuite
{
public:

  static ParameterMapTestPerformance *createSuite() { return new ParameterMapTestPerformance(); }
  static void destroySuite( ParameterMapTestPerformance *suite ) { delete suite; }

  ParameterMapTestPerformance()
  {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    m_testInst = boost::make_shared<Instrument>(("basic"));

    // One object
    const double cylRadius(0.004), cylHeight(0.0002);
    Object_sptr pixelShape = \
        ComponentCreationHelper::createCappedCylinder(cylRadius, cylHeight, V3D(0.0,-cylHeight/2.0,0.0),
                                                      V3D(0.,1.0,0.), "pixel-shape");

    //Create a hierarchy
    // Inst
    //   -- topbank
    //     -- subbank_1
    //       -- subbank_2
    //        -- leaf

    //Make a new top bank
    CompAssembly *topbank = new CompAssembly("topbank");
    //Make a new subbank
    CompAssembly *subbank1 = new CompAssembly("subbank_1");
    //Make a new subbank
    CompAssembly *subbank2 = new CompAssembly("subbank_2");
    subbank1->add(subbank2);
    m_leaf = new Detector("pixel-00", 1, pixelShape, subbank2); // position irrelevant here
    subbank2->add(m_leaf);

    m_testInst->markAsDetector(m_leaf);
    m_testInst->add(subbank1);
    m_testInst->add(topbank);

    // Add a double parameter at the top level
    m_pmap.addDouble(m_testInst->getComponentID(), "instlevel",10.0);
    // and at leaf level
    m_pmap.addDouble(m_leaf->getComponentID(), "leaflevel",11.0);
  }

  void test_Inst_Par_Lookup_Via_GetRecursive_And_Leaf_Component()
  {
    // Look for the top level instrument parameter via a leaf component

    Mantid::Geometry::Parameter_sptr par_sptr;

    for(size_t i = 0; i < 10000; ++i)
    {
      par_sptr = m_pmap.getRecursive(m_leaf->getComponentID(), "instlevel");
    }
    // Use it to ensure the compiler doesn't optimise the loop away
    TS_ASSERT_DELTA(10.0, par_sptr->value<double>(),1e-12);
  }

  void test_Leaf_Par_Lookup_Via_GetRecursive_And_Leaf_Component()
  {
    Mantid::Geometry::Parameter_sptr par_sptr;

    for(size_t i = 0; i < 10000; ++i)
    {
      par_sptr = m_pmap.getRecursive(m_leaf->getComponentID(), "leaflevel");
    }
    // Use it to ensure the compiler doesn't optimise the loop away
    TS_ASSERT_DELTA(11.0, par_sptr->value<double>(),1e-12);
  }

  void test_Leaf_Par_Lookup_Via_Get_And_Leaf_Component()
  {
    Mantid::Geometry::Parameter_sptr par_sptr;

    for(size_t i = 0; i < 10000; ++i)
    {
      par_sptr = m_pmap.get(m_leaf->getComponentID(), "leaflevel");
    }
    // Use it to ensure the compiler doesn't optimise the loop away
    TS_ASSERT_DELTA(11.0, par_sptr->value<double>(),1e-12);
  }


private:
  Mantid::Geometry::Instrument_sptr m_testInst;
  Mantid::Geometry::ParameterMap m_pmap;
  Mantid::Geometry::IDetector *m_leaf;
};



#endif /* PARAMETERMAPTEST_H_ */
