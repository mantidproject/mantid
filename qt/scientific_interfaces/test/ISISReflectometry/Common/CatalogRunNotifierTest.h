// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#include "../../../ISISReflectometry/GUI/Runs/CatalogRunNotifier.h"
#include "../ReflMockObjects.h"
#include "../Runs/MockRunsView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::Mock;
using testing::NiceMock;
using testing::_;

class CatalogRunNotifierTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CatalogRunNotifierTest *createSuite() {
    return new CatalogRunNotifierTest();
  }
  static void destroySuite(CatalogRunNotifierTest *suite) { delete suite; }

  void testConstructorSubscribesToView() {
    EXPECT_CALL(m_view, subscribeTimer(_)).Times(1);
    auto runNotifier = makeRunNotifier();
    verifyAndClear();
  }

  void testStartPollingStartsTimer() {
    auto runNotifier = makeRunNotifier();
    EXPECT_CALL(m_view, startTimer(_)).Times(1);
    runNotifier.startPolling();
    verifyAndClear();
  }

  void testStopPollingStopsTimer() {
    auto runNotifier = makeRunNotifier();
    EXPECT_CALL(m_view, stopTimer()).Times(1);
    runNotifier.stopPolling();
    verifyAndClear();
  }

  void testTimerEventNotifiesPresenter() {
    auto runNotifier = makeRunNotifier();
    EXPECT_CALL(m_notifyee, notifyCheckForNewRuns()).Times(1);
    runNotifier.subscribe(&m_notifyee);
    runNotifier.notifyTimerEvent();
    verifyAndClear();
  }

private:
  NiceMock<MockRunsView> m_view;
  NiceMock<MockRunNotifierSubscriber> m_notifyee;

  CatalogRunNotifier makeRunNotifier() {
    auto runNotifier = CatalogRunNotifier(&m_view);
    runNotifier.subscribe(&m_notifyee);
    return runNotifier;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_notifyee));
  }
};
