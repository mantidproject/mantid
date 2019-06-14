// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef PROPERTYMANAGERDATASERVICETEST_H_
#define PROPERTYMANAGERDATASERVICETEST_H_

#include "MantidKernel/Exception.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidKernel/PropertyManagerDataService.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class PropertyManagerDataServiceTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PropertyManagerDataServiceTest *createSuite() {
    return new PropertyManagerDataServiceTest();
  }
  static void destroySuite(PropertyManagerDataServiceTest *suite) {
    delete suite;
  }

  PropertyManagerDataServiceTest()
      : inst1(new PropertyManager), inst2(new PropertyManager) {}
  void testAdd() {
    // Adding an Instrument with empty name should throw
    TS_ASSERT_THROWS(PropertyManagerDataService::Instance().add("", inst1),
                     const std::runtime_error &);
    // This should not throw, valid name
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().add("inst1", inst1));
    TS_ASSERT_EQUALS(inst1.use_count(), 2);
  }
  void testAddOrReplace() {
    // AddorReplace an Instrument with empty name should throw
    TS_ASSERT_THROWS(
        PropertyManagerDataService::Instance().addOrReplace("", inst2),
        const std::runtime_error &);
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().addOrReplace("inst2", inst2));
    TS_ASSERT_EQUALS(inst2.use_count(), 2);
    // Test replace
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().addOrReplace("inst1", inst2));
    TS_ASSERT_EQUALS(inst2.use_count(), 3);
    TS_ASSERT_EQUALS(inst1.use_count(), 1);
    // i
    TS_ASSERT_EQUALS(PropertyManagerDataService::Instance().retrieve("inst1"),
                     inst2);
    // Change back
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().addOrReplace("inst1", inst1));
    TS_ASSERT_EQUALS(inst2.use_count(), 2);
    TS_ASSERT_EQUALS(inst1.use_count(), 2);
  }
  void testSize() {
    // Number of elements in the store should now be 1
    TS_ASSERT_EQUALS(PropertyManagerDataService::Instance().size(), 2);
  }
  void testRetrieve() {
    // Retrieve the instrument
    TS_ASSERT_EQUALS(PropertyManagerDataService::Instance().retrieve("inst1"),
                     inst1);
    // Should throw if the instrument can not be retrieved
    TS_ASSERT_THROWS(
        PropertyManagerDataService::Instance().retrieve("notregistered"),
        const Mantid::Kernel::Exception::NotFoundError &);
  }
  void testRemove() {
    // Removing a non-existing data Object should give a warning in the Log but
    // not throw
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().remove("inst3"));
    // Removing a valid instrument
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().remove("inst1"));
    TS_ASSERT_EQUALS(PropertyManagerDataService::Instance().size(), 1);
    TS_ASSERT_EQUALS(inst1.use_count(), 1);
  }
  void testClear() {
    TS_ASSERT_THROWS_NOTHING(PropertyManagerDataService::Instance().clear());
    TS_ASSERT_EQUALS(PropertyManagerDataService::Instance().size(), 0);
    TS_ASSERT_EQUALS(inst1.use_count(), 1);
    TS_ASSERT_EQUALS(inst2.use_count(), 1);
  }
  void testDoesExist() {
    // Add inst1
    PropertyManagerDataService::Instance().add("inst1", inst1);
    TS_ASSERT_THROWS_NOTHING(
        PropertyManagerDataService::Instance().doesExist("youpla"));
    ;
    TS_ASSERT(PropertyManagerDataService::Instance().doesExist("inst1"));
    TS_ASSERT(!PropertyManagerDataService::Instance().doesExist("inst3"));
  }
  void testGetObjectNames() {
    PropertyManagerDataService::Instance().add("inst2", inst2);
    std::vector<std::string> expectedNames = {"inst1", "inst2"};
    auto result = PropertyManagerDataService::Instance().getObjectNames();
    TS_ASSERT_EQUALS(result, expectedNames);
    // Check with an empty store
    PropertyManagerDataService::Instance().clear();
    expectedNames.clear();
    result = PropertyManagerDataService::Instance().getObjectNames();
    TS_ASSERT_EQUALS(result, expectedNames);
  }

private:
  boost::shared_ptr<PropertyManager> inst1, inst2;
};

#endif /*PROPERTYMANAGERDATASERVICETEST_H_*/
