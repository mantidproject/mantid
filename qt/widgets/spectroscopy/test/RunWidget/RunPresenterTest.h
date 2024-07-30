// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Common/MockUserInputValidator.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunPresenter.h"

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
    EXPECT_CALL(*m_subscriber, handleValidation(_)).Times(1);
    EXPECT_CALL(*m_view, setRunText("Running...")).Times(1);
    EXPECT_CALL(*m_subscriber, handleRun()).Times(1);

    m_presenter->handleRunClicked();
  }

  void test_setRunEnabled_true_calls_the_appropriate_view_function() {
    EXPECT_CALL(*m_view, setRunText("Run")).Times(1);
    m_presenter->setRunEnabled(true);
  }

  void test_setRunEnabled_false_calls_the_appropriate_view_function() {
    EXPECT_CALL(*m_view, setRunText("Running...")).Times(1);
    m_presenter->setRunEnabled(false);
  }

  void test_setRunText_calls_the_appropriate_view_function() {
    EXPECT_CALL(*m_view, setRunText("Finding file...")).Times(1);
    m_presenter->setRunText("Finding file...");
  }

  void test_validate_when_no_error_returned() {
    auto validator = std::make_unique<NiceMock<MockUserInputValidator>>();
    auto validatorRaw = validator.get();

    EXPECT_CALL(*m_subscriber, handleValidation(validatorRaw)).Times(1);
    ON_CALL(*validatorRaw, generateErrorMessage).WillByDefault(Return(""));

    // This should not be called
    EXPECT_CALL(*m_view, displayWarning(_)).Times(0);

    ASSERT_TRUE(m_presenter->validate(std::move(validator)));
  }

  void test_validate_when_an_error_message_is_returned() {
    auto const message = "This is an error message";

    auto validator = std::make_unique<NiceMock<MockUserInputValidator>>();
    auto validatorRaw = validator.get();

    EXPECT_CALL(*m_subscriber, handleValidation(validatorRaw)).Times(1);
    ON_CALL(*validatorRaw, generateErrorMessage).WillByDefault(Return(message));

    EXPECT_CALL(*m_view, displayWarning(message)).Times(1);

    ASSERT_FALSE(m_presenter->validate(std::move(validator)));
  }

private:
  std::unique_ptr<NiceMock<MockRunSubscriber>> m_subscriber;
  std::unique_ptr<NiceMock<MockRunView>> m_view;
  std::unique_ptr<RunPresenter> m_presenter;
};
