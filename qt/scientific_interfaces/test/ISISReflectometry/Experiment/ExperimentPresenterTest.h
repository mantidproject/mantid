#ifndef MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Experiment/ExperimentPresenter.h"
#include "MockExperimentView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;
using testing::Mock;
using testing::NiceMock;
using testing::_;

class ExperimentPresenterTest : public CxxTest::TestSuite  {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ExperimentPresenterTest *createSuite() {
    return new ExperimentPresenterTest();
  }
  static void destroySuite(ExperimentPresenterTest *suite) { delete suite; }

  ExperimentPresenterTest() : m_view() {}

  bool verifyAndClearExpectations() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    return true;
  }

  ExperimentPresenter makePresenter(IExperimentView &view) {
    return ExperimentPresenter(&view, /*thetaTolerance=*/0.01);
  }

  void test_this_is_a_test() {
    //...
  }

protected:
  NiceMock<MockExperimentView> m_view;
};

#endif // MANTID_CUSTOMINTERFACES_EXPERIMENTPRESENTERTEST_H_
