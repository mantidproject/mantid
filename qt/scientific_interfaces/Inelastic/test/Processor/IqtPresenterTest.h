// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Processor/IqtPresenter.h"

#include "../QENSFitting/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Common/MockUserInputValidator.h"

#include <MantidQtWidgets/Common/MockAlgorithmRunner.h>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class IqtPresenterTest : public CxxTest::TestSuite {
public:
  static IqtPresenterTest *createSuite() { return new IqtPresenterTest(); }

  static void destroySuite(IqtPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("workspace_test", m_workspace);
    m_validator = std::make_unique<NiceMock<MockUserInputValidator>>();

    m_view = std::make_unique<NiceMock<MockIqtView>>();
    auto model = std::make_unique<NiceMock<MockIqtModel>>();
    m_model = model.get();

    m_outputPlotView = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    m_runView = std::make_unique<NiceMock<MockRunView>>();
    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();

    ON_CALL(*m_view, getPlotOptions()).WillByDefault(Return((m_outputPlotView.get())));
    ON_CALL(*m_view, getRunView()).WillByDefault(Return((m_runView.get())));
    m_presenter = std::make_unique<IqtPresenter>(nullptr, std::move(algorithmRunner), m_view.get(), std::move(model));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_handleValidation_will_raise_error_if_emax_lower_than_emin() {
    ON_CALL(*m_model, EMin).WillByDefault(Return(1));
    ON_CALL(*m_model, EMax).WillByDefault(Return(0));
    EXPECT_CALL(*m_validator, addErrorMessage("ELow must be less than EHigh.\n", false)).Times(Exactly(1));
    m_presenter->handleValidation(m_validator.get());
  }

  void test_handleSampDataReady_will_raise_error_with_invalid_workspace() {
    EXPECT_CALL(*m_view, showMessageBox("Unable to retrieve workspace: ghost_ws")).Times(Exactly(1));
    EXPECT_CALL(*m_view, setPreviewSpectrumMaximum(0)).Times(Exactly(1));
    m_presenter->handleSampDataReady("ghost_ws");
  }

  void test_handleSampDataReady_will_set_correct_input_workspace_on_presenter() {
    m_presenter->handlePreviewSpectrumChanged(4);
    EXPECT_CALL(*m_view, setPreviewSpectrumMaximum(4)).Times(Exactly(1));
    EXPECT_CALL(*m_view, plotInput(m_workspace, 4)).Times(Exactly(1));
    m_presenter->handleSampDataReady("workspace_test");
  }

  void test_handleValueChanged_sets_correct_double_property() {
    double value = 0.1;
    EXPECT_CALL(*m_model, setEnergyMin(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("ELow", value);
    EXPECT_CALL(*m_model, setEnergyMax(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("EHigh", value);
    EXPECT_CALL(*m_model, setNumBins(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("SampleBinning", value);
  }
  void test_handlePreviewSpectrum_changes_to_correct_spectra() {
    m_presenter->handleSampDataReady("workspace_test");
    EXPECT_CALL(*m_view, plotInput(m_workspace, 1)).Times(Exactly(1));
    m_presenter->handlePreviewSpectrumChanged(1);
  }

  void test_handlePlotCurrentPreview_does_not_plot_with_incorrect_ws_or_index() {
    // Invalid workspace
    m_presenter->handleSampDataReady("ghost_ws");
    EXPECT_CALL(*m_view, showMessageBox("Workspace not found - data may not be loaded.")).Times(Exactly(1));
    m_presenter->handlePlotCurrentPreview();

    // Index larger than max spectra (5)
    m_presenter->handleSampDataReady("workspace_test");
    m_presenter->handlePreviewSpectrumChanged(7);
    EXPECT_CALL(*m_view, showMessageBox("Workspace not found - data may not be loaded.")).Times(Exactly(1));
    m_presenter->handlePlotCurrentPreview();
  }

private:
  NiceMock<MockIqtModel> *m_model;
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;

  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_outputPlotView;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockIqtView>> m_view;
  std::unique_ptr<NiceMock<MockUserInputValidator>> m_validator;
  std::unique_ptr<IqtPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
