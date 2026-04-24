// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Corrections/ContainerSubtractionPresenter.h"
#include "Corrections/ContainerSubtractionView.h"

#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MockObjects.h"

#include "MantidKernel/WarningSuppressions.h"

using namespace testing;
using namespace MantidQt::CustomInterfaces;

class ContainerSubtractionPresenterTest : public CxxTest::TestSuite {
public:
  static ContainerSubtractionPresenterTest *createSuite() { return new ContainerSubtractionPresenterTest(); }

  static void destroySuite(ContainerSubtractionPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);

    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();
    m_runView = std::make_unique<NiceMock<MockRunView>>();
    m_outputNameView = std::make_unique<NiceMock<MockOutputNameView>>();
    m_view = std::make_unique<NiceMock<MockContainerSubtractionView>>();
    m_outputPlotView = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    auto model = std::make_unique<NiceMock<MockContainerSubtractionModel>>();
    m_model = model.get();

    ON_CALL(*m_view, getRunView()).WillByDefault(Return((m_runView.get())));
    ON_CALL(*m_view, getPlotOptions()).WillByDefault(Return((m_outputPlotView.get())));
    ON_CALL(*m_view, getOutputNameView()).WillByDefault(Return((m_outputNameView.get())));

    EXPECT_CALL(*m_view, subscribe(_)).Times(Exactly(1));
    m_presenter = std::make_unique<ContainerSubtractionPresenter>(nullptr, std::move(algorithmRunner), std::move(model),
                                                                  m_view.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    m_presenter.reset();
    m_view.reset();
  }

  void test_handle_sample_ready_updates_plot_for_new_valid_ws() {
    const auto testName = "test";
    const auto ws = m_workspace;
    MatrixWorkspace_sptr ws_empty;

    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(ws));
    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, modCanWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, subtractedWS()).WillByDefault(ReturnRef(ws_empty));
    EXPECT_CALL(*m_model, setSampleWS(testName)).Times(Exactly(1));
    EXPECT_CALL(*m_model, removeSubtractedWS()).Times(Exactly(1));
    EXPECT_CALL(*m_view, setSpMax(4)).Times(Exactly(1));
    EXPECT_CALL(*m_view, clearPlot()).Times(Exactly(1));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::SAMPLE, ws, 0)).Times(Exactly(1));

    m_presenter->handleSampleReady(testName);
  }

  void test_handle_can_ready_updates_plot_for_new_valid_ws() {
    const auto testName = "test";
    const auto ws = m_workspace;
    MatrixWorkspace_sptr ws_empty;

    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(ws));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, modCanWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, subtractedWS()).WillByDefault(ReturnRef(ws_empty));
    EXPECT_CALL(*m_model, setCanWS(testName)).Times(Exactly(1));
    EXPECT_CALL(*m_model, removeSubtractedWS()).Times(Exactly(1));
    EXPECT_CALL(*m_view, setSpMax(4)).Times(Exactly(1));
    EXPECT_CALL(*m_view, clearPlot()).Times(Exactly(1));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::CONTAINER, ws, 0)).Times(Exactly(1));

    m_presenter->handleCanReady(testName);
  }

  void test_handle_can_ready_does_not_update_plot_for_invalid_ws() {
    const auto testName = "test";
    const MatrixWorkspace_sptr ws_empty;

    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, modCanWS()).WillByDefault(ReturnRef(ws_empty));
    ON_CALL(*m_model, subtractedWS()).WillByDefault(ReturnRef(ws_empty));
    EXPECT_CALL(*m_model, setCanWS(testName)).Times(Exactly(1));
    EXPECT_CALL(*m_model, removeSubtractedWS()).Times(Exactly(0));
    EXPECT_CALL(*m_view, setSpMax(4)).Times(Exactly(0));
    EXPECT_CALL(*m_view, clearPlot()).Times(Exactly(0));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::SAMPLE, ws_empty, 0)).Times(Exactly(0));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::CONTAINER, ws_empty, 0)).Times(Exactly(0));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::SUBTRACTED, ws_empty, 0)).Times(Exactly(0));

    m_presenter->handleCanReady(testName);
  }

  void test_update_plots_only_updates_available_ws() {
    const auto ws = m_workspace;
    const MatrixWorkspace_sptr invalidWs;
    EXPECT_CALL(*m_view, clearPlot()).Times(Exactly(1));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(ws));

    ON_CALL(*m_model, subtractedWS()).WillByDefault(ReturnRef(invalidWs));
    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(invalidWs));
    ON_CALL(*m_model, modCanWS()).WillByDefault(ReturnRef(invalidWs));

    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::SAMPLE, ws, 0)).Times(Exactly(1));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::CONTAINER, ws, 0)).Times(Exactly(0));
    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::SUBTRACTED, ws, 0)).Times(Exactly(0));
    m_presenter->updatePlot(0);
  }

  void test_update_plots_chooses_mod_can_if_both_available() {
    const MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace(4, 5);
    const MatrixWorkspace_sptr invalidWs;
    EXPECT_CALL(*m_view, clearPlot()).Times(Exactly(1));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(invalidWs));
    ON_CALL(*m_model, subtractedWS()).WillByDefault(ReturnRef(invalidWs));

    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(m_workspace));
    ON_CALL(*m_model, modCanWS()).WillByDefault(ReturnRef(ws2));

    EXPECT_CALL(*m_view, plotSpectrum(CSCurves::CONTAINER, ws2, 0)).Times(Exactly(1));
    m_presenter->updatePlot(0);
  }

  void test_handle_run_requests_rebin_for_uneven_binning() {
    const MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace(5, 7);

    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(m_workspace));
    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(ws2));
    EXPECT_CALL(*m_view, requestRebinToSample()).Times(Exactly(1));

    m_presenter->handleRun();
  }

  void test_handle_validation_fails_for_wrong_ws() {
    auto validator = std::make_unique<UserInputValidator>();
    const MatrixWorkspace_sptr invalidWs;
    const MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace(5, 4);

    EXPECT_CALL(*m_view, validate(validator.get())).Times(Exactly(1));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(m_workspace));
    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(invalidWs));

    m_presenter->handleValidation(validator.get());

    TS_ASSERT(!validator->isAllInputValid());
    TS_ASSERT(validator->generateErrorMessage().find("Sample or Container workspaces are not loaded") !=
              std::string::npos);
  }

  void test_handle_validation_fails_for_uneven_workspaces() {
    auto validator = std::make_unique<UserInputValidator>();
    const MatrixWorkspace_sptr ws2 = WorkspaceCreationHelper::create2DWorkspace(6, 4);

    EXPECT_CALL(*m_view, validate(validator.get())).Times(Exactly(1));
    ON_CALL(*m_model, sampleWS()).WillByDefault(ReturnRef(m_workspace));
    ON_CALL(*m_model, canWS()).WillByDefault(ReturnRef(ws2));

    m_presenter->handleValidation(validator.get());
    TS_ASSERT(!validator->isAllInputValid());
    TS_ASSERT(validator->generateErrorMessage().find(
                  "Sample and Container do not have a matching number of Histograms") != std::string::npos);
  }

private:
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_outputPlotView;
  std::unique_ptr<NiceMock<MockOutputNameView>> m_outputNameView;
  std::unique_ptr<ContainerSubtractionPresenter> m_presenter;
  std::unique_ptr<NiceMock<MockContainerSubtractionView>> m_view;
  NiceMock<MockContainerSubtractionModel> *m_model;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
};
