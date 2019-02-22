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

  void test_that_the_model_has_been_instantiated_correctly() {}

private:
  MockIIndirectSettingsView *m_view;
  MockIndirectSettingsModel *m_model;
  std::unique_ptr<IndirectSettingsPresenter> m_presenter;
};

#endif /* MANTIDQT_INDIRECTSETTINGSPRESENTERTEST_H_ */
