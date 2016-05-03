#ifndef MANTID_KERNEL_PROPERTYMANAGERPROPERTYTEST_H_
#define MANTID_KERNEL_PROPERTYMANAGERPROPERTYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/PropertyManagerProperty.h"
#include "MantidKernel/PropertyManager.h"

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
    TS_ASSERT_EQUALS(Direction::Output, pmap.direction());
  }

  void test_Constructor_Sets_Name_Direction_DefaultValue() {
    using Mantid::Kernel::Direction;

    auto testMgr = createPropMgrWithInt();
    PropertyManagerProperty pmap("Test", testMgr, Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(Direction::Output, pmap.direction());
    TS_ASSERT_EQUALS(testMgr, pmap());
    TS_ASSERT(pmap.isDefault());
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

    PropertyManager_sptr args = topMgr->getProperty("Args");
  }

  void test_Property_Set_With_Json_String_Overwrites_Existing_Values() {
    PropertyManagerProperty prop("Test", createPropMgrWithInt());

    auto secondMgr = createPropMgrWithInt();
    secondMgr->setProperty("Prop1", 5);
    TS_ASSERT_EQUALS("", prop.setValue(secondMgr->asString(true)));
    auto retrieved = prop();
    TS_ASSERT_EQUALS(1, retrieved->propertyCount());
    TS_ASSERT(prop()->existsProperty("Prop1"));
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Name_Is_Not_Accepted() {
    TS_ASSERT_THROWS(PropertyManagerProperty(""), std::invalid_argument);
  }

  void test_Empty_Property_Set_With_Json_String_Throws_Error() {
    PropertyManagerProperty prop("Test");

    auto json = createPropMgrWithInt()->asString(true);
    std::string msg;
    TS_ASSERT_THROWS_NOTHING(msg = prop.setValue(json));
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
