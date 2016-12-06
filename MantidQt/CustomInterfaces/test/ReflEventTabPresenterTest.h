#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflEventTabPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflEventTabPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflEventTabPresenterTest *createSuite() {
    return new ReflEventTabPresenterTest();
  }
  static void destroySuite(ReflEventTabPresenterTest *suite) { delete suite; }

  ReflEventTabPresenterTest() {}

  void test_something() { TS_ASSERT(false); }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTTABPRESENTERTEST_H */
