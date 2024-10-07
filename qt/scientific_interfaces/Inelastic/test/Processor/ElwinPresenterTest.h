// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "Processor/ElwinModel.h"
#include "Processor/ElwinPresenter.h"
#include "Processor/ElwinView.h"

#include "../QENSFitting/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"

#include <MantidQtWidgets/Common/MockAlgorithmRunner.h>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

class ElwinPresenterTest : public CxxTest::TestSuite {
public:
  static ElwinPresenterTest *createSuite() { return new ElwinPresenterTest(); }

  static void destroySuite(ElwinPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockElwinView>>();
    m_outputPlotView = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    m_runView = std::make_unique<NiceMock<MockRunView>>();

    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();

    auto model = std::make_unique<NiceMock<MockElwinModel>>();
    auto dataModel = std::make_unique<NiceMock<MockDataModel>>();
    m_model = model.get();
    m_dataModel = dataModel.get();

    ON_CALL(*m_view, getPlotOptions()).WillByDefault(Return((m_outputPlotView.get())));
    ON_CALL(*m_view, getRunView()).WillByDefault(Return((m_runView.get())));
    ON_CALL(*m_dataModel, getSpectra(WorkspaceID{0})).WillByDefault(Return(FunctionModelSpectra("0-1")));

    m_presenter = std::make_unique<ElwinPresenter>(nullptr, std::move(algorithmRunner), m_view.get(), std::move(model),
                                                   std::move(dataModel));

    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("workspace_test", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_algorithmRunner));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_dataModel));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_outputPlotView.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runView.get()));

    m_presenter.reset();
    m_view.reset();
    m_outputPlotView.reset();
    m_runView.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_handleValueChanged_sets_correct_bool_property() {

    EXPECT_CALL(*m_model, setNormalise(true)).Times((Exactly(1)));
    m_presenter->handleValueChanged("Normalise to Lowest Temp", true);

    EXPECT_CALL(*m_model, setBackgroundSubtraction(true)).Times((Exactly(1)));
    m_presenter->handleValueChanged("Background Subtraction", true);
  }

  void test_handleValueChanged_sets_correct_double_property() {
    double value = 0.1;

    EXPECT_CALL(*m_model, setIntegrationStart(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("IntegrationStart", value);
    EXPECT_CALL(*m_model, setIntegrationEnd(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("IntegrationEnd", value);
    EXPECT_CALL(*m_model, setBackgroundStart(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("BackgroundStart", value);
    EXPECT_CALL(*m_model, setBackgroundEnd(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("BackgroundEnd", value);
  }

  void test_handleRunClicked_doesnt_run_with_invalid_ranges() {
    EXPECT_CALL(*m_outputPlotView, clearWorkspaces()).Times(1);
    m_presenter->handleRun();
  }

  void test_handlePlotPreviewClicked_calls_warning_when_no_workspace() {
    EXPECT_CALL(*m_view, showMessageBox("Workspace not found - data may not be loaded.")).Times(Exactly(1));
    m_presenter->handlePlotPreviewClicked();
  }

  void test_handlePreviewSpectrumChanged_calls_correct_spectrum() {
    m_presenter->setInputWorkspace(m_workspace);
    auto spectrum = 1;
    EXPECT_CALL(*m_view, getPreviewSpec()).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(*m_view, plotInput(m_workspace, spectrum)).Times(Exactly(1));
    m_presenter->handlePreviewSpectrumChanged(spectrum);
  }

  void test_handleAddData_sets_preview_workspace_and_spectrum() {
    auto dialog = new MantidQt::MantidWidgets::AddWorkspaceDialog(nullptr);
    m_presenter->setSelectedSpectrum(0);
    m_presenter->setInputWorkspace(m_workspace);

    ON_CALL(*m_dataModel, getNumberOfWorkspaces()).WillByDefault(Return(1));
    EXPECT_CALL(*m_view, updatePreviewWorkspaceNames(m_dataModel->getWorkspaceNames())).Times(Exactly(1));
    EXPECT_CALL(*m_view, plotInput(m_workspace, 0)).Times(Exactly(1));
    m_presenter->handleAddData(dialog);
  }

  void test_handleRowModeChanged_gets_domains_when_rows_are_not_collapsed() {
    EXPECT_CALL(*m_view, isRowCollapsed()).WillOnce(Return(false));
    EXPECT_CALL(*m_dataModel, getNumberOfDomains()).Times(Exactly(1));
    m_presenter->handleRowModeChanged();
  }

  void test_handleRowModeChanged_gets_workspaces_when_rows_are_collapsed() {
    EXPECT_CALL(*m_view, isRowCollapsed()).WillOnce(Return(true));
    EXPECT_CALL(*m_dataModel, getNumberOfWorkspaces()).Times(Exactly(1));
    m_presenter->handleRowModeChanged();
  }

private:
  NiceMock<MockDataModel> *m_dataModel;
  NiceMock<MockElwinModel> *m_model;
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_outputPlotView;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockElwinView>> m_view;
  std::unique_ptr<ElwinPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
