// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H
#define MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H

#include "GUI/Options/OptionsDialogPresenter.h"
#include "MockOptionsDialogModel.h"
#include "MockOptionsDialogPresenter.h"
#include "MockOptionsDialogView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class OptionsDialogPresenterTest : public CxxTest::TestSuite {
public:
  static OptionsDialogPresenterTest *createSuite() {
    return new OptionsDialogPresenterTest();
  }
  static void destroySuite(OptionsDialogPresenterTest *suite) { delete suite; }

  OptionsDialogPresenterTest()
      : m_view(), m_model(), m_boolOptions{{"WarnDiscardChanges", true},
                                           {"WarnProcessAll", true},
                                           {"WarnProcessPartialGroup ", true},
                                           {"Round", false}},
        m_intOptions{{"RoundPrecision", 3}} {}

  void testPresenterSubscribesToView() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    presenter.notifySubscribeView();
    verifyAndClear();
  }

  // TODO: these tests need rewriting now notifyInitOptions has gone and this is
  // done in the constructor instead

  //  void testInitOptionsClearsVariables() {
  //    auto presenter = makePresenter();
  //    EXPECT_EQ(m_boolOptions.size(), 0);
  //    EXPECT_EQ(m_intOptions.size(), 0);
  //    presenter.notifyInitOptions();
  //    verifyAndClear();
  //  }
  //
  //  void testInitOptionsAttemptsToLoadFromModel() {
  //    auto presenter = makePresenter();
  //    EXPECT_CALL(m_model, loadSettingsProxy(m_boolOptions, m_intOptions))
  //        .Times(1)
  //        .WillOnce(Return());
  //    presenter.notifyInitOptions();
  //    assertLoadOptions();
  //    verifyAndClear();
  //  }
  //
  //  void testInitOptionsAppliesDefaultOptionsIfLoadUnsuccessful() {
  //    auto presenter = makePresenter();
  //    EXPECT_CALL(m_model, loadSettingsProxy(m_boolOptions, m_intOptions))
  //        .Times(1)
  //        .WillOnce(Return());
  //    EXPECT_CALL(m_model, applyDefaultOptionsProxy(m_boolOptions,
  //    m_intOptions))
  //        .Times(1)
  //        .WillOnce(Return());
  //    presenter.notifyInitOptions();
  //    assertDefaultOptions();
  //    verifyAndClear();
  //  }

  void testLoadOptionsQueriesModel() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_model, loadSettingsProxy(m_boolOptions, m_intOptions))
        .Times(1)
        .WillOnce(Return());
    assertLoadOptions();
    presenter.notifyLoadOptions();
    verifyAndClear();
  }

  void testLoadOptionsUpdatesView() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, setOptions(m_boolOptions, m_intOptions)).Times(1);
    presenter.notifyLoadOptions();
    verifyAndClear();
  }

  void testLoadOptionsNotifiesMainWindow() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_notifyee, notifyOptionsChanged()).Times(1);
    presenter.notifyLoadOptions();
    verifyAndClear();
  }

  void testSaveOptionsUpdatesModel() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_model, saveSettings(m_boolOptions, m_intOptions)).Times(1);
    presenter.notifySaveOptions();
    verifyAndClear();
  }

  void testSaveOptionsNotifiesMainWindow() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_notifyee, notifyOptionsChanged()).Times(1);
    presenter.notifySaveOptions();
    verifyAndClear();
  }

  void testSaveOptionsQueriesView() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, getOptions(m_boolOptions, m_intOptions)).Times(1);
    presenter.notifySaveOptions();
    verifyAndClear();
  }

private:
  NiceMock<MockOptionsDialogView> m_view;
  NiceMock<MockOptionsDialogModel> *m_model;
  NiceMock<MockOptionsDialogPresenterSubscriber> m_notifyee;
  std::map<std::string, bool> m_boolOptions;
  std::map<std::string, int> m_intOptions;

  OptionsDialogPresenter makePresenter() {
    auto model = std::make_unique<NiceMock<MockOptionsDialogModel>>();
    m_model = model.get();
    auto presenter = OptionsDialogPresenter(&m_view, std::move(model));
    presenter.subscribe(&m_notifyee);
    return presenter;
  }

  void assertLoadOptions() {
    TS_ASSERT_EQUALS(m_boolOptions["WarnProcessAll"], false);
    TS_ASSERT_EQUALS(m_boolOptions["WarnDiscardChanges"], true);
    TS_ASSERT_EQUALS(m_boolOptions["WarnProcessPartialGroup"], false);
    TS_ASSERT_EQUALS(m_boolOptions["Round"], true);
    TS_ASSERT_EQUALS(m_intOptions["RoundPrecision"], 2);
  }

  void assertDefaultOptions() {
    TS_ASSERT_EQUALS(m_boolOptions["WarnProcessAll"], false);
    TS_ASSERT_EQUALS(m_boolOptions["WarnDiscardChanges"], false);
    TS_ASSERT_EQUALS(m_boolOptions["WarnProcessPartialGroup"], false);
    TS_ASSERT_EQUALS(m_boolOptions["Round"], true);
    TS_ASSERT_EQUALS(m_intOptions["RoundPrecision"], 5);
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_notifyee));
  }
};
#endif // MANTID_MANTIDWIDGETS_OPTIONSDIALOGTEST_H
