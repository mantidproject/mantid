#ifndef MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflEventPresenter.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflEventPresenterTest : public CxxTest::TestSuite {

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflEventPresenterTest *createSuite() {
    return new ReflEventPresenterTest();
  }
  static void destroySuite(ReflEventPresenterTest *suite) { delete suite; }

  ReflEventPresenterTest() {}

  void test_something() { TS_ASSERT(false); }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLEVENTPRESENTERTEST_H */
