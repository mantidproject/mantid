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
#include "MockALFAlgorithmManager.h"
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
    m_algProperties = std::make_unique<AlgorithmRuntimeProps>();

    auto algorithmManager = std::make_unique<NiceMock<MockALFAlgorithmManager>>();
    auto model = std::make_unique<NiceMock<MockALFInstrumentModel>>();

    m_algorithmManager = algorithmManager.get();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockALFInstrumentView>>();
    m_presenter = std::make_unique<ALFInstrumentPresenter>(m_view.get(), std::move(model), std::move(algorithmManager));

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
    auto algorithmManager = std::make_unique<NiceMock<MockALFAlgorithmManager>>();
    auto model = std::make_unique<NiceMock<MockALFInstrumentModel>>();
    auto view = std::make_unique<NiceMock<MockALFInstrumentView>>();

    EXPECT_CALL(*view, subscribePresenter(_)).Times(1);
    EXPECT_CALL(*model, loadedWsName()).Times(1).WillOnce(Return("ALFData"));
    EXPECT_CALL(*view, setUpInstrument("ALFData")).Times(1);
    EXPECT_CALL(*algorithmManager, subscribe(_)).Times(1);

    auto algorithmManagerRaw = algorithmManager.get();
    auto modelRaw = model.get();
    auto presenter =
        std::make_unique<ALFInstrumentPresenter>(view.get(), std::move(model), std::move(algorithmManager));

    TS_ASSERT(Mock::VerifyAndClearExpectations(&algorithmManagerRaw));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&modelRaw));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&view));
  }

  void test_getSampleLoadWidget_gets_the_sample_load_widget_from_the_view() {
    EXPECT_CALL(*m_view, generateSampleLoadWidget()).Times(1);
    m_presenter->getSampleLoadWidget();
  }

  void test_getVanadiumLoadWidget_gets_the_vanadium_load_widget_from_the_view() {
    EXPECT_CALL(*m_view, generateVanadiumLoadWidget()).Times(1);
    m_presenter->getVanadiumLoadWidget();
  }

  void test_getInstrumentView_gets_the_instrument_view_widget_from_the_view() {
    EXPECT_CALL(*m_view, getInstrumentView()).Times(1);
    m_presenter->getInstrumentView();
  }

  void test_loadSettings_will_load_the_settings_in_the_view() {
    EXPECT_CALL(*m_view, loadSettings()).Times(1);
    m_presenter->loadSettings();
  }

  void test_saveSettings_will_save_the_settings_in_the_view() {
    EXPECT_CALL(*m_view, saveSettings()).Times(1);
    m_presenter->saveSettings();
  }

  void test_loadSample_will_not_attempt_a_load_when_an_empty_filepath_is_provided() {
    EXPECT_CALL(*m_view, disable("Loading sample")).Times(1);
    EXPECT_CALL(*m_analysisPresenter, clear()).Times(1);

    EXPECT_CALL(*m_view, getSampleFile()).Times(1).WillOnce(Return(std::nullopt));
    EXPECT_CALL(*m_model, setData(ALFData::SAMPLE, Eq(nullptr))).Times(1);
    expectGenerateLoadedWorkspace();

    // Expect no calls to these methods
    EXPECT_CALL(*m_model, loadProperties(_)).Times(0);
    EXPECT_CALL(*m_algorithmManager, load(_)).Times(0);

    m_presenter->loadSample();
  }

  void test_loadSample_will_not_show_a_warning_when_loading_is_successful() {
    std::string const filename("ALF82301");

    EXPECT_CALL(*m_view, getSampleFile()).Times(1).WillOnce(Return(filename));
    EXPECT_CALL(*m_view, disable("Loading sample")).Times(1);
    EXPECT_CALL(*m_analysisPresenter, clear()).Times(1);
    EXPECT_CALL(*m_model, loadProperties(filename)).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, load(_)).Times(1);

    m_presenter->loadSample();
  }

  void test_notifyLoadComplete_opens_a_warning_if_the_data_is_not_ALF_data() {
    EXPECT_CALL(*m_model, isALFData(_)).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_view, enable()).Times(1);
    EXPECT_CALL(*m_view, displayWarning("The loaded data is not from the ALF instrument")).Times(1);

    m_presenter->notifyLoadComplete(nullptr);
  }

  void test_notifyLoadComplete_normalises_the_data_if_its_ALF_data() {
    EXPECT_CALL(*m_model, isALFData(_)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_model, normaliseByCurrentProperties(_))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, normaliseByCurrent(NotNull())).Times(1);

    // Expect no call to displayWarning
    EXPECT_CALL(*m_view, displayWarning(_)).Times(0);

    m_presenter->notifyLoadComplete(nullptr);
  }

  void test_notifyNormaliseByCurrentComplete_will_update_the_run_in_the_view() {
    EXPECT_CALL(*m_model, setData(ALFData::SAMPLE, _)).Times(1);
    EXPECT_CALL(*m_model, run(ALFData::SAMPLE)).Times(1).WillOnce(Return(35321u));
    EXPECT_CALL(*m_view, setSampleRun("35321")).Times(1);
    expectGenerateLoadedWorkspace();

    m_presenter->notifyNormaliseByCurrentComplete(nullptr);
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

  void test_notifyRebinToWorkspaceComplete_will_normalise_the_sample_by_the_vanadium() {
    EXPECT_CALL(*m_model, setData(ALFData::VANADIUM, _)).Times(1);
    expectNormaliseSampleByVanadium();

    m_presenter->notifyRebinToWorkspaceComplete(nullptr);
  }

  void test_notifyDivideComplete_will_replace_special_values() {
    EXPECT_CALL(*m_model, replaceSpecialValuesProperties(_))
        .Times(1)
        .WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, replaceSpecialValues(NotNull())).Times(1);

    m_presenter->notifyDivideComplete(nullptr);
  }

  void test_notifyReplaceSpecialValuesComplete_converts_the_sample_to_dSpacing() {
    expectConvertSampleToDSpacing();
    m_presenter->notifyReplaceSpecialValuesComplete(nullptr);
  }

  void test_notifyConvertUnitsComplete_adds_the_workspace_to_the_ADS() {
    EXPECT_CALL(*m_model, replaceSampleWorkspaceInADS(_)).Times(1);
    EXPECT_CALL(*m_view, enable()).Times(1);
    m_presenter->notifyConvertUnitsComplete(nullptr);
  }

  void test_notifyCreateWorkspaceComplete_calls_the_rebunch_algorithm() {
    EXPECT_CALL(*m_model, scaleXProperties(_)).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, scaleX(NotNull())).Times(1);

    m_presenter->notifyCreateWorkspaceComplete(nullptr);
  }

  void test_notifyScaleXComplete_calls_the_rebunch_algorithm() {
    EXPECT_CALL(*m_model, rebunchProperties(_)).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, rebunch(NotNull())).Times(1);

    m_presenter->notifyScaleXComplete(nullptr);
  }

  void test_notifyRebunchComplete_will_set_the_two_thetas_in_the_analysis_presenter() {
    std::vector<double> twoThetas{1.0, 2.0};
    EXPECT_CALL(*m_model, twoThetasClosestToZero()).Times(1).WillOnce(Return(twoThetas));
    EXPECT_CALL(*m_analysisPresenter, setExtractedWorkspace(_, twoThetas)).Times(1);

    m_presenter->notifyRebunchComplete(nullptr);
  }

  void test_notifyAlgorithmError_will_display_a_message_in_the_view() {
    std::string const message("This is a warning message");

    EXPECT_CALL(*m_view, enable()).Times(1);
    EXPECT_CALL(*m_view, displayWarning(message)).Times(1);

    m_presenter->notifyAlgorithmError(message);
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

  void expectGenerateLoadedWorkspace() {
    EXPECT_CALL(*m_model, hasData(_)).Times(1).WillOnce(Return(true));

    EXPECT_CALL(*m_model, binningMismatch()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_model, rebinToWorkspaceProperties()).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, rebinToWorkspace(_)).Times(1);
  }

  void expectConvertSampleToDSpacing() {
    EXPECT_CALL(*m_model, axisIsDSpacing()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_model, convertUnitsProperties(_)).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, convertUnits(_)).Times(1);
  }

  void expectNormaliseSampleByVanadium() {
    EXPECT_CALL(*m_model, hasData(ALFData::VANADIUM)).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_model, divideProperties()).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, divide(_)).Times(1);
  }

  void expectUpdateAnalysisViewFromModel(bool hasTubes = true) {
    EXPECT_CALL(*m_model, hasSelectedTubes()).Times(1).WillOnce(Return(hasTubes));

    if (hasTubes) {
      EXPECT_CALL(*m_view, getInstrumentActor()).Times(1).WillOnce(ReturnRef(*m_instrumentActor));
      EXPECT_CALL(*m_model, createWorkspaceAlgorithmProperties(_)).WillOnce(Return(ByMove(std::move(m_algProperties))));
      EXPECT_CALL(*m_algorithmManager, createWorkspace(NotNull())).Times(1);
    } else {
      EXPECT_CALL(*m_analysisPresenter, setExtractedWorkspace(IsNull(), std::vector<double>{})).Times(1);
    }
  }

  void expectUpdateAnalysisViewFromModelNotCalled() {
    EXPECT_CALL(*m_view, getInstrumentActor()).Times(0);
    EXPECT_CALL(*m_model, createWorkspaceAlgorithmProperties(_)).Times(0);
    EXPECT_CALL(*m_algorithmManager, createWorkspace(_)).Times(0);
  }

  std::unique_ptr<AlgorithmRuntimeProps> m_algProperties;

  NiceMock<MockALFAlgorithmManager> *m_algorithmManager;
  NiceMock<MockALFInstrumentModel> *m_model;
  std::unique_ptr<NiceMock<MockALFInstrumentView>> m_view;
  std::unique_ptr<ALFInstrumentPresenter> m_presenter;
  std::unique_ptr<NiceMock<MockInstrumentActor>> m_instrumentActor;
  std::unique_ptr<NiceMock<MockALFAnalysisPresenter>> m_analysisPresenter;
};
