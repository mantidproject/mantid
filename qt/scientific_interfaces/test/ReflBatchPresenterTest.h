#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_

#include "../ISISReflectometry/BatchPresenter.h"
#include "MockBatchView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class ReflBatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBatchPresenterTest *createSuite() {
    return new ReflBatchPresenterTest();
  }

  static void destroySuite(ReflEventTabPresenterTest *suite) {
    delete suite;
  }

  void testExpandsAllGroupsWhenRequested() {
    MockBatchView view;
    

  }


};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
