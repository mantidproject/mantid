#ifndef MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_

#include "../ISISReflectometry/BatchPresenter.h"
#include "MockBatchView.h"
#include "MantidQtWidgets/Common/Batch/MockJobTreeView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using testing::Return;

class ReflBatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflBatchPresenterTest *createSuite() {
    return new ReflBatchPresenterTest();
  }

  static void destroySuite(ReflBatchPresenterTest *suite) { delete suite; }

  void testExpandsAllGroupsWhenRequested() {
    MantidQt::MantidWidgets::Batch::MockJobTreeView jobs;
    MockBatchView view;
    ON_CALL(view, jobs()).WillByDefault(::testing::ReturnRef(jobs));
    EXPECT_CALL(jobs, expandAll());

    BatchPresenter presenter(&view, {});
    presenter.notifyExpandAllRequested();
  }
};

#endif // MANTID_CUSTOMINTERFACES_REFLBATCHPRESENTERTEST_H_
