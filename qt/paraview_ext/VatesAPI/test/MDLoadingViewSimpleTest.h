#ifndef MD_LOADING_VIEW_SIMPLE_TEST_H
#define MD_LOADING_VIEW_SIMPLE_TEST_H

#include "MantidVatesAPI/MDLoadingViewSimple.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::VATES;

class MDLoadingViewSimpleTest : public CxxTest::TestSuite {

public:
  void test_that_defaults_are_returned() {
    MDLoadingViewSimple view;

    TSM_ASSERT_EQUALS("Should have a time of 0.0 by default", view.getTime(),
                      0.0);
    TSM_ASSERT_EQUALS("Should have a recursion depth of 5 by default",
                      view.getRecursionDepth(), 5);
    TSM_ASSERT_EQUALS("Should be set to loadingInMemory to true",
                      view.getLoadInMemory(), true);
  }

  void test_that_settings_are_correctly_stored() {
    // Arrange
    MDLoadingViewSimple view;
    auto loadingInMemory = false;
    auto time = 1.0;
    size_t recursionDepth = 7;
    view.setLoadInMemory(loadingInMemory);
    view.setRecursionDepth(recursionDepth);
    view.setTime(time);

    // Act + Assert
    TSM_ASSERT_EQUALS("Should have a time of 1.0", view.getTime(), time);
    TSM_ASSERT_EQUALS("Should have a recursion depth of 7",
                      view.getRecursionDepth(), recursionDepth);
    TSM_ASSERT_EQUALS("Should be set to loadingInMemory to false",
                      view.getLoadInMemory(), loadingInMemory);
  }
};

#endif
