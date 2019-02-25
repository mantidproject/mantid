// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTSETTINGSPRESENTERTEST_H_
#define MANTIDQT_INDIRECTSETTINGSPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectSettingsPresenter.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIIndirectSettingsView : public IIndirectSettingsView {
public:
  /// Signals
  void emitUpdateRestrictInputByName(std::string const &text) {
    emit updateRestrictInputByName(text);
  }

  void emitOkClicked() { emit okClicked(); }

  void emitApplyClicked() { emit applyClicked(); }

  void emitCancelClicked() { emit cancelClicked(); }

  /// Public methods
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

  MOCK_METHOD3(setSetting, void(QString const &settingsGroup,
                                QString const &settingName, bool const &value));
  MOCK_METHOD2(getSetting, QVariant(QString const &settingsGroup,
                                    QString const &settingName));

  MOCK_METHOD1(setApplyText, void(QString const &text));
  MOCK_METHOD1(setApplyEnabled, void(bool enable));
  MOCK_METHOD1(setOkEnabled, void(bool enable));
  MOCK_METHOD1(setCancelEnabled, void(bool enable));
};

/// Mock object to mock the model
class MockIndirectSettingsModel : public IndirectSettingsModel {
public:
  /// Public methods
  MOCK_CONST_METHOD0(getSettingsGroup, std::string());

  MOCK_CONST_METHOD0(hasInterfaceSettings, bool());
  MOCK_CONST_METHOD1(isSettingAvailable, bool(std::string const &settingName));

  MOCK_METHOD1(setFacility, void(std::string const &settingName));
  MOCK_CONST_METHOD0(getFacility, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectSettingsPresenterTest : public CxxTest::TestSuite {
public:
  static IndirectSettingsPresenterTest *createSuite() {
    return new IndirectSettingsPresenterTest();
  }

  static void destroySuite(IndirectSettingsPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_view = new NiceMock<MockIIndirectSettingsView>();
    m_model = new NiceMock<MockIndirectSettingsModel>();
    m_presenter = std::make_unique<IndirectSettingsPresenter>(m_model, m_view);
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    m_presenter.reset(); /// The view and model are destructed here
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void
  test_that_calling_a_presenter_method_will_invoke_the_relevant_view_and_model_methods() {
    QString const settingGroup("Data Analysis");
    checkForLoadingOfSettings(settingGroup);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the views signals invoke the correct methods
  ///----------------------------------------------------------------------

  void
  test_that_the_updateRestrictInputByName_signal_will_set_the_input_data_to_be_restricted_when_passed_ISIS() {
    EXPECT_CALL(*m_view, setRestrictInputByNameChecked(true)).Times(1);
    m_view->emitUpdateRestrictInputByName("ISIS");
  }

  void
  test_that_the_updateRestrictInputByName_signal_will_set_the_input_data_to_be_unrestricted_when_not_passed_ISIS() {
    EXPECT_CALL(*m_view, setRestrictInputByNameChecked(false)).Times(1);
    m_view->emitUpdateRestrictInputByName("ILL");
  }

  void test_that_the_okClicked_signal_will_attempt_to_save_the_settings() {
    QString const settingGroup("Data Analysis");

    checkForSavingOfSettings(settingGroup);

    m_view->emitOkClicked();
  }

  void test_that_the_applyClicked_signal_will_attempt_to_save_the_settings() {
    QString const settingGroup("Data Analysis");

    checkForSavingOfSettings(settingGroup);

    m_view->emitApplyClicked();
  }

  void
  test_that_the_applyClicked_signal_will_disable_the_settings_buttons_while_it_is_applying_the_changes() {
    Expectation disableApply =
        EXPECT_CALL(*m_view, setApplyEnabled(false)).Times(1);
    Expectation disableOk = EXPECT_CALL(*m_view, setOkEnabled(false)).Times(1);
    Expectation disableCancel =
        EXPECT_CALL(*m_view, setCancelEnabled(false)).Times(1);

    EXPECT_CALL(*m_view, setApplyEnabled(true)).Times(1).After(disableApply);
    EXPECT_CALL(*m_view, setOkEnabled(true)).Times(1).After(disableOk);
    EXPECT_CALL(*m_view, setCancelEnabled(true)).Times(1).After(disableCancel);

    m_view->emitApplyClicked();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods of the presenter
  ///----------------------------------------------------------------------

  void test_that_loadSettings_will_attempt_to_load_the_relevant_settings() {
    QString const settingGroup("Data Analysis");
    checkForLoadingOfSettings(settingGroup);
  }

  void test_that_getSetting_will_attempt_to_get_a_setting_from_the_view() {
    QString const settingGroup("Data Analysis");
    std::string const settingName("plot-error-bars");

    ON_CALL(*m_model, getSettingsGroup())
        .WillByDefault(Return(settingGroup.toStdString()));
    ON_CALL(*m_view,
            getSetting(settingGroup, QString::fromStdString(settingName)))
        .WillByDefault(Return(true));

    Expectation getSettingsGroup =
        EXPECT_CALL(*m_model, getSettingsGroup())
            .Times(1)
            .WillOnce(Return(settingGroup.toStdString()));
    EXPECT_CALL(*m_view,
                getSetting(settingGroup, QString::fromStdString(settingName)))
        .Times(1)
        .After(getSettingsGroup);

    m_presenter->getSetting(settingName);
  }

private:
  void checkForLoadingOfSettings(QString const &settingGroup) {
    ON_CALL(*m_model, isSettingAvailable("restrict-input-by-name"))
        .WillByDefault(Return(true));
    ON_CALL(*m_model, isSettingAvailable("plot-error-bars"))
        .WillByDefault(Return(true));
    ON_CALL(*m_model, getSettingsGroup())
        .WillByDefault(Return(settingGroup.toStdString()));
    ON_CALL(*m_view,
            getSetting(settingGroup, QString("restrict-input-by-name")))
        .WillByDefault(Return(true));
    ON_CALL(*m_view, getSetting(settingGroup, QString("plot-error-bars")))
        .WillByDefault(Return(true));

    ExpectationSet expectations =
        EXPECT_CALL(*m_model, isSettingAvailable("restrict-input-by-name"))
            .Times(1)
            .WillOnce(Return(true));
    expectations +=
        EXPECT_CALL(*m_view, setRestrictInputByNameChecked(true)).Times(1);
    expectations += EXPECT_CALL(*m_model, isSettingAvailable("plot-error-bars"))
                        .Times(1)
                        .WillOnce(Return(true));
    EXPECT_CALL(*m_view, setPlotErrorBarsChecked(true))
        .Times(1)
        .After(expectations);

    m_presenter->loadSettings();
  }

  void checkForSavingOfSettings(QString const &settingGroup) {
    ON_CALL(*m_model, getSettingsGroup())
        .WillByDefault(Return(settingGroup.toStdString()));
    ON_CALL(*m_model, isSettingAvailable("restrict-input-by-name"))
        .WillByDefault(Return(false));
    ON_CALL(*m_model, isSettingAvailable("plot-error-bars"))
        .WillByDefault(Return(false));

    EXPECT_CALL(*m_model, getSettingsGroup())
        .Times(1)
        .WillOnce(Return(settingGroup.toStdString()));
    EXPECT_CALL(*m_model, isSettingAvailable("restrict-input-by-name"))
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_model, isSettingAvailable("plot-error-bars"))
        .Times(1)
        .WillOnce(Return(false));
  }

  MockIIndirectSettingsView *m_view;
  MockIndirectSettingsModel *m_model;
  std::unique_ptr<IndirectSettingsPresenter> m_presenter;
};

#endif /* MANTIDQT_INDIRECTSETTINGSPRESENTERTEST_H_ */
