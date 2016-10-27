#ifndef MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTabPresenter.h"
#include "ReflMockObjects.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflSaveTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflSaveTabPresenterTest *createSuite() {
    return new ReflSaveTabPresenterTest();
  }
  static void destroySuite(ReflSaveTabPresenterTest *suite) { delete suite; }

  ReflSaveTabPresenterTest() {}

  void test_something() {
    MockSaveTabView mockView;
    ReflSaveTabPresenter presenter(&mockView);

    TS_ASSERT(false);
  }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLSAVETABPRESENTERTEST_H */
