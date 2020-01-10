// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_MANTIDWIDGETS_OPTIONSDIALOGPRESENTERTEST_H
#define MANTID_MANTIDWIDGETS_OPTIONSDIALOGPRESENTERTEST_H

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

  OptionsDialogPresenterTest() : m_view(), m_model() {}

  void setUp() override {}

  void tearDown() override {}

  void testPresenterSubscribesToView() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    presenter.notifySubscribeView();
    verifyAndClear();
  }

  void testInitOptionsClearsVariables() {
    auto model =
        std::make_unique<NiceMock<MockOptionsDialogModelUnsuccessfulLoad>>();
    auto presenter = makePresenter(std::move(model));
    EXPECT_EQ(presenter.m_boolOptions.size(), 0);
    EXPECT_EQ(presenter.m_intOptions.size(), 0);
    presenter.notifyInitOptions();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_modelUnsuccessfulLoad));
  }

  void testInitOptionsAttemptsToLoadFromModel() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.saveOptions();
    EXPECT_CALL(*m_model, loadSettingsProxy(presenter.m_boolOptions,
                                            presenter.m_intOptions))
        .WillOnce(Return());
    presenter.notifyInitOptions();
    assertLoadOptions(presenter);
    verifyAndClear();
  }

  void testInitOptionsAppliesDefaultOptionsIfLoadUnsuccessful() {
    auto model = std::make_unique<
        NiceMock<MockOptionsDialogModelUnsuccessfulDefaults>>();
    auto presenter = makePresenter(std::move(model));
    EXPECT_CALL(*m_modelUnsuccessfulDefaults, loadSettingsProxy(_, _))
        .WillOnce(Return());
    EXPECT_CALL(*m_modelUnsuccessfulDefaults, applyDefaultOptionsProxy(_, _))
        .WillOnce(Return());
    presenter.notifyInitOptions();
    assertDefaultOptions(presenter);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_modelUnsuccessfulDefaults));
  }

  void testLoadOptionsQueriesModel() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.saveOptions();
    EXPECT_CALL(*m_model, loadSettingsProxy(presenter.m_boolOptions,
                                            presenter.m_intOptions))
        .Times(AtLeast(1));
    EXPECT_CALL(*mainWindowSubscriber.get(), optionsChanged())
        .Times(AtLeast(1));
    presenter.loadOptions();
    assertLoadOptions(presenter);
    verifyAndClear(std::move(mainWindowSubscriber));
  }

  void testLoadOptionsUpdatesView() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.saveOptions();
    EXPECT_CALL(m_view,
                setOptions(presenter.m_boolOptions, presenter.m_intOptions))
        .Times(AtLeast(1));
    EXPECT_CALL(*mainWindowSubscriber.get(), optionsChanged())
        .Times(AtLeast(1));
    presenter.loadOptions();
    assertLoadOptions(presenter);
    verifyAndClear(std::move(mainWindowSubscriber));
  }

  void testLoadOptionsNotifiesMainWindow() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    EXPECT_CALL(*mainWindowSubscriber.get(), optionsChanged())
        .Times(AtLeast(1));
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.loadOptions();
    assertLoadOptions(presenter);
    verifyAndClear(std::move(mainWindowSubscriber));
  }

  void testSaveOptionsUpdatesModel() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.loadOptions();
    EXPECT_CALL(*m_model,
                saveSettings(presenter.m_boolOptions, presenter.m_intOptions))
        .Times(AtLeast(1));
    presenter.saveOptions();
    verifyAndClear();
  }

  void testSaveOptionsNotifiesMainWindow() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();
    EXPECT_CALL(*mainWindowSubscriber.get(), optionsChanged())
        .Times(AtLeast(1));
    presenter.subscribe(mainWindowSubscriber.get());
    presenter.saveOptions();
    verifyAndClear(std::move(mainWindowSubscriber));
  }

  void testSaveOptionsQueriesView() {
    auto presenter = makePresenter();
    auto mainWindowSubscriber =
        std::make_unique<NiceMock<MockOptionsDialogMainWindowSubscriber>>();

    presenter.subscribe(mainWindowSubscriber.get());
    presenter.loadOptions();
    EXPECT_CALL(m_view,
                getOptions(presenter.m_boolOptions, presenter.m_intOptions))
        .Times(AtLeast(1));
    presenter.saveOptions();
    verifyAndClear();
  }

private:
  class OptionsDialogPresenterFriend : public OptionsDialogPresenter {
    friend class OptionsDialogPresenterTest;

  public:
    OptionsDialogPresenterFriend() = default;
    OptionsDialogPresenterFriend(IOptionsDialogView *view,
                                 std::unique_ptr<IOptionsDialogModel> model)
        : OptionsDialogPresenter(view, std::move(model)) {}
  };

  NiceMock<MockOptionsDialogView> m_view;
  NiceMock<MockOptionsDialogModel> *m_model;
  NiceMock<MockOptionsDialogModelUnsuccessfulLoad> *m_modelUnsuccessfulLoad;
  NiceMock<MockOptionsDialogModelUnsuccessfulDefaults>
      *m_modelUnsuccessfulDefaults;

  OptionsDialogPresenterFriend makePresenter() {
    auto model = std::make_unique<NiceMock<MockOptionsDialogModel>>();
    m_model = model.get();
    auto presenter = OptionsDialogPresenterFriend(&m_view, std::move(model));
    return presenter;
  }

  OptionsDialogPresenterFriend makePresenter(
      std::unique_ptr<NiceMock<MockOptionsDialogModelUnsuccessfulLoad>> model) {
    m_modelUnsuccessfulLoad = model.get();
    auto presenter = OptionsDialogPresenterFriend(&m_view, std::move(model));
    return presenter;
  }

  OptionsDialogPresenterFriend makePresenter(
      std::unique_ptr<NiceMock<MockOptionsDialogModelUnsuccessfulDefaults>>
          model) {
    m_modelUnsuccessfulDefaults = model.get();
    auto presenter = OptionsDialogPresenterFriend(&m_view, std::move(model));
    presenter.m_boolOptions["WarnDiscardChanges"] = true;
    presenter.m_boolOptions["WarnProcessAll"] = true;
    presenter.m_boolOptions["WarnProcessPartialGroup"] = true;
    presenter.m_boolOptions["Round"] = false;
    presenter.m_intOptions["RoundPrecision"] = 3;
    return presenter;
  }

  void assertLoadOptions(OptionsDialogPresenterFriend &presenter) {
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnProcessAll"], false);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnDiscardChanges"], true);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnProcessPartialGroup"], false);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["Round"], true);
    TS_ASSERT_EQUALS(presenter.m_intOptions["RoundPrecision"], 2);
  }

  void assertDefaultOptions(OptionsDialogPresenterFriend &presenter) {
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnProcessAll"], false);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnDiscardChanges"], false);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["WarnProcessPartialGroup"], false);
    TS_ASSERT_EQUALS(presenter.m_boolOptions["Round"], true);
    TS_ASSERT_EQUALS(presenter.m_intOptions["RoundPrecision"], 5);
  }

  void verifyAndClear(
      std::unique_ptr<NiceMock<MockOptionsDialogMainWindowSubscriber>>
          mainWindowSubscriber = std::make_unique<
              NiceMock<MockOptionsDialogMainWindowSubscriber>>()) {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mainWindowSubscriber));
  }
};
#endif // MANTID_MANTIDWIDGETS_OPTIONSDIALOGPRESENTERTEST_H
