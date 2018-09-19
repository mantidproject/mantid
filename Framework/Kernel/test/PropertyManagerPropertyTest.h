#ifndef MANTID_KERNEL_PROPERTYMANAGERPROPERTYTEST_H_
#define MANTID_KERNEL_PROPERTYMANAGERPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include "MantidKernel/PropertyManagerProperty.h"

#include <boost/scoped_ptr.hpp>

using Mantid::Kernel::PropertyManagerProperty;

class PropertyManagerPropertyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyManagerPropertyTest *createSuite() {
    return new PropertyManagerPropertyTest();
  }
  static void destroySuite(PropertyManagerPropertyTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success tests
  //----------------------------------------------------------------------------
  void test_Constructor_Default_Direction_Is_Input() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test");
    // MSVC warns about comparing signed/unsigned here
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Input),
                     pmap.direction());
  }

  void test_Constructor_Sets_Name_And_Direction() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test", Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output),
                     pmap.direction());
  }

  void test_Constructor_Sets_Name_Direction_DefaultValue() {
    using Mantid::Kernel::Direction;

    auto testMgr = createPropMgrWithInt();
    PropertyManagerProperty pmap("Test", testMgr, Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output),
                     pmap.direction());
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
    TS_ASSERT_EQUALS(static_cast<unsigned int>(Direction::Output),
                     copy->direction());
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
    auto topMgr = boost::make_shared<PropertyManager>();
    topMgr->declareProperty(
        Mantid::Kernel::make_unique<PropertyManagerProperty>("Args"));
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

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Name_Is_Not_Accepted() {
    TS_ASSERT_THROWS(PropertyManagerProperty(""), std::invalid_argument);
  }

  void test_Empty_Property_Set_With_Json_String_Returns_Error() {
    PropertyManagerProperty prop("Test");

    auto json = createPropMgrWithInt()->asString(true);
    std::string msg;
    TS_ASSERT_THROWS_NOTHING(msg = prop.setValue(json));
    TS_ASSERT(msg.length() > 0);
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

    auto testMgr = boost::make_shared<PropertyManager>();
    testMgr->declareProperty(
        Mantid::Kernel::make_unique<PropertyWithValue<int>>("Prop1", 1));

    return testMgr;
  }
};

#endif /* MANTID_KERNEL_PROPERTYMANAGERPROPERTYTEST_H_ */
