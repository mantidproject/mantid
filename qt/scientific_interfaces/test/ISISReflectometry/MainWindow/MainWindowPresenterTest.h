// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MAINWINDOWPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_MAINWINDOWPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/MainWindow/MainWindowPresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../Batch/MockBatchView.h"
#include "../ReflMockObjects.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/Instrument_fwd.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/MockSlitCalculator.h"
#include "MockMainWindowView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::
    ModelCreationHelper;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using MantidQt::MantidWidgets::ISlitCalculator;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;

class MainWindowPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MainWindowPresenterTest *createSuite() {
    return new MainWindowPresenterTest();
  }
  static void destroySuite(MainWindowPresenterTest *suite) { delete suite; }

  MainWindowPresenterTest() : m_view(), m_messageHandler() {
    Mantid::API::FrameworkManager::Instance();
    // Get the view to return a couple of batches by default
    m_batchViews.emplace_back(new MockBatchView);
    m_batchViews.emplace_back(new MockBatchView);
    ON_CALL(m_view, batches()).WillByDefault(Return(m_batchViews));
  }

  void setUp() override {
    auto &config = Mantid::Kernel::ConfigService::Instance();
    backup_facility = config.getString("default.facility");
    backup_instrument = config.getString("default.instrument");
  }

  void tearDown() override {
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.setString("default.facility", backup_facility);
    config.setString("default.instrument", backup_instrument);
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testConstructorAddsBatchPresenterForAllBatchViews() {
    EXPECT_CALL(m_view, batches()).Times(1);
    for (auto batchPresenter : m_batchPresenters)
      expectBatchAdded(batchPresenter);

    auto presenter = makePresenter();
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(), m_batchViews.size());
    verifyAndClear();
  }

  void testBatchPresenterAddedWhenNewBatchRequested() {
    auto presenter = makePresenter();
    auto batchView = new NiceMock<MockBatchView>();
    EXPECT_CALL(m_view, newBatch())
        .Times(1)
        .WillOnce(Return(dynamic_cast<IBatchView *>(batchView)));
    auto batchPresenter = new NiceMock<MockBatchPresenter>();
    EXPECT_CALL(*m_makeBatchPresenter, makeProxy(batchView))
        .Times(1)
        .WillOnce(Return(batchPresenter));
    expectBatchAdded(batchPresenter);

    presenter.notifyNewBatchRequested();
    verifyAndClear();
  }

  void testBatchRemovedWhenCloseBatchRequested() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchCanBeClosed(batchIndex);
    expectBatchRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertFirstBatchWasRemovedFromModel(presenter);
    verifyAndClear();
  }

  void testBatchNotRemovedIfRequestCloseFailed() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectRequestCloseBatchFailed(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
    verifyAndClear();
  }

  void testBatchNotRemovedIfAutoreducing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsAutoreducing(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
    verifyAndClear();
  }

  void testBatchNotRemovedIfProcessing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsProcessing(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
    verifyAndClear();
  }

  void testWarningGivenIfRemoveBatchWhileAutoreducing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsAutoreducing(batchIndex);
    expectCannotCloseBatchWarning();
    presenter.notifyCloseBatchRequested(batchIndex);
    verifyAndClear();
  }

  void testWarningGivenIfRemoveBatchWhileProcessing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsProcessing(batchIndex);
    expectCannotCloseBatchWarning();
    presenter.notifyCloseBatchRequested(batchIndex);
    verifyAndClear();
  }

  void testReductionResumedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchReductionResumed());
    presenter.notifyAnyBatchReductionResumed();
    verifyAndClear();
  }

  void testReductionPausedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchReductionPaused());
    presenter.notifyAnyBatchReductionPaused();
    verifyAndClear();
  }

  void testShowOptionsOpensDialog() {
    auto presenter = makePresenter();
    // TODO Add test when options dialog is impelemented
    // EXPECT_CALL(*m_optionsDialog, show()).Times(1);
    presenter.notifyShowOptionsRequested();
    verifyAndClear();
  }

  void testShowSlitCalculatorSetsInstrument() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "TEST_INSTRUMENT");
    expectSlitCalculatorInstrumentUpdated(instrument);
    presenter.notifyShowSlitCalculatorRequested();
    verifyAndClear();
  }

  void testShowSlitCalculatorOpensDialog() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_slitCalculator, show()).Times(1);
    presenter.notifyShowSlitCalculatorRequested();
    verifyAndClear();
  }

  void testAutoreductionResumedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionResumed());
    presenter.notifyAnyBatchAutoreductionResumed();
    verifyAndClear();
  }

  void testAutoreductionPausedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionPaused());
    presenter.notifyAnyBatchAutoreductionPaused();
    verifyAndClear();
  }

  void testAnyBatchIsProcessing() {
    auto presenter = makePresenter();
    expectBatchIsNotProcessing(0);
    expectBatchIsProcessing(1);
    auto isProcessing = presenter.isAnyBatchProcessing();
    TS_ASSERT_EQUALS(isProcessing, true);
    verifyAndClear();
  }

  void testNoBatchesAreProcessing() {
    auto presenter = makePresenter();
    expectBatchIsNotProcessing(0);
    expectBatchIsNotProcessing(1);
    auto isProcessing = presenter.isAnyBatchProcessing();
    TS_ASSERT_EQUALS(isProcessing, false);
    verifyAndClear();
  }

  void testAnyBatchIsAutoreducing() {
    auto presenter = makePresenter();
    expectBatchIsNotAutoreducing(0);
    expectBatchIsAutoreducing(1);
    auto isAutoreducing = presenter.isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(isAutoreducing, true);
    verifyAndClear();
  }

  void testNoBatchesAreAutoreducing() {
    auto presenter = makePresenter();
    expectBatchIsNotAutoreducing(0);
    expectBatchIsNotAutoreducing(1);
    auto isAutoreducing = presenter.isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(isAutoreducing, false);
    verifyAndClear();
  }

  void testChangeInstrumentRequestedUpdatesInstrumentInModel() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    presenter.notifyChangeInstrumentRequested(instrument);
    TS_ASSERT_EQUALS(presenter.instrumentName(), instrument);
    verifyAndClear();
  }

  void testChangeInstrumentRequestedUpdatesInstrumentInChildPresenters() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(*m_batchPresenters[0], notifyInstrumentChanged(instrument))
        .Times(1);
    EXPECT_CALL(*m_batchPresenters[1], notifyInstrumentChanged(instrument))
        .Times(1);
    presenter.notifyChangeInstrumentRequested(instrument);
    verifyAndClear();
  }

  void testUpdateInstrumentUpdatesInstrumentInChildPresenters() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "POLREF");
    EXPECT_CALL(*m_batchPresenters[0], notifyInstrumentChanged(instrument))
        .Times(1);
    EXPECT_CALL(*m_batchPresenters[1], notifyInstrumentChanged(instrument))
        .Times(1);
    presenter.notifyUpdateInstrumentRequested();
    verifyAndClear();
  }

  void testUpdateInstrumentUpdatesInstrumentInSlitCalculator() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "POLREF");
    expectSlitCalculatorInstrumentUpdated(instrument);
    presenter.notifyUpdateInstrumentRequested();
    verifyAndClear();
  }

  void testUpdateInstrumentDoesNotChangeInstrumentName() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "POLREF");
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(presenter.instrumentName(), instrument);
    verifyAndClear();
  }

  void testUpdateInstrumentThrowsIfInstrumentNotSet() {
    auto presenter = makePresenter();
    TS_ASSERT_THROWS_ANYTHING(presenter.notifyUpdateInstrumentRequested());
    verifyAndClear();
  }

  void testUpdateInstrumentSetsFacilityInConfig() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "POLREF");
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.setString("default.facility", "OLD_FACILITY");
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(config.getString("default.facility"), "ISIS");
    verifyAndClear();
  }

  void testUpdateInstrumentSetsInstrumentInConfig() {
    auto presenter = makePresenter();
    auto const instrument = setupInstrument(presenter, "POLREF");
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.setString("default.instrument", "OLD_INSTRUMENT");
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(config.getString("default.instrument"), instrument);
    verifyAndClear();
  }

private:
  NiceMock<MockMainWindowView> m_view;
  NiceMock<MockMessageHandler> m_messageHandler;
  std::vector<IBatchView *> m_batchViews;
  std::vector<NiceMock<MockBatchPresenter> *> m_batchPresenters;
  NiceMock<MockBatchPresenterFactory> *m_makeBatchPresenter;
  NiceMock<MockSlitCalculator> *m_slitCalculator;

  class MainWindowPresenterFriend : public MainWindowPresenter {
    friend class MainWindowPresenterTest;

  public:
    MainWindowPresenterFriend(
        IMainWindowView *view, IMessageHandler *messageHandler,
        std::unique_ptr<ISlitCalculator> slitCalculator,
        std::unique_ptr<IBatchPresenterFactory> makeBatchPresenter)
        : MainWindowPresenter(view, messageHandler, std::move(slitCalculator),
                              std::move(makeBatchPresenter)) {}
  };

  MainWindowPresenterFriend makePresenter() {
    auto slitCalculator = std::make_unique<NiceMock<MockSlitCalculator>>();
    m_slitCalculator = slitCalculator.get();
    auto makeBatchPresenter =
        std::make_unique<NiceMock<MockBatchPresenterFactory>>();
    m_makeBatchPresenter = makeBatchPresenter.get();
    // Set up a mock batch presenter for each view to be returned from the
    // factory
    for (auto batchView : m_batchViews) {
      auto batchPresenter = new NiceMock<MockBatchPresenter>();
      m_batchPresenters.emplace_back(batchPresenter);
      ON_CALL(*m_makeBatchPresenter, makeProxy(batchView))
          .WillByDefault(Return(batchPresenter));
    }
    // Make the presenter
    auto presenter = MainWindowPresenterFriend(&m_view, &m_messageHandler,
                                               std::move(slitCalculator),
                                               std::move(makeBatchPresenter));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_messageHandler));
    for (auto batchPresenter : m_batchPresenters)
      TS_ASSERT(Mock::VerifyAndClearExpectations(batchPresenter));
    m_batchPresenters.clear();
  }

  std::string setupInstrument(MainWindowPresenterFriend &presenter,
                              std::string const &instrumentName) {
    presenter.m_instrument =
        boost::make_shared<Mantid::Geometry::Instrument>(instrumentName);
    return presenter.instrumentName();
  }

  void expectBatchAdded(MockBatchPresenter *batchPresenter) {
    EXPECT_CALL(*batchPresenter, acceptMainPresenter(_)).Times(1);
    EXPECT_CALL(*batchPresenter, initInstrumentList()).Times(1);
    EXPECT_CALL(*batchPresenter, notifyInstrumentChanged(_)).Times(1);
    EXPECT_CALL(*batchPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
  }

  void expectBatchCanBeClosed(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing())
        .Times(1)
        .WillOnce(Return(false));
    EXPECT_CALL(*m_batchPresenters[batchIndex], requestClose())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectBatchIsAutoreducing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectBatchIsProcessing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing())
        .Times(1)
        .WillOnce(Return(true));
  }

  void expectBatchIsNotAutoreducing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing())
        .Times(1)
        .WillOnce(Return(false));
  }

  void expectBatchIsNotProcessing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing())
        .Times(1)
        .WillOnce(Return(false));
  }

  void expectRequestCloseBatchFailed(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], requestClose())
        .Times(1)
        .WillOnce(Return(false));
  }

  void expectBatchRemovedFromView(int batchIndex) {
    EXPECT_CALL(m_view, removeBatch(batchIndex)).Times(1);
  }

  void expectBatchNotRemovedFromView(int batchIndex) {
    EXPECT_CALL(m_view, removeBatch(batchIndex)).Times(0);
  }

  void expectCannotCloseBatchWarning() {
    EXPECT_CALL(m_messageHandler,
                giveUserCritical("Cannot close batch while processing or "
                                 "autoprocessing is in progress",
                                 "Error"))
        .Times(1);
  }

  void expectSlitCalculatorInstrumentUpdated(std::string const &instrument) {
    EXPECT_CALL(*m_slitCalculator, setCurrentInstrumentName(instrument))
        .Times(1);
    EXPECT_CALL(*m_slitCalculator, processInstrumentHasBeenChanged()).Times(1);
  }

  void assertFirstBatchWasRemovedFromModel(
      MainWindowPresenterFriend const &presenter) {
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(), 1);
    // Note that our local list of raw pointers is not updated so
    // the first item is invalid and the second item is now the
    // only remaining batch presenter in the model
    TS_ASSERT_EQUALS(presenter.m_batchPresenters[0].get(),
                     m_batchPresenters[1]);
  }

  void
  assertBatchNotRemovedFromModel(MainWindowPresenterFriend const &presenter) {
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(),
                     m_batchPresenters.size());
    for (size_t index = 0; index < m_batchPresenters.size(); ++index)
      TS_ASSERT_EQUALS(presenter.m_batchPresenters[index].get(),
                       m_batchPresenters[index]);
  }

private:
  std::string backup_facility;
  std::string backup_instrument;
};

#endif // MANTID_CUSTOMINTERFACES_MAINWINDOWPRESENTERTEST_H_
