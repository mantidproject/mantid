// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "BayesFitting/QuasiPresenter.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MockObjects.h"

#include <memory>

using namespace MantidQt::CustomInterfaces;
using namespace testing;

class QuasiPresenterTest : public CxxTest::TestSuite {
public:
  static QuasiPresenterTest *createSuite() { return new QuasiPresenterTest(); }

  static void destroySuite(QuasiPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);

    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();
    auto model = std::make_unique<NiceMock<MockQuasiModel>>();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockQuasiView>>();
    m_runView = std::make_unique<NiceMock<MockRunView>>();

    ON_CALL(*m_view, getRunView()).WillByDefault(Return((m_runView.get())));
    m_presenter = std::make_unique<QuasiPresenter>(nullptr, std::move(algorithmRunner), std::move(model), m_view.get());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_algorithmRunner));

    m_presenter.reset();
    m_view.reset();
  }

  void test_handleSampleInputReady_calls_the_expected_functions_when_sample_returned_is_a_nullptr() {
    std::string const workspaceName("WorkspaceName_red");

    EXPECT_CALL(*m_view, enableView(true)).Times(1);
    EXPECT_CALL(*m_model, setSample(workspaceName)).Times(1);
    ON_CALL(*m_model, sample()).WillByDefault(Return(nullptr));

    // Expect that these are not called
    EXPECT_CALL(*m_view, setPreviewSpectrumMax(_)).Times(0);
    EXPECT_CALL(*m_view, setXRange(_)).Times(0);

    m_presenter->handleSampleInputReady(workspaceName);
  }

  void test_handleSampleInputReady_calls_the_expected_functions_when_sample_returns_a_workspace() {
    std::string const workspaceName("WorkspaceName_red");

    EXPECT_CALL(*m_view, enableView(true)).Times(1);
    EXPECT_CALL(*m_model, setSample(workspaceName)).Times(1);
    ON_CALL(*m_model, sample()).WillByDefault(Return(m_workspace));

    // Expect that these are not called
    EXPECT_CALL(*m_view, setPreviewSpectrumMax(4u)).Times(1);
    expectUpdateMiniPlot();
    EXPECT_CALL(*m_view, setXRange(_)).Times(1);

    m_presenter->handleSampleInputReady(workspaceName);
  }

  void test_handleResolutionInputReady_calls_the_expected_functions() {
    std::string const workspaceName("WorkspaceName_res");

    EXPECT_CALL(*m_view, enableView(true)).Times(1);
    ON_CALL(*m_model, isResolution(workspaceName)).WillByDefault(Return(true));
    EXPECT_CALL(*m_view, enableUseResolution(true)).Times(1);
    EXPECT_CALL(*m_model, setResolution(workspaceName)).Times(1);

    m_presenter->handleResolutionInputReady(workspaceName);
  }

  void test_handleFileAutoLoaded_calls_the_expected_functions() {
    EXPECT_CALL(*m_view, enableView(true)).Times(1);
    m_presenter->handleFileAutoLoaded();
  }

  void test_handlePreviewSpectrumChanged_calls_updateMiniPlot() {
    expectUpdateMiniPlot();
    m_presenter->handlePreviewSpectrumChanged();
  }

  void test_handleSaveClicked_when_there_are_no_output_workspaces() {
    ON_CALL(*m_model, outputFitGroup()).WillByDefault(Return(nullptr));

    ON_CALL(*m_model, outputResult()).WillByDefault(Return(nullptr));

    ON_CALL(*m_model, outputProbability()).WillByDefault(Return(nullptr));

    // Expect that this is not called
    EXPECT_CALL(*m_model, setupSaveAlgorithm(_)).Times(0);

    std::deque<MantidQt::API::IConfiguredAlgorithm_sptr> emptyQueue;
    EXPECT_CALL(*m_algorithmRunner, execute(emptyQueue)).Times(1);

    m_presenter->handleSaveClicked();
  }

  void test_handleSaveClicked_calls_execute_with_a_populated_queue_when_some_output_workspaces_are_available() {
    ON_CALL(*m_model, outputFitGroup()).WillByDefault(Return(nullptr));

    ON_CALL(*m_model, outputResult()).WillByDefault(Return(m_workspace));

    ON_CALL(*m_model, outputProbability()).WillByDefault(Return(m_workspace));

    EXPECT_CALL(*m_algorithmRunner,
                execute(Matcher<std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>>(SizeIs(Eq(2)))))
        .Times(1);

    m_presenter->handleSaveClicked();
  }

  void test_setFileExtensionsByName_calls_the_expected_view_function() {
    bool const filter(true);

    EXPECT_CALL(*m_view, setFileExtensionsByName(filter)).Times(1);

    m_presenter->setFileExtensionsByName(filter);
  }

  void test_setLoadHistory_calls_the_expected_view_function() {
    bool const loadHistory(true);

    EXPECT_CALL(*m_view, setLoadHistory(loadHistory)).Times(1);

    m_presenter->setLoadHistory(loadHistory);
  }

private:
  void expectUpdateMiniPlot() {
    std::size_t const spectrum(0u);

    ON_CALL(*m_model, sample()).WillByDefault(Return(m_workspace));

    EXPECT_CALL(*m_view, clearPlot()).Times(1);
    ON_CALL(*m_view, previewSpectrum()).WillByDefault(Return(spectrum));
    expectAddSpectrum("Sample", spectrum);

    ON_CALL(*m_model, outputFit(spectrum)).WillByDefault(Return(nullptr));
  }

  void expectAddSpectrum(std::string const &label, std::size_t const spectrumIndex, std::string const &colour = "") {
    EXPECT_CALL(*m_view, addSpectrum(label, m_workspace, spectrumIndex, colour)).Times(1);
  }

  Mantid::API::MatrixWorkspace_sptr m_workspace;

  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  NiceMock<MockQuasiModel> *m_model;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockQuasiView>> m_view;
  std::unique_ptr<QuasiPresenter> m_presenter;
};
