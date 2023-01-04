// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAnalysisMocks.h"
#include "ALFInstrumentMocks.h"
#include "ALFInstrumentPresenter.h"
#include "DetectorTube.h"
#include "MockInstrumentActor.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/InstrumentVisitor.h"
#include "MantidKernel/WarningSuppressions.h"

#include <memory>
#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace testing;
using namespace MantidQt::MantidWidgets;
using namespace Mantid::Geometry;

namespace {

GNU_DIAG_OFF_SUGGEST_OVERRIDE

MATCHER(ComponentNotNull, "Check component is not null") { return bool(arg); }

GNU_DIAG_ON_SUGGEST_OVERRIDE

} // namespace

class ALFInstrumentPresenterTest : public CxxTest::TestSuite {
public:
  ALFInstrumentPresenterTest() { FrameworkManager::Instance(); }

  static ALFInstrumentPresenterTest *createSuite() { return new ALFInstrumentPresenterTest(); }

  static void destroySuite(ALFInstrumentPresenterTest *suite) { delete suite; }

  void setUp() override {
    auto model = std::make_unique<NiceMock<MockALFInstrumentModel>>();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockALFInstrumentView>>();
    m_presenter = std::make_unique<ALFInstrumentPresenter>(m_view.get(), std::move(model));

    m_instrumentActor = std::make_unique<NiceMock<MockInstrumentActor>>();

    m_analysisPresenter = std::make_unique<NiceMock<MockALFAnalysisPresenter>>();
    m_presenter->subscribeAnalysisPresenter(m_analysisPresenter.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_analysisPresenter));

    m_presenter.reset();
    m_view.reset();
  }

  void test_instantiating_the_presenter_will_set_up_the_instrument() {
    auto model = std::make_unique<NiceMock<MockALFInstrumentModel>>();
    auto view = std::make_unique<NiceMock<MockALFInstrumentView>>();

    EXPECT_CALL(*model, loadedWsName()).Times(1).WillOnce(Return("ALFData"));
    EXPECT_CALL(*view, setUpInstrument("ALFData")).Times(1);

    auto modelRaw = model.get();
    auto presenter = std::make_unique<ALFInstrumentPresenter>(view.get(), std::move(model));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&modelRaw));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
  }

  void test_getSampleLoadWidget_gets_the_load_widget_from_the_view() {
    EXPECT_CALL(*m_view, generateSampleLoadWidget()).Times(1);
    m_presenter->getSampleLoadWidget();
  }

  void test_getInstrumentView_gets_the_instrument_view_widget_from_the_view() {
    EXPECT_CALL(*m_view, getInstrumentView()).Times(1);
    m_presenter->getInstrumentView();
  }

  void test_loadSample_will_not_attempt_a_load_when_an_empty_filepath_is_provided() {
    EXPECT_CALL(*m_view, getSampleFile()).Times(1).WillOnce(Return(std::nullopt));

    // Expect no calls to these methods
    EXPECT_CALL(*m_analysisPresenter, clear()).Times(0);
    EXPECT_CALL(*m_model, loadAndNormalise(_)).Times(0);

    m_presenter->loadSample();
  }

  void test_loadSample_will_not_show_a_warning_when_loading_is_successful() {
    std::size_t const run(82301u);
    std::string const filename("ALF82301");

    EXPECT_CALL(*m_view, getSampleFile()).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(*m_analysisPresenter, clear()).Times(1);
    EXPECT_CALL(*m_model, loadAndNormalise(filename)).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_model, setSample(_)).Times(1);

    // Expect no call to warningBox
    EXPECT_CALL(*m_view, warningBox(_)).Times(0);

    EXPECT_CALL(*m_model, sampleRun()).Times(1).WillOnce(Return(run));
    EXPECT_CALL(*m_view, setSampleRun(std::to_string(run))).Times(1);

    EXPECT_CALL(*m_model, generateLoadedWorkspace()).Times(1);

    m_presenter->loadSample();
  }

  void test_notifyInstrumentActorReset_generates_an_angle_workspace_and_notifies_the_analysis_presenter() {
    expectUpdateAnalysisViewFromModel();
    m_presenter->notifyInstrumentActorReset();
  }

  void test_notifyShapeChanged_generates_an_angle_workspace_and_notifies_the_analysis_presenter() {
    auto const detectors = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};

    EXPECT_CALL(*m_view, getSelectedDetectors()).Times(1).WillOnce(Return(detectors));
    EXPECT_CALL(*m_model, setSelectedTubes(detectors)).Times(1).WillOnce(Return(true));

    expectUpdateInstrumentViewFromModel(detectors);
    expectUpdateAnalysisViewFromModel();

    m_presenter->notifyShapeChanged();
  }

  void test_notifyShapeChanged_does_not_update_views_if_detectors_are_not_set() {
    auto const detectors = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};

    EXPECT_CALL(*m_view, getSelectedDetectors()).Times(1).WillOnce(Return(detectors));
    EXPECT_CALL(*m_model, setSelectedTubes(detectors)).Times(1).WillOnce(Return(false));

    expectUpdateInstrumentViewFromModelNotCalled();
    expectUpdateAnalysisViewFromModelNotCalled();

    m_presenter->notifyShapeChanged();
  }

  void test_notifyTubesSelected_generates_an_angle_workspace_and_notifies_the_analysis_presenter() {
    auto const detectors = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};

    EXPECT_CALL(*m_model, addSelectedTube(detectors.front())).Times(1).WillOnce(Return(true));

    expectUpdateInstrumentViewFromModel(detectors);
    expectUpdateAnalysisViewFromModel();

    m_presenter->notifyTubesSelected(detectors);
  }

  void test_notifyTubesSelected_does_not_update_views_if_tube_is_not_added() {
    auto const detectors = std::vector<DetectorTube>{{2500u, 2501u, 2502u}};

    EXPECT_CALL(*m_model, addSelectedTube(detectors.front())).Times(1).WillOnce(Return(false));

    expectUpdateInstrumentViewFromModelNotCalled();
    expectUpdateAnalysisViewFromModelNotCalled();

    m_presenter->notifyTubesSelected(detectors);
  }

private:
  void expectUpdateInstrumentViewFromModel(std::vector<DetectorTube> const &tubes) {
    EXPECT_CALL(*m_view, clearShapes()).Times(1);
    EXPECT_CALL(*m_model, selectedTubes()).Times(1).WillOnce(Return(tubes));
    EXPECT_CALL(*m_view, drawRectanglesAbove(tubes)).Times(1);
  }

  void expectUpdateInstrumentViewFromModelNotCalled() {
    EXPECT_CALL(*m_view, clearShapes()).Times(0);
    EXPECT_CALL(*m_model, selectedTubes()).Times(0);
    EXPECT_CALL(*m_view, drawRectanglesAbove(_)).Times(0);
  }

  void expectUpdateAnalysisViewFromModel() {
    MatrixWorkspace_sptr const expectedExtractedWorkspace = nullptr;
    auto const expectedTwoTheta = std::vector<double>{1.1, 2.2};
    std::tuple<MatrixWorkspace_sptr, std::vector<double>> const expectedReturn = {expectedExtractedWorkspace,
                                                                                  expectedTwoTheta};

    EXPECT_CALL(*m_view, getInstrumentActor()).Times(1).WillOnce(ReturnRef(*m_instrumentActor));
    EXPECT_CALL(*m_model, generateOutOfPlaneAngleWorkspace(_)).Times(1).WillOnce(Return(expectedReturn));

    EXPECT_CALL(*m_analysisPresenter, setExtractedWorkspace(expectedExtractedWorkspace, expectedTwoTheta)).Times(1);
  }

  void expectUpdateAnalysisViewFromModelNotCalled() {
    EXPECT_CALL(*m_view, getInstrumentActor()).Times(0);
    EXPECT_CALL(*m_model, generateOutOfPlaneAngleWorkspace(_)).Times(0);
    EXPECT_CALL(*m_analysisPresenter, setExtractedWorkspace(_, _)).Times(0);
  }

  NiceMock<MockALFInstrumentModel> *m_model;
  std::unique_ptr<NiceMock<MockALFInstrumentView>> m_view;
  std::unique_ptr<ALFInstrumentPresenter> m_presenter;
  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<NiceMock<MockALFAnalysisPresenter>> m_analysisPresenter;
};
