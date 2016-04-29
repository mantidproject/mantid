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
  void test_Default_Direction_Is_Input() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test");
    TS_ASSERT_EQUALS(Direction::Input, pmap.direction());
  }

  void test_Name_And_Direction_Are_Set_Correctly() {
    using Mantid::Kernel::Direction;

    PropertyManagerProperty pmap("Test", Direction::Output);
    TS_ASSERT_EQUALS("Test", pmap.name());
    TS_ASSERT_EQUALS(Direction::Output, pmap.direction());
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

  void test_PropertyManager_Can_Implicitly_Convert_To_Value_Type() {
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

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------
  void test_Empty_Name_Is_Not_Accepted() {
    TS_ASSERT_THROWS(PropertyManagerProperty(""), std::invalid_argument);
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
