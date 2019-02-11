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
      : m_view(), m_instruments{"INTER", "OFFSPEC", "POLREF", "SURF", "CRISP"},
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
        m_runsTable(m_instruments, 0.1, ReductionJobs()), m_slicing() {}

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

  void testChildPresentersUpdatedWhenReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_savePresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, reductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, reductionResumed()).Times(1);
    presenter.notifyReductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenReductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_savePresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, reductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, reductionPaused()).Times(1);
    presenter.notifyReductionPaused();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAutoreductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_savePresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_eventPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, autoreductionResumed()).Times(1);
    EXPECT_CALL(*m_runsPresenter, autoreductionResumed()).Times(1);
    presenter.notifyAutoreductionResumed();
    verifyAndClear();
  }

  void testChildPresentersUpdatedWhenAutoreductionPaused() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_savePresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_eventPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_experimentPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_instrumentPresenter, autoreductionPaused()).Times(1);
    EXPECT_CALL(*m_runsPresenter, autoreductionPaused()).Times(1);
    presenter.notifyAutoreductionPaused();
    verifyAndClear();
  }

  void testWhenReductionResumed() {
    auto presenter = makePresenter();
    EXPECT_CALL(*m_runsPresenter, settingsChanged()).Times(1);
    presenter.notifySettingsChanged();
    verifyAndClear();
  }

private:
  NiceMock<MockBatchView> m_view;
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

  Experiment makeExperiment() {
    auto polarizationCorrections =
        PolarizationCorrections(PolarizationCorrectionType::None);
    auto floodCorrections = FloodCorrections(FloodCorrectionType::Workspace);
    auto transmissionRunRange = boost::none;
    auto stitchParameters = std::map<std::string, std::string>();
    auto perThetaDefaults = std::vector<PerThetaDefaults>();
    return Experiment(AnalysisMode::PointDetector, ReductionType::Normal,
                      SummationType::SumInLambda, false, false,
                      std::move(polarizationCorrections),
                      std::move(floodCorrections),
                      std::move(transmissionRunRange),
                      std::move(stitchParameters), std::move(perThetaDefaults));
  }

  Instrument makeInstrument() {
    auto wavelengthRange = RangeInLambda(0.0, 0.0);
    auto monitorCorrections = MonitorCorrections(
        0, true, RangeInLambda(0.0, 0.0), RangeInLambda(0.0, 0.0));
    auto detectorCorrections =
        DetectorCorrections(false, DetectorCorrectionType::VerticalShift);
    return Instrument(wavelengthRange, monitorCorrections, detectorCorrections);
  }

  RunsTable makeRunsTable() {
    return RunsTable(m_instruments, m_tolerance, ReductionJobs());
  }

  Batch makeModel() {
    m_experiment = makeExperiment();
    m_instrument = makeInstrument();
    m_runsTable = makeRunsTable();
    m_slicing = Slicing();
    Batch batch(m_experiment, m_instrument, m_runsTable, m_slicing);
    return batch;
  }

  BatchPresenterFriend makePresenter() {
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
    auto presenter = BatchPresenterFriend(
        &m_view, makeModel(), std::move(runsPresenter),
        std::move(eventPresenter), std::move(experimentPresenter),
        std::move(instrumentPresenter), std::move(savePresenter));
    return presenter;
  }

  void verifyAndClear() {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_runsPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_eventPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_experimentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_instrumentPresenter));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_savePresenter));
  }
};

#endif // MANTID_CUSTOMINTERFACES_BATCHPRESENTERTEST_H_
