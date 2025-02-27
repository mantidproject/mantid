// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/MainWindow/MainWindowPresenter.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "../Batch/MockBatchView.h"
#include "../Options/MockOptionsDialogPresenter.h"
#include "../Options/MockOptionsDialogView.h"
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
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using MantidQt::MantidWidgets::ISlitCalculator;
using testing::_;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::ReturnRef;
using testing::Throw;

namespace {
static const std::string DEFAULT_INSTRUMENT = "INTER";
} // unnamed namespace

class MainWindowPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MainWindowPresenterTest *createSuite() { return new MainWindowPresenterTest(); }
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
    // Verifying and clearing of expectations happens when mock variables are destroyed.
    // Some of our mocks are created as member variables and will exist until all tests have run, so we need to
    // explicitly verify and clear them after each test.
    verifyAndClear();
  }

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
  }

  void testMainWindowPresenterSubscribesToOptionsPresenter() {
    auto optionsPresenter = makeOptionsPresenter();
    EXPECT_CALL(*m_optionsPresenter, subscribe(_)).Times(1);
    auto presenter = makePresenter(std::move(optionsPresenter));
  }

  void testConstructorAddsBatchPresenterForAllBatchViews() {
    EXPECT_CALL(m_view, batches()).Times(1);
    auto optionsPresenter = makeOptionsPresenter();
    auto slitCalculator = makeSlitCalculator();
    auto makeBatchPresenter = makeBatchPresenterFactory();
    expectSlitCalculatorInstrumentUpdated(DEFAULT_INSTRUMENT);
    for (size_t i = 0; i < m_batchPresenters.size(); ++i) {
      expectBatchAdded(m_batchPresenters[i], DEFAULT_INSTRUMENT);
    }
    auto presenter =
        makePresenter(std::move(optionsPresenter), std::move(slitCalculator), std::move(makeBatchPresenter));
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(), m_batchViews.size());
  }

  void testBatchPresenterAddedWhenNewBatchRequested() {
    auto presenter = makePresenter();
    auto batchView = new NiceMock<MockBatchView>();
    EXPECT_CALL(m_view, newBatch()).Times(1).WillOnce(Return(dynamic_cast<IBatchView *>(batchView)));
    auto batchPresenter = new NiceMock<MockBatchPresenter>();
    EXPECT_CALL(*m_makeBatchPresenter, makeProxy(batchView)).Times(1).WillOnce(Return(batchPresenter));
    expectBatchAdded(batchPresenter, DEFAULT_INSTRUMENT);
    expectSlitCalculatorInstrumentNotUpdated();

    presenter.notifyNewBatchRequested();
  }

  void testBatchRemovedWhenCloseBatchRequested() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchCanBeClosed(batchIndex);
    expectBatchRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertFirstBatchWasRemovedFromModel(presenter);
  }

  void testBatchNotRemovedIfRequestCloseFailed() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectRequestCloseBatchFailed(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
  }

  void testBatchNotRemovedIfAutoreducing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsAutoreducing(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
  }

  void testBatchNotRemovedIfProcessing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsProcessing(batchIndex);
    expectBatchNotRemovedFromView(batchIndex);
    presenter.notifyCloseBatchRequested(batchIndex);
    assertBatchNotRemovedFromModel(presenter);
  }

  void testWarningGivenIfRemoveBatchWhileAutoreducing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsAutoreducing(batchIndex);
    expectCannotCloseBatchWarning();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testWarningGivenIfRemoveBatchWhileProcessing() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsProcessing(batchIndex);
    expectCannotCloseBatchWarning();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testWarningGivenIfRemoveUnsavedBatchOptionChecked() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsNotAutoreducing(batchIndex);
    expectBatchIsNotProcessing(batchIndex);
    expectWarnDiscardChanges(true);
    expectBatchUnsaved(batchIndex);
    expectAskDiscardChanges();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testNoWarningGivenIfRemoveUnsavedBatchOptionUnchecked() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsNotAutoreducing(batchIndex);
    expectBatchIsNotProcessing(batchIndex);
    expectBatchSaved(batchIndex);
    expectDoNotAskDiscardChanges();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testNoWarningIfRemoveSavedBatchOptionChecked() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsNotAutoreducing(batchIndex);
    expectBatchIsNotProcessing(batchIndex);
    expectBatchSaved(batchIndex);
    expectDoNotAskDiscardChanges();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testNoWarningIfRemoveSavedBatchOptionUnchecked() {
    auto presenter = makePresenter();
    auto const batchIndex = 0;
    expectBatchIsNotAutoreducing(batchIndex);
    expectBatchIsNotProcessing(batchIndex);
    expectBatchSaved(batchIndex);
    expectDoNotAskDiscardChanges();
    presenter.notifyCloseBatchRequested(batchIndex);
  }

  void testReductionResumedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchReductionResumed());
    presenter.notifyAnyBatchReductionResumed();
  }

  void testReductionPausedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchReductionPaused());
    presenter.notifyAnyBatchReductionPaused();
  }

  void testShowOptionsOpensDialog() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_optionsPresenter, showView()).Times(AtLeast(1));
    presenter.notifyShowOptionsRequested();
  }

  void testShowSlitCalculatorSetsInstrument() {
    auto presenter = makePresenter();
    expectSlitCalculatorInstrumentUpdated(DEFAULT_INSTRUMENT);
    presenter.notifyShowSlitCalculatorRequested();
  }

  void testShowSlitCalculatorOpensDialog() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_slitCalculator, show()).Times(1);
    presenter.notifyShowSlitCalculatorRequested();
  }

  void testAutoreductionResumedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionResumed());
    presenter.notifyAnyBatchAutoreductionResumed();
  }

  void testAutoreductionPausedNotifiesAllBatchPresenters() {
    auto presenter = makePresenter();
    for (auto batchPresenter : m_batchPresenters)
      EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionPaused());
    presenter.notifyAnyBatchAutoreductionPaused();
  }

  void testAnyBatchIsProcessing() {
    auto presenter = makePresenter();
    expectBatchIsNotProcessing(0);
    expectBatchIsProcessing(1);
    auto isProcessing = presenter.isAnyBatchProcessing();
    TS_ASSERT_EQUALS(isProcessing, true);
  }

  void testNoBatchesAreProcessing() {
    auto presenter = makePresenter();
    expectBatchIsNotProcessing(0);
    expectBatchIsNotProcessing(1);
    auto isProcessing = presenter.isAnyBatchProcessing();
    TS_ASSERT_EQUALS(isProcessing, false);
  }

  void testAnyBatchIsAutoreducing() {
    auto presenter = makePresenter();
    expectBatchIsNotAutoreducing(0);
    expectBatchIsAutoreducing(1);
    auto isAutoreducing = presenter.isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(isAutoreducing, true);
  }

  void testNoBatchesAreAutoreducing() {
    auto presenter = makePresenter();
    expectBatchIsNotAutoreducing(0);
    expectBatchIsNotAutoreducing(1);
    auto isAutoreducing = presenter.isAnyBatchAutoreducing();
    TS_ASSERT_EQUALS(isAutoreducing, false);
  }

  void testChangeInstrumentRequestedUpdatesInstrumentInModel() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    presenter.notifyChangeInstrumentRequested(instrument);
    TS_ASSERT_EQUALS(presenter.instrumentName(), instrument);
  }

  void testChangeInstrumentRequestedUpdatesInstrumentInChildPresenters() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(*m_batchPresenters[0], notifyInstrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_batchPresenters[1], notifyInstrumentChanged(instrument)).Times(1);
    presenter.notifyChangeInstrumentRequested(instrument);
  }

  void testChangeInstrumentRequestedDoesNotUpdateInstrumentIfNotChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_batchPresenters[0], notifyInstrumentChanged(DEFAULT_INSTRUMENT)).Times(0);
    EXPECT_CALL(*m_batchPresenters[1], notifyInstrumentChanged(DEFAULT_INSTRUMENT)).Times(0);
    presenter.notifyChangeInstrumentRequested(DEFAULT_INSTRUMENT);
  }

  void testChangeInstrumentUpdatesInstrumentInSlitCalculator() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    expectSlitCalculatorInstrumentUpdated(instrument);
    presenter.notifyChangeInstrumentRequested(instrument);
  }

  void testChangeInstrumentDoesNotUpdateInstrumentInSlitCalculatorIfNotChanged() {
    auto presenter = makePresenter();
    expectSlitCalculatorInstrumentNotUpdated();
    presenter.notifyChangeInstrumentRequested(DEFAULT_INSTRUMENT);
  }

  void testUpdateInstrumentDoesNotUpdateInstrumentInSlitCalculator() {
    auto presenter = makePresenter();
    expectSlitCalculatorInstrumentNotUpdated();
    presenter.notifyUpdateInstrumentRequested();
  }

  void testUpdateInstrumentDoesNotUpdateInstrumentInChildPresenters() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_batchPresenters[0], notifyInstrumentChanged(DEFAULT_INSTRUMENT)).Times(0);
    EXPECT_CALL(*m_batchPresenters[1], notifyInstrumentChanged(DEFAULT_INSTRUMENT)).Times(0);
    presenter.notifyUpdateInstrumentRequested();
  }

  void testUpdateInstrumentDoesNotChangeInstrumentName() {
    auto presenter = makePresenter();
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(presenter.instrumentName(), DEFAULT_INSTRUMENT);
  }

  void testUpdateInstrumentThrowsIfInstrumentNotSet() {
    auto presenter = makePresenter();
    presenter.m_instrument = nullptr;
    TS_ASSERT_THROWS_ANYTHING(presenter.notifyUpdateInstrumentRequested());
  }

  void testUpdateInstrumentSetsFacilityInConfig() {
    auto presenter = makePresenter();
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.setString("default.facility", "OLD_FACILITY");
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(config.getString("default.facility"), "ISIS");
  }

  void testUpdateInstrumentSetsInstrumentInConfig() {
    auto presenter = makePresenter();
    auto &config = Mantid::Kernel::ConfigService::Instance();
    config.setString("default.instrument", "OLD_INSTRUMENT");
    presenter.notifyUpdateInstrumentRequested();
    TS_ASSERT_EQUALS(config.getString("default.instrument"), DEFAULT_INSTRUMENT);
  }

  void testCloseEventChecksIfPrevented() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_batchPresenters[0], isProcessing).Times(1);
    EXPECT_CALL(*m_batchPresenters[1], isProcessing).Times(1);
    EXPECT_CALL(*m_batchPresenters[0], isAutoreducing).Times(1);
    EXPECT_CALL(*m_batchPresenters[1], isAutoreducing).Times(1);
    EXPECT_CALL(m_view, acceptCloseEvent).Times(1);
    presenter.notifyCloseEvent();
  }

  void testCloseEventIgnoredIfAutoreducing() {
    auto presenter = makePresenter();
    expectBatchIsAutoreducing(0);
    EXPECT_CALL(m_view, ignoreCloseEvent).Times(1);
    presenter.notifyCloseEvent();
  }

  void testCloseEventIgnoredIfProcessing() {
    auto presenter = makePresenter();
    expectBatchIsProcessing(0);
    EXPECT_CALL(m_view, ignoreCloseEvent).Times(1);
    presenter.notifyCloseEvent();
  }

  void testCloseEventAcceptedIfNotWorking() {
    auto presenter = makePresenter();
    expectBatchIsNotAutoreducing(0);
    expectBatchIsNotProcessing(0);
    EXPECT_CALL(m_view, acceptCloseEvent).Times(1);
    presenter.notifyCloseEvent();
  }

  void testSaveBatch() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectBatchIsSavedToFile(batchIndex);
    presenter.notifySaveBatchRequested(batchIndex);
  }

  void testSaveBatchToInvalidPath() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectBatchIsNotSavedToInvalidFile(batchIndex);
    presenter.notifySaveBatchRequested(batchIndex);
  }

  void testSaveBatchHandlesFailedSave() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectBatchIsNotSavedWhenSaveFails(batchIndex);
    presenter.notifySaveBatchRequested(batchIndex);
  }

  void testLoadBatch() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectBatchIsLoadedFromFile(batchIndex);
    presenter.notifyLoadBatchRequested(batchIndex);
  }

  void testWarningGivenIfLoadBatchOverUnsavedBatch() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectWarnDiscardChanges(true);
    expectBatchUnsaved(batchIndex);
    expectAskDiscardChanges();
    presenter.notifyLoadBatchRequested(batchIndex);
  }

  void testNoWarningGivenIfLoadBatchOverSavedBatch() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectBatchSaved(batchIndex);
    expectDoNotAskDiscardChanges();
    presenter.notifyLoadBatchRequested(batchIndex);
  }

  void testLoadBatchDiscardChanges() {
    auto presenter = makePresenter();
    auto const filename = std::string("test.json");
    auto const map = QMap<QString, QVariant>();
    auto const batchIndex = 1;
    expectWarnDiscardChanges(true);
    expectBatchUnsaved(batchIndex);
    expectUserDiscardsChanges();
    EXPECT_CALL(m_messageHandler, askUserForLoadFileName("JSON (*.json)")).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(m_fileHandler, loadJSONFromFile(filename)).Times(1).WillOnce(Return(map));
    EXPECT_CALL(*m_decoder, decodeBatch(&m_view, batchIndex, map)).Times(1);
    presenter.notifyLoadBatchRequested(batchIndex);
  }

  void testWarningGivenCloseGUIWithUnsavedChanges() {
    auto presenter = makePresenter();
    auto const batchIndex = 1;
    expectWarnDiscardChanges(true);
    expectBatchUnsaved(batchIndex);
    expectAskDiscardChanges();
    presenter.isCloseEventPrevented();
  }

  void testBatchPresentersNotifySetRoundPrecisionOnOptionsChanged() {
    auto presenter = makePresenter();
    auto prec = 2;
    ON_CALL(*m_optionsPresenter, getIntOption(std::string("RoundPrecision"))).WillByDefault(ReturnRef(prec));
    expectRoundChecked(true);
    for (auto batchPresenter : m_batchPresenters) {
      EXPECT_CALL(*batchPresenter, notifySetRoundPrecision(prec));
    }
    presenter.notifyOptionsChanged();
  }

  void testBatchPresentersNotifyResetRoundPrecisionOnOptionsChanged() {
    auto presenter = makePresenter();
    expectRoundChecked(false);
    for (auto batchPresenter : m_batchPresenters) {
      EXPECT_CALL(*batchPresenter, notifyResetRoundPrecision());
    }
    presenter.notifyOptionsChanged();
  }

private:
  NiceMock<MockMainWindowView> m_view;
  NiceMock<MockMessageHandler> m_messageHandler;
  NiceMock<MockFileHandler> m_fileHandler;
  NiceMock<MockEncoder> *m_encoder;
  NiceMock<MockDecoder> *m_decoder;
  std::vector<IBatchView *> m_batchViews;
  std::vector<NiceMock<MockBatchPresenter> *> m_batchPresenters;
  NiceMock<MockBatchPresenterFactory> *m_makeBatchPresenter;
  NiceMock<MockSlitCalculator> *m_slitCalculator;

  class MainWindowPresenterFriend : public MainWindowPresenter {
    friend class MainWindowPresenterTest;

  public:
    MainWindowPresenterFriend(IMainWindowView *view, IReflMessageHandler *messageHandler, IFileHandler *fileHandler,
                              std::unique_ptr<IEncoder> encoder, std::unique_ptr<IDecoder> decoder,
                              std::unique_ptr<ISlitCalculator> slitCalculator,
                              std::unique_ptr<IOptionsDialogPresenter> optionsDialogPresenter,
                              std::unique_ptr<IBatchPresenterFactory> makeBatchPresenter)
        : MainWindowPresenter(view, messageHandler, fileHandler, std::move(encoder), std::move(decoder),
                              std::move(slitCalculator), std::move(optionsDialogPresenter),
                              std::move(makeBatchPresenter)) {}
  };

  // Create an OptionsDialogPresenter and cache the raw pointer to it in
  // m_optionsPresenter
  std::unique_ptr<NiceMock<MockOptionsDialogPresenter>> makeOptionsPresenter() {
    auto optionsPresenter = std::make_unique<NiceMock<MockOptionsDialogPresenter>>();
    m_optionsPresenter = optionsPresenter.get();
    return optionsPresenter;
  }

  std::unique_ptr<NiceMock<MockSlitCalculator>> makeSlitCalculator() {
    auto slitCalculator = std::make_unique<NiceMock<MockSlitCalculator>>();
    m_slitCalculator = slitCalculator.get();
    return slitCalculator;
  }

  std::unique_ptr<NiceMock<MockBatchPresenterFactory>> makeBatchPresenterFactory() {
    auto makeBatchPresenter = std::make_unique<NiceMock<MockBatchPresenterFactory>>();
    m_makeBatchPresenter = makeBatchPresenter.get();
    // Set up a mock batch presenter for each view to be returned from the
    // factory
    for (auto batchView : m_batchViews) {
      auto batchPresenter = new NiceMock<MockBatchPresenter>();
      ON_CALL(*batchPresenter, initInstrumentList(_)).WillByDefault(Return(DEFAULT_INSTRUMENT));
      m_batchPresenters.emplace_back(batchPresenter);
      ON_CALL(*m_makeBatchPresenter, makeProxy(batchView)).WillByDefault(Return(batchPresenter));
    }
    return makeBatchPresenter;
  }

  MainWindowPresenterFriend
  makePresenter(std::unique_ptr<NiceMock<MockOptionsDialogPresenter>> optionsPresenter = nullptr,
                std::unique_ptr<NiceMock<MockSlitCalculator>> slitCalculator = nullptr,
                std::unique_ptr<NiceMock<MockBatchPresenterFactory>> makeBatchPresenter = nullptr) {
    if (!optionsPresenter) {
      optionsPresenter = makeOptionsPresenter();
    }

    auto encoder = std::make_unique<NiceMock<MockEncoder>>();
    m_encoder = encoder.get();
    auto decoder = std::make_unique<NiceMock<MockDecoder>>();
    m_decoder = decoder.get();

    if (!slitCalculator) {
      slitCalculator = makeSlitCalculator();
    }

    if (!makeBatchPresenter) {
      makeBatchPresenter = makeBatchPresenterFactory();
    }

    // Make the presenter
    auto presenter = MainWindowPresenterFriend(&m_view, &m_messageHandler, &m_fileHandler, std::move(encoder),
                                               std::move(decoder), std::move(slitCalculator),
                                               std::move(optionsPresenter), std::move(makeBatchPresenter));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_messageHandler));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_optionsPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_fileHandler));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_encoder));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_decoder));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_slitCalculator));
    for (auto batchPresenter : m_batchPresenters)
      TS_ASSERT(Mock::VerifyAndClearExpectations(batchPresenter));
    m_batchPresenters.clear();
  }

  void expectBatchAdded(MockBatchPresenter *batchPresenter, std::string const &instrumentName) {
    EXPECT_CALL(*batchPresenter, acceptMainPresenter(_)).Times(1);
    EXPECT_CALL(*batchPresenter, initInstrumentList(_)).Times(1).WillOnce(Return(instrumentName));
    EXPECT_CALL(*batchPresenter, notifyInstrumentChanged(instrumentName)).Times(1);
    EXPECT_CALL(*batchPresenter, notifyReductionPaused()).Times(1);
    EXPECT_CALL(*batchPresenter, notifyAnyBatchAutoreductionPaused()).Times(1);
  }

  void expectBatchCanBeClosed(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_batchPresenters[batchIndex], requestClose()).Times(1).WillOnce(Return(true));
  }

  void expectBatchIsAutoreducing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing()).Times(1).WillOnce(Return(true));
  }

  void expectBatchIsProcessing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing()).Times(1).WillOnce(Return(true));
  }

  void expectBatchIsNotAutoreducing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isAutoreducing()).Times(1).WillOnce(Return(false));
  }

  void expectBatchSaved(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isBatchUnsaved()).Times(1).WillOnce(Return(false));
  }

  void expectBatchUnsaved(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isBatchUnsaved()).Times(1).WillOnce(Return(true));
  }

  void expectBatchIsNotProcessing(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], isProcessing()).Times(1).WillOnce(Return(false));
  }

  void expectRequestCloseBatchFailed(int batchIndex) {
    EXPECT_CALL(*m_batchPresenters[batchIndex], requestClose()).Times(1).WillOnce(Return(false));
  }

  void expectBatchRemovedFromView(int batchIndex) { EXPECT_CALL(m_view, removeBatch(batchIndex)).Times(1); }

  void expectBatchNotRemovedFromView(int batchIndex) { EXPECT_CALL(m_view, removeBatch(batchIndex)).Times(0); }

  void expectCannotCloseBatchWarning() {
    EXPECT_CALL(m_messageHandler, giveUserCritical("Cannot close batch while processing or "
                                                   "autoprocessing is in progress",
                                                   "Error"))
        .Times(1);
  }

  void expectWarnDiscardChanges(bool setting) {
    EXPECT_CALL(*m_optionsPresenter, getBoolOption(std::string("WarnDiscardChanges")))
        .Times(1)
        .WillOnce(Return(setting));
  }

  void expectRoundChecked(bool setting) {
    EXPECT_CALL(*m_optionsPresenter, getBoolOption(std::string("Round")))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(setting));
  }

  void expectSlitCalculatorInstrumentUpdated(std::string const &instrument) {
    EXPECT_CALL(*m_slitCalculator, setCurrentInstrumentName(instrument)).Times(1);
    EXPECT_CALL(*m_slitCalculator, processInstrumentHasBeenChanged()).Times(1);
  }

  void expectSlitCalculatorInstrumentNotUpdated() {
    EXPECT_CALL(*m_slitCalculator, setCurrentInstrumentName(_)).Times(0);
    EXPECT_CALL(*m_slitCalculator, processInstrumentHasBeenChanged()).Times(0);
  }

  void expectBatchIsSavedToFile(int batchIndex) {
    auto const filename = std::string("test.json");
    auto const map = QMap<QString, QVariant>();
    EXPECT_CALL(m_messageHandler, askUserForSaveFileName("JSON (*.json)")).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(*m_encoder, encodeBatch(&m_view, batchIndex, false)).Times(1).WillOnce(Return(map));
    EXPECT_CALL(m_fileHandler, saveJSONToFile(filename, map)).Times(1);
  }

  void expectBatchIsNotSavedToInvalidFile(int batchIndex) {
    auto const filename = std::string("/test.json");
    auto const map = QMap<QString, QVariant>();
    EXPECT_CALL(m_messageHandler, askUserForSaveFileName("JSON (*.json)")).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(*m_encoder, encodeBatch(&m_view, batchIndex, false)).Times(1).WillOnce(Return(map));
    EXPECT_CALL(m_fileHandler, saveJSONToFile(filename, map))
        .Times(1)
        .WillOnce(Throw(std::invalid_argument("Test error")));
    EXPECT_CALL(
        m_messageHandler,
        giveUserCritical(
            "Invalid path provided. Check you have the correct permissions for this save location. \nTest error",
            "Save Batch"))
        .Times(1);
  }

  void expectBatchIsNotSavedWhenSaveFails(int batchIndex) {
    auto const filename = std::string("/test.json");
    auto const map = QMap<QString, QVariant>();
    EXPECT_CALL(m_messageHandler, askUserForSaveFileName("JSON (*.json)")).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(*m_encoder, encodeBatch(&m_view, batchIndex, false)).Times(1).WillOnce(Return(map));
    EXPECT_CALL(m_fileHandler, saveJSONToFile(filename, map))
        .Times(1)
        .WillOnce(Throw(std::runtime_error("Test error, save failed.")));
    EXPECT_CALL(
        m_messageHandler,
        giveUserCritical("An error occurred while saving. Please try again. \nTest error, save failed.", "Save Batch"))
        .Times(1);
  }

  void expectBatchIsLoadedFromFile(int batchIndex) {
    auto const filename = std::string("test.json");
    auto const map = QMap<QString, QVariant>();
    EXPECT_CALL(m_messageHandler, askUserForLoadFileName("JSON (*.json)")).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(m_fileHandler, loadJSONFromFile(filename)).Times(1).WillOnce(Return(map));
    EXPECT_CALL(*m_decoder, decodeBatch(&m_view, batchIndex, map)).Times(1);
  }

  void expectAskDiscardChanges() {
    EXPECT_CALL(m_messageHandler,
                askUserOkCancel("This will cause unsaved changes to be lost. Continue?", "Discard changes?"))
        .Times(1);
  }

  void expectDoNotAskDiscardChanges() { EXPECT_CALL(m_messageHandler, askUserOkCancel(_, _)).Times(0); }

  void expectUserDiscardsChanges() {
    EXPECT_CALL(m_messageHandler, askUserOkCancel(_, _)).Times(1).WillOnce(Return(true));
  }

  void expectUserDoesNotDiscardChanges() {
    EXPECT_CALL(m_messageHandler, askUserOkCancel(_, _)).Times(1).WillOnce(Return(false));
  }

  void assertFirstBatchWasRemovedFromModel(MainWindowPresenterFriend const &presenter) {
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(), 1);
    // Note that our local list of raw pointers is not updated so
    // the first item is invalid and the second item is now the
    // only remaining batch presenter in the model
    TS_ASSERT_EQUALS(presenter.m_batchPresenters[0].get(), m_batchPresenters[1]);
  }

  void assertBatchNotRemovedFromModel(MainWindowPresenterFriend const &presenter) {
    TS_ASSERT_EQUALS(presenter.m_batchPresenters.size(), m_batchPresenters.size());
    for (size_t index = 0; index < m_batchPresenters.size(); ++index)
      TS_ASSERT_EQUALS(presenter.m_batchPresenters[index].get(), m_batchPresenters[index]);
  }

private:
  std::string backup_facility;
  std::string backup_instrument;
  NiceMock<MockOptionsDialogPresenter> *m_optionsPresenter;
};
