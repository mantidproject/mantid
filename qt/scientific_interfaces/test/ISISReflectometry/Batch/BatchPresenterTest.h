// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_BATCHPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_BATCHPRESENTERTEST_H_

#include "../../../ISISReflectometry/GUI/Batch/BatchPresenter.h"
#include "../ReflMockObjects.h"
#include "MockBatchView.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace MantidQt::CustomInterfaces;
using MantidQt::API::IConfiguredAlgorithm_sptr;
using testing::AtLeast;
using testing::Mock;
using testing::NiceMock;
using testing::Return;
using testing::_;

class BatchPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BatchPresenterTest *createSuite() { return new BatchPresenterTest(); }
  static void destroySuite(BatchPresenterTest *suite) { delete suite; }

  BatchPresenterTest()
      : m_view(),
        m_jobRunner(nullptr), m_instruments{"INTER", "OFFSPEC", "POLREF",
                                            "SURF", "CRISP"},
        m_tolerance(0.1),
        m_experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                     SummationType::SumInLambda, false, false,
                     PolarizationCorrections(PolarizationCorrectionType::None),
                     FloodCorrections(FloodCorrectionType::Workspace),
                     boost::none, std::map<std::string, std::string>(),
                     std::vector<PerThetaDefaults>()),
        m_instrument(
            RangeInLambda(0.0, 0.0),
            MonitorCorrections(0, true, RangeInLambda(0.0, 0.0),
                               RangeInLambda(0.0, 0.0)),
            DetectorCorrections(false, DetectorCorrectionType::VerticalShift)),
        m_runsTable(m_instruments, 0.1, ReductionJobs()),
        m_slicing(), m_mockAlgorithmsList{
                         boost::make_shared<MockBatchJobAlgorithm>()} {}

  void testPresenterSubscribesToView() {
    EXPECT_CALL(m_view, subscribe(_)).Times(1);
    auto presenter = makePresenter();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenInstrumentChanged() {
    auto presenter = makePresenter();
    auto const instrument = std::string("POLREF");
    EXPECT_CALL(*m_runsPresenter, instrumentChanged(instrument)).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, instrumentChanged(instrument)).Times(1);
    presenter.notifyInstrumentChanged(instrument);
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenSettingsChanged() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, settingsChanged()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

  void testModelUpdatedWhenReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, reductionResumed()).Times(1);
    presenter.notifyReductionResumed();
    verifyAndClear();
  }

  void testBatchIsExecutedWhenReductionResumed() {
    auto presenter = makePresenter();
    expectBatchIsExecuted();
    presenter.notifyReductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenReductionResumed() {
    auto presenter = makePresenter();
    expectReductionResumed();
    presenter.notifyReductionResumed();
    verifyAndClear();
  }

  void testBatchIsCancelledWhenReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, cancelAlgorithmQueue()).Times(1);
    presenter.notifyReductionPaused();
    verifyAndClear();
  }

  void testModelUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, reductionPaused()).Times(1);
    presenter.notifyBatchCancelled();
    verifyAndClear();
  }

  void testRowStateUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter.notifyBatchCancelled();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenBatchCancelled() {
    auto presenter = makePresenter();
    expectReductionPaused();
    expectAutoreductionPaused();
    presenter.notifyBatchCancelled();
    verifyAndClear();
  }

  void testModelUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, autoreductionResumed()).Times(1);
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testBatchIsExecutedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    expectBatchIsExecuted();
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    expectAutoreductionResumed();
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testModelUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, autoreductionPaused()).Times(1);
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testBatchIsCancelledWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(m_view, cancelAlgorithmQueue()).Times(1);
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    expectAutoreductionPaused();
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testAutoreductionComplete() {
    // TODO Add expectations here when autoreduction is implemented
    auto presenter = makePresenter();
    presenter.notifyAutoreductionCompleted();
    verifyAndClear();
  }

  void testNextBatchIsStartedWhenBatchFinished() {
    auto presenter = makePresenter();
    expectBatchIsExecuted();
    presenter.notifyBatchComplete(false);
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenBatchFinishedAndNothingLeftToProcess() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, getAlgorithms())
        .Times(1)
        .WillOnce(Return(std::deque<IConfiguredAlgorithm_sptr>()));
    expectReductionPaused();
    presenter.notifyBatchComplete(false);
    verifyAndClear();
  }

  void testRowStateUpdatedWhenBatchFinished() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter.notifyBatchComplete(false);
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter();
    auto name = std::string("test_workspace");
    EXPECT_CALL(*m_jobRunner, notifyWorkspaceDeleted(name)).Times(1);
    presenter.postDeleteHandle(name);
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspaceDeleted() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter.postDeleteHandle("");
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter();
    auto oldName = std::string("test_workspace1");
    auto newName = std::string("test_workspace2");
    EXPECT_CALL(*m_jobRunner, notifyWorkspaceRenamed(oldName, newName))
        .Times(1);
    presenter.renameHandle(oldName, newName);
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspaceRenamed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter.renameHandle("", "");
    verifyAndClear();
  }

  void testModelUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_jobRunner, notifyAllWorkspacesDeleted()).Times(1);
    presenter.clearADSHandle();
    verifyAndClear();
  }

  void testRowStateUpdatedWhenWorkspacesCleared() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, notifyRowStateChanged()).Times(1);
    presenter.clearADSHandle();
    verifyAndClear();
  }

private:
  NiceMock<MockBatchView> m_view;
  NiceMock<MockBatchJobRunner> *m_jobRunner;
  NiceMock<MockRunsPresenter> *m_runsPresenter;
  NiceMock<MockEventPresenter> *m_eventPresenter;
  NiceMock<MockExperimentPresenter> *m_experimentPresenter;
  NiceMock<MockInstrumentPresenter> *m_instrumentPresenter;
  NiceMock<MockSavePresenter> *m_savePresenter;
  std::vector<std::string> m_instruments;
  double m_tolerance;
  Experiment m_experiment;
  Instrument m_instrument;
  RunsTable m_runsTable;
  Slicing m_slicing;
  std::deque<IConfiguredAlgorithm_sptr> m_mockAlgorithmsList;

  class BatchPresenterFriend : public BatchPresenter {
    friend class BatchPresenterTest;

  public:
    BatchPresenterFriend(
        IBatchView *view, Batch model,
        std::unique_ptr<IRunsPresenter> runsPresenter,
        std::unique_ptr<IEventPresenter> eventPresenter,
        std::unique_ptr<IExperimentPresenter> experimentPresenter,
        std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
        std::unique_ptr<ISavePresenter> savePresenter)
        : BatchPresenter(
              view, std::move(model), std::move(runsPresenter),
              std::move(eventPresenter), std::move(experimentPresenter),
              std::move(instrumentPresenter), std::move(savePresenter)) {}
  };

  RunsTable makeRunsTable() {
    return RunsTable(m_instruments, m_tolerance, ReductionJobs());
  }

  Batch makeModel() {
    Batch batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    return batch;
  }

  BatchPresenterFriend makePresenter() {
    // Create pointers to the child presenters and pass them into the batch
    auto runsPresenter =
        Mantid::Kernel::make_unique<NiceMock<MockRunsPresenter>>();
    auto eventPresenter =
        Mantid::Kernel::make_unique<NiceMock<MockEventPresenter>>();
    auto experimentPresenter =
        Mantid::Kernel::make_unique<NiceMock<MockExperimentPresenter>>();
    auto instrumentPresenter =
        Mantid::Kernel::make_unique<NiceMock<MockInstrumentPresenter>>();
    auto savePresenter =
        Mantid::Kernel::make_unique<NiceMock<MockSavePresenter>>();
    m_runsPresenter = runsPresenter.get();
    m_eventPresenter = eventPresenter.get();
    m_experimentPresenter = experimentPresenter.get();
    m_instrumentPresenter = instrumentPresenter.get();
    m_savePresenter = savePresenter.get();
    // Create the batch presenter
    auto presenter = BatchPresenterFriend(
        &m_view, makeModel(), std::move(runsPresenter),
        std::move(eventPresenter), std::move(experimentPresenter),
        std::move(instrumentPresenter), std::move(savePresenter));
    // Replace the constructed job runner with a mock
    m_jobRunner = new NiceMock<MockBatchJobRunner>();
    presenter.m_jobRunner.reset(m_jobRunner);
    ON_CALL(*m_jobRunner, getAlgorithms())
        .WillByDefault(Return(m_mockAlgorithmsList));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runsPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_eventPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_experimentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_instrumentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_savePresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_jobRunner));
  }

  void expectReductionResumed() {
    EXPECT_CALL(*m_savePresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, reductionResumed()).Times(1);
  }

  void expectReductionPaused() {
    EXPECT_CALL(*m_savePresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, reductionPaused()).Times(1);
  }

  void expectAutoreductionResumed() {
    EXPECT_CALL(*m_savePresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, autoreductionResumed()).Times(1);
  }

  void expectAutoreductionPaused() {
    EXPECT_CALL(*m_savePresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, autoreductionPaused()).Times(1);
  }

  void expectBatchIsExecuted() {
    EXPECT_CALL(*m_jobRunner, getAlgorithms()).Times(1);
    EXPECT_CALL(m_view, clearAlgorithmQueue()).Times(1);
    EXPECT_CALL(m_view, setAlgorithmQueue(m_mockAlgorithmsList)).Times(1);
    EXPECT_CALL(m_view, executeAlgorithmQueue()).Times(1);
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHPRESENTERTEST_H_
