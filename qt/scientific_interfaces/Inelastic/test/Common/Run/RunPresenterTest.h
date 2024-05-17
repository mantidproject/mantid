// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "../MockObjects.h"
#include "Common/Run/RunPresenter.h"

#include <memory>

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class RunPresenterTest : public CxxTest::TestSuite {
public:
  static RunPresenterTest *createSuite() { return new RunPresenterTest(); }

  static void destroySuite(RunPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_subscriber = std::make_unique<NiceMock<MockRunSubscriber>>();
    m_view = std::make_unique<NiceMock<MockRunView>>();

    m_presenter = std::make_unique<RunPresenter>(m_subscriber.get(), m_view.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_subscriber));

    m_presenter.reset();
    m_subscriber.reset();
    m_view.reset();
  }

  void test_handleRunClicked_calls_the_expected_subscriber_function() {
    EXPECT_CALL(*m_view, setRunEnabled(false)).Times(1);
    EXPECT_CALL(*m_subscriber, handleRunClicked()).Times(1);

    m_presenter->handleRunClicked();
  }

  void test_setRunEnabled_calls_the_appropriate_view_function() {
    EXPECT_CALL(*m_view, setRunEnabled(true)).Times(1);
    m_presenter->setRunEnabled(true);
  }

private:
  std::unique_ptr<NiceMock<MockRunSubscriber>> m_subscriber;
  std::unique_ptr<NiceMock<MockRunView>> m_view;
  std::unique_ptr<RunPresenter> m_presenter;
};
