#ifndef VATES_API_PRESENTER_UTILITIES_TEST_H_
#define VATES_API_PRESENTER_UTILITIES_TEST_H_

#include "MantidVatesAPI/FactoryChains.h"
#include <cxxtest/TestSuite.h>

class PresenterUtilitiesTest : public CxxTest::TestSuite {
public:
  void test_that_time_stamped_name_is_produced() {
    // Arrange
    std::string name = "testName";
    // Act
    auto timeStampedName = Mantid::VATES::createTimeStampedName(name);
    // Assert
    TSM_ASSERT("Time stamped name should be larger than the original name",
               name.size() < timeStampedName.size());
    TSM_ASSERT("Time stamped name should start with original name",
               timeStampedName.find(name) == 0);
  }
};
#endif
