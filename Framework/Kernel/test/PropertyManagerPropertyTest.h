// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyWithValue.hxx"

#include <boost/scoped_ptr.hpp>

using Mantid::Kernel::PropertyManagerProperty;

class PropertyManagerPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyManagerPropertyTest *createSuite() { return new PropertyManagerPropertyTest(); }
  static void destroySuite(PropertyManagerPropertyTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Constructor_Default_Direction_Is_Input() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test");
    // MSVC warns about comparing signed/unsigned here
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Input), pmap.direction());
  }

  void test_Constructor_Sets_Name_And_Direction() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test", Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output), pmap.direction());
  }

  void test_Constructor_Sets_Name_Direction_DefaultValue() {
    using Mantid::Kernel::Direction;

    auto testMgr = createPropMgrWithInt();
    PropertyManagerProperty pmap("Test", testMgr, Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output), pmap.direction());
    TS_ASSERT_EQUALS(testMgr, pmap());
    TS_ASSERT(pmap.isDefault());
  }

  void test_Clone_Gives_PropertyManagerProperty_Copy() {
    using Mantid::Kernel::Direction;
    using Mantid::Kernel::Property;

    auto testMgr = createPropMgrWithInt();
    PropertyManagerProperty pmap("Test", testMgr, Direction::Output);
    boost::scoped_ptr<PropertyManagerProperty> copy(pmap.clone());
    TS_ASSERT_EQUALS("Test", copy->name());
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output), copy->direction());
    TS_ASSERT_EQUALS(testMgr, (*copy)());
    TS_ASSERT(copy->isDefault());
  }

  void test_Assignment_Updates_Stored_Value() {
    using Mantid::Kernel::PropertyManager;

    PropertyManagerProperty pmap("Test");
    auto emptyMgr = pmap();
    auto testMgr = createPropMgrWithInt();
    pmap = testMgr;
    auto retrieved = pmap();
    TS_ASSERT_EQUALS(retrieved, testMgr);
    TS_ASSERT_DIFFERS(retrieved, emptyMgr);
  }

  void test_Property_Can_Implicitly_Convert_To_Value_Type() {
    using Mantid::Kernel::PropertyManager;
    using Mantid::Kernel::PropertyManager_sptr;
    // Create a top-level PropertyManager, add a PropertyManagerProperty
    // as a nested PropertyManager
    auto topMgr = std::make_shared<PropertyManager>();
    topMgr->declareProperty(std::make_unique<PropertyManagerProperty>("Args"));
    topMgr->setProperty("Args", createPropMgrWithInt());

    PropertyManager_sptr args;
    TS_ASSERT_THROWS_NOTHING(args = topMgr->getProperty("Args"));
    TS_ASSERT(args);
  }

  void test_Property_Set_With_Json_String_Overwrites_Existing_Values() {
    PropertyManagerProperty prop("Test", createPropMgrWithInt());

    auto secondMgr = createPropMgrWithInt();
    secondMgr->setProperty("Prop1", 5);
    TS_ASSERT_EQUALS("", prop.setValue(secondMgr->asString(true)));
    TS_ASSERT_EQUALS(secondMgr->asString(true), prop.value());
    auto retrieved = prop();
    TS_ASSERT_EQUALS(1, retrieved->propertyCount());
    TS_ASSERT(prop()->existsProperty("Prop1"));
  }

  void test_Property_Set_With_Json_String_Declares_Missing_Values() {
    PropertyManagerProperty prop("Test");
    const std::string jsonString = R"({"APROP":"equation=12+3","anotherProp":"1.3,2.5"})";

    TS_ASSERT_THROWS_NOTHING(prop.setValue(jsonString));

    auto mgr = prop();
    TS_ASSERT_EQUALS("equation=12+3", static_cast<std::string>(mgr->getProperty("APROP")));
    TS_ASSERT_EQUALS("1.3,2.5", static_cast<std::string>(mgr->getProperty("anotherProp")));
  }

  void test_Property_Set_With_Non_Json_ObjectValue_Returns_Help_Msg() {
    PropertyManagerProperty prop("Test");
    const std::string helpMsg{prop.setValueFromJson(Json::Value(1))};
    TS_ASSERT(!helpMsg.empty());
  }

  void test_Property_Set_With_Json_ObjectValue_Is_Accepted() {
    const std::string intKey{"k1"}, realKey{"k2"};
    const int intValue(1);
    const double realValue(5.3);
    Json::Value dict(Json::objectValue);
    dict[intKey] = intValue;
    dict[realKey] = realValue;

    PropertyManagerProperty prop("Test");
    prop.setValueFromJson(dict);

    auto propMgr = prop();
    TS_ASSERT_EQUALS(intValue, static_cast<int>(propMgr->getProperty(intKey)));
    TS_ASSERT_EQUALS(realValue, static_cast<double>(propMgr->getProperty(realKey)));
  }

  void test_Property_Set_With_String_Checks_PropertyManager_DataService() {
    using Mantid::Kernel::PropertyManagerDataService;
    auto globalMgr = createPropMgrWithInt();
    const std::string pmdsName("globalprops");
    auto &pmds = PropertyManagerDataService::Instance();
    pmds.add(pmdsName, globalMgr);
    PropertyManagerProperty prop("PMDSTest");

    TS_ASSERT_EQUALS("", prop.setValue(pmdsName));
    TS_ASSERT_EQUALS(pmdsName, prop.value());
    auto retrieved = prop();
    TS_ASSERT_EQUALS(1, retrieved->propertyCount());
    TS_ASSERT(prop()->existsProperty("Prop1"));

    pmds.remove(pmdsName);
  }

  void test_Property_Set_As_DataService_Key_Then_Json_Returns_Correct_String() {
    using Mantid::Kernel::PropertyManagerDataService;
    auto globalMgr = createPropMgrWithInt();
    const std::string pmdsName("globalprops");
    auto &pmds = PropertyManagerDataService::Instance();
    pmds.add(pmdsName, globalMgr);

    PropertyManagerProperty prop("PMDSTest");
    prop.setValue(pmdsName);
    TS_ASSERT_EQUALS(pmdsName, prop.value());
    pmds.remove(pmdsName);

    TS_ASSERT_EQUALS("", prop.setValue(globalMgr->asString(true)));
    TS_ASSERT_EQUALS(globalMgr->asString(true), prop.value());
  }

  void test_getDefault_Returns_Empty_String_For_Empty_Default() {
    PropertyManagerProperty prop("PMDSTest");
    TS_ASSERT_EQUALS("", prop.getDefault());
  }

  void test_getDefault_Returns_Correct_JSON_String_For_Given_Default() {
    auto mgr = createPropMgrWithInt();
    PropertyManagerProperty prop("PMDSTest", mgr);
    TS_ASSERT_EQUALS(mgr->asString(true), prop.getDefault());
  }

  void test_Empty_Property_Value_Returns_Empty_String_As_Value() {
    using Mantid::Kernel::PropertyManager;

    PropertyManagerProperty pmap("Test");
    TS_ASSERT_EQUALS("", pmap.value());
  }

  void test_asJson_Gives_Json_ObjectValue() {
    using Mantid::Kernel::PropertyManager;
    auto propMgr = std::make_shared<PropertyManager>();
    propMgr->declareProperty("IntProp", 1);
    propMgr->declareProperty("DoubleProp", 15.1);
    PropertyManagerProperty prop("PMDSTest", propMgr);

    auto jsonVal = prop.valueAsJson();

    TS_ASSERT_EQUALS(2, jsonVal.size());
    TS_ASSERT_EQUALS(1, jsonVal["IntProp"].asInt());
    TS_ASSERT_EQUALS(15.1, jsonVal["DoubleProp"].asDouble());
  }

  void test_Encode_Nested_PropertyManager_As_Nested_Json_Objects() {
    using Mantid::Kernel::PropertyManager;
    using Mantid::Kernel::PropertyManagerProperty;
    auto inner = std::make_shared<PropertyManager>();
    inner->declareProperty("IntProp", 2);
    inner->declareProperty("DoubleProp", 16.1);
    auto outer = std::make_shared<PropertyManager>();
    outer->declareProperty("IntProp", 1);
    outer->declareProperty(std::make_unique<PropertyManagerProperty>("PropMgr", inner));
    PropertyManagerProperty prop("PMDSTest", outer);

    auto outerVal = prop.valueAsJson();
    TS_ASSERT_EQUALS(2, outerVal.size());
    TS_ASSERT_EQUALS(1, outerVal["IntProp"].asInt());
    auto innerVal = outerVal["PropMgr"];
    TS_ASSERT_EQUALS(Json::objectValue, innerVal.type());
    TS_ASSERT_EQUALS(2, innerVal.size());
    TS_ASSERT_EQUALS(16.1, innerVal["DoubleProp"].asDouble());
    TS_ASSERT_EQUALS(2, innerVal["IntProp"].asDouble());
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Name_Is_Not_Accepted() {
    TS_ASSERT_THROWS(PropertyManagerProperty(""), const std::invalid_argument &);
  }

  void test_String_Not_Holding_Valid_Json_or_Global_PM_Name_Returns_Error() {
    PropertyManagerProperty prop("Test");
    std::string msg;
    TS_ASSERT_THROWS_NOTHING(msg = prop.setValue("notvalid"));
    TS_ASSERT(msg.length() > 0);
  }

private:
  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
  Mantid::Kernel::PropertyManager_sptr createPropMgrWithInt() {
    using Mantid::Kernel::PropertyManager;
    using Mantid::Kernel::PropertyWithValue;

    auto testMgr = std::make_shared<PropertyManager>();
    testMgr->declareProperty(std::make_unique<PropertyWithValue<int>>("Prop1", 1));

    return testMgr;
  }
};
