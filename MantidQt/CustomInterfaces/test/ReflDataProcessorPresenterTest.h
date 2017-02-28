#ifndef MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H
#define MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtCustomInterfaces/Reflectometry/ReflDataProcessorPresenter.h"

class ReflDataProcessorPresenterTest : public CxxTest::TestSuite {

private:

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflDataProcessorPresenterTest *createSuite() {
    return new ReflDataProcessorPresenterTest();
  }
  static void destroySuite(ReflDataProcessorPresenterTest *suite) {
    delete suite;
  }

  ReflDataProcessorPresenterTest() { FrameworkManager::Instance(); }
};

#endif /* MANTID_CUSTOMINTERFACES_REFLDATAPROCESSORPRESENTERTEST_H */