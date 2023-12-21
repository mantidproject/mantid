// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Common/Settings.h"
#include "Common/SettingsPresenter.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockSettingsView : public ISettingsView {
public:
  /// Public methods
  MOCK_METHOD0(getView, QWidget *());
  MOCK_METHOD1(subscribePresenter, void(SettingsPresenter *));

  MOCK_METHOD1(setInterfaceSettingsVisible, void(bool visible));
  MOCK_METHOD1(setInterfaceGroupBoxTitle, void(QString const &title));

  MOCK_METHOD1(setRestrictInputByNameVisible, void(bool visible));
  MOCK_METHOD1(setPlotErrorBarsVisible, void(bool visible));

  MOCK_METHOD1(setSelectedFacility, void(QString const &text));
  MOCK_CONST_METHOD0(getSelectedFacility, QString());

  MOCK_METHOD1(setRestrictInputByNameChecked, void(bool check));
  MOCK_CONST_METHOD0(isRestrictInputByNameChecked, bool());

  MOCK_METHOD1(setPlotErrorBarsChecked, void(bool check));
  MOCK_CONST_METHOD0(isPlotErrorBarsChecked, bool());

  MOCK_METHOD1(setDeveloperFeatureFlags, void(QStringList const &flags));
  MOCK_CONST_METHOD0(developerFeatureFlags, QStringList());

  MOCK_METHOD3(setSetting, void(QString const &settingsGroup, QString const &settingName, bool const &value));
  MOCK_METHOD2(getSetting, QVariant(QString const &settingsGroup, QString const &settingName));

  MOCK_METHOD1(setApplyText, void(QString const &text));
  MOCK_METHOD1(setApplyEnabled, void(bool enable));
  MOCK_METHOD1(setOkEnabled, void(bool enable));
  MOCK_METHOD1(setCancelEnabled, void(bool enable));
};

/// Mock object to mock the model
class MockSettingsModel : public SettingsModel {
public:
  /// Public methods
  MOCK_CONST_METHOD0(getSettingsGroup, std::string());

  MOCK_METHOD1(setFacility, void(std::string const &settingName));
  MOCK_CONST_METHOD0(getFacility, std::string());
};

class MockSettings : public ISettings {
public:
  MOCK_METHOD0(notifyApplySettings, void());
  MOCK_METHOD0(notifyCloseSettings, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class SettingsPresenterTest : public CxxTest::TestSuite {
public:
  static SettingsPresenterTest *createSuite() { return new SettingsPresenterTest(); }

  static void destroySuite(SettingsPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockSettingsView>>();
    auto model = std::make_unique<NiceMock<MockSettingsModel>>();
    m_model = model.get();
    m_presenter = std::make_unique<SettingsPresenter>(std::move(model), m_view.get());

    m_parent = std::make_unique<NiceMock<MockSettings>>();
    m_presenter->subscribeParent(m_parent.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));

    m_presenter.reset(); /// The model is destructed here
    m_view.reset();
    m_parent.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_calling_a_presenter_method_will_invoke_the_relevant_view_and_model_methods() {
    checkForLoadingOfSettings();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the views signals invoke the correct methods
  ///----------------------------------------------------------------------

  void test_that_the_okClicked_signal_will_attempt_to_save_the_settings() {
    checkForSavingOfSettings();
    m_presenter->notifyOkClicked();
  }

  void test_that_the_applyClicked_signal_will_attempt_to_save_the_settings() {
    checkForSavingOfSettings();
    m_presenter->notifyApplyClicked();
  }

  void test_that_the_applyClicked_signal_will_disable_the_settings_buttons_while_it_is_applying_the_changes() {
    Expectation disableApply = EXPECT_CALL(*m_view, setApplyEnabled(false)).Times(1);
    Expectation disableOk = EXPECT_CALL(*m_view, setOkEnabled(false)).Times(1);
    Expectation disableCancel = EXPECT_CALL(*m_view, setCancelEnabled(false)).Times(1);

    EXPECT_CALL(*m_view, setApplyEnabled(true)).Times(1).After(disableApply);
    EXPECT_CALL(*m_view, setOkEnabled(true)).Times(1).After(disableOk);
    EXPECT_CALL(*m_view, setCancelEnabled(true)).Times(1).After(disableCancel);

    m_presenter->notifyApplyClicked();
  }

private:
  void checkForLoadingOfSettings() {
    std::string const facility("ISIS");

    ON_CALL(*m_model, getFacility()).WillByDefault(Return(facility));

    ExpectationSet expectations = EXPECT_CALL(*m_model, getFacility()).WillOnce(Return(facility));
    expectations += EXPECT_CALL(*m_view, setSelectedFacility(QString::fromStdString(facility))).Times(1);

    m_presenter->loadSettings();
  }

  void checkForSavingOfSettings() {
    std::string const facility("ISIS");

    ON_CALL(*m_view, getSelectedFacility()).WillByDefault(Return(QString::fromStdString(facility)));
    ON_CALL(*m_view, isRestrictInputByNameChecked()).WillByDefault(Return(true));
    ON_CALL(*m_view, isPlotErrorBarsChecked()).WillByDefault(Return(true));
    ON_CALL(*m_view, developerFeatureFlags()).WillByDefault(Return(QStringList{""}));

    Expectation expectation = EXPECT_CALL(*m_view, getSelectedFacility()).Times(1);
    EXPECT_CALL(*m_model, setFacility(facility)).Times(1).After(expectation);
  }

  std::unique_ptr<NiceMock<MockSettingsView>> m_view;
  NiceMock<MockSettingsModel> *m_model;
  std::unique_ptr<SettingsPresenter> m_presenter;

  std::unique_ptr<NiceMock<MockSettings>> m_parent;
};
