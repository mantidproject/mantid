// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ALFAnalysisMocks.h"
#include "ALFAnalysisPresenter.h"
#include "MockALFAlgorithmManager.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace Mantid::API;
using namespace testing;
using Mantid::Geometry::Instrument;
using namespace MantidQt::MantidWidgets;

class ALFAnalysisPresenterTest : public CxxTest::TestSuite {
public:
  ALFAnalysisPresenterTest() {
    FrameworkManager::Instance();
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    m_function = Mantid::API::FunctionFactory::Instance().createFunction("Gaussian");
  }

  static ALFAnalysisPresenterTest *createSuite() { return new ALFAnalysisPresenterTest(); }

  static void destroySuite(ALFAnalysisPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_algProperties = std::make_unique<AlgorithmRuntimeProps>();

    m_range = std::make_pair(0.0, 1.0);
    m_peakCentre = 0.5;
    m_background = 1.0;
    m_allTwoTheta = std::vector<double>{1.0, 2.3, 3.3};
    m_averageTwoTheta = 2.2;

    auto algorithmManager = std::make_unique<NiceMock<MockALFAlgorithmManager>>();
    auto model = std::make_unique<NiceMock<MockALFAnalysisModel>>();

    m_algorithmManager = algorithmManager.get();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockALFAnalysisView>>();
    m_presenter = std::make_unique<ALFAnalysisPresenter>(m_view.get(), std::move(model), std::move(algorithmManager));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    m_presenter.reset();
    m_view.reset();
  }

  void test_getView_will_get_the_view() {
    EXPECT_CALL(*m_view, getView()).Times(1).WillOnce(Return(nullptr));
    TS_ASSERT_EQUALS(nullptr, m_presenter->getView());
  }

  void test_setExtractedWorkspace_will_set_the_workspace_and_thetas_in_the_model_and_update_the_view() {
    auto const twoThetas = std::vector<double>{1.1, 2.2};

    EXPECT_CALL(*m_model, setExtractedWorkspace(Eq(m_workspace), twoThetas)).Times(1);

    expectCalculateEstimate(m_workspace);

    m_presenter->setExtractedWorkspace(m_workspace, twoThetas);
  }

  void test_setExtractedWorkspace_will_update_the_view_if_no_data_is_extracted() {
    auto workspace = nullptr;
    auto const twoThetas = std::vector<double>{1.1, 2.2};

    EXPECT_CALL(*m_model, setExtractedWorkspace(Eq(workspace), twoThetas)).Times(1);

    expectCalculateEstimate(workspace);

    m_presenter->setExtractedWorkspace(workspace, twoThetas);
  }

  void test_notifyPeakPickerChanged_will_removeFitSpectrum_if_fit_status_is_empty() {
    EXPECT_CALL(*m_view, getPeak()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_model, setPeakParameters(_)).Times(1);

    EXPECT_CALL(*m_model, fitStatus()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(*m_view, setPeakCentreStatus("")).Times(1);

    EXPECT_CALL(*m_view, removeFitSpectrum()).Times(1);

    // Assert is not called as is unnecessary
    EXPECT_CALL(*m_view, replot()).Times(0);

    m_presenter->notifyPeakPickerChanged();
  }

  void test_notifyPeakPickerChanged_will_not_removeFitSpectrum_if_fit_status_is_not_empty() {
    EXPECT_CALL(*m_view, getPeak()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_model, setPeakParameters(_)).Times(1);

    EXPECT_CALL(*m_model, fitStatus()).Times(1).WillOnce(Return("Success"));
    EXPECT_CALL(*m_view, setPeakCentreStatus("Success")).Times(1);

    // Assert is not called
    EXPECT_CALL(*m_view, removeFitSpectrum()).Times(0);

    // Assert is not called as is unnecessary
    EXPECT_CALL(*m_view, replot()).Times(0);

    m_presenter->notifyPeakPickerChanged();
  }

  void test_notifyPeakCentreEditingFinished_sets_the_peak_centre_in_the_model_and_fit_status_in_the_view() {
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_model, peakCentre()).Times(1).WillOnce(Return(0.0));
    EXPECT_CALL(*m_model, setPeakCentre(m_peakCentre)).Times(1);

    expectUpdatePeakCentreInViewFromModel();

    expectUpdateRotationAngleCalled();

    m_presenter->notifyPeakCentreEditingFinished();
  }

  void test_notifyPeakCentreEditingFinished_does_not_update_anything_if_the_peak_centre_remains_the_same() {
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_model, peakCentre()).Times(1).WillOnce(Return(m_peakCentre + 0.000000001));

    // Assert not called as the peak centre remains the same
    EXPECT_CALL(*m_model, setPeakCentre(m_peakCentre)).Times(0);
    EXPECT_CALL(*m_model, background()).Times(0);
    EXPECT_CALL(*m_model, getPeakCopy()).Times(0);
    EXPECT_CALL(*m_view, setPeak(_, _)).Times(0);
    EXPECT_CALL(*m_model, fitStatus()).Times(0);
    EXPECT_CALL(*m_view, setPeakCentreStatus("")).Times(0);
    EXPECT_CALL(*m_view, removeFitSpectrum()).Times(0);
    EXPECT_CALL(*m_view, replot()).Times(0);

    expectUpdateRotationAngleNotCalled();

    m_presenter->notifyPeakCentreEditingFinished();
  }

  void test_notifyPeakCentreEditingFinished_does_not_remove_fit_spectrum_when_fit_status_is_not_empty() {
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_model, setPeakCentre(m_peakCentre)).Times(1);

    EXPECT_CALL(*m_model, background()).Times(1).WillOnce(Return(m_background));
    EXPECT_CALL(*m_model, getPeakCopy()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_view, setPeak(_, m_background)).Times(1);

    EXPECT_CALL(*m_model, fitStatus()).Times(1).WillOnce(Return("Success"));
    EXPECT_CALL(*m_view, setPeakCentreStatus("Success")).Times(1);

    // Assert is not called
    EXPECT_CALL(*m_view, removeFitSpectrum()).Times(0);

    EXPECT_CALL(*m_view, replot()).Times(1);

    expectUpdateRotationAngleCalled();

    m_presenter->notifyPeakCentreEditingFinished();
  }

  void test_notifyFitClicked_will_display_a_warning_when_data_is_not_extracted() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_view, displayWarning("Need to have extracted data to do a fit or estimate.")).Times(1);

    expectUpdateRotationAngleNotCalled();

    m_presenter->notifyFitClicked();
  }

  void test_notifyFitClicked_will_display_a_warning_when_the_peak_centre_is_outside_the_fit_range() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(-1.0));
    EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(m_range));
    EXPECT_CALL(*m_view, displayWarning("The Peak Centre provided is outside the fit range.")).Times(1);

    expectUpdateRotationAngleNotCalled();

    m_presenter->notifyFitClicked();
  }

  void test_notifyFitClicked_will_perform_a_fit_when_the_workspace_and_peak_centre_is_valid() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_view, getRange()).Times(2).WillRepeatedly(Return(m_range));

    EXPECT_CALL(*m_view, disable("Fitting")).Times(1);
    EXPECT_CALL(*m_model, fitProperties(m_range)).Times(1).WillOnce(Return(ByMove(std::move(m_algProperties))));
    EXPECT_CALL(*m_algorithmManager, fit(NotNull())).Times(1);

    m_presenter->notifyFitClicked();
  }

  void test_that_notifyExportWorkspaceToADSClicked_calls_the_expected_model_function() {
    EXPECT_CALL(*m_model, exportWorkspaceCopyToADS()).Times(1);
    m_presenter->notifyExportWorkspaceToADSClicked();
  }

  void test_that_notifyExternalPlotClicked_will_open_an_external_plot_from_view() {
    auto const workspace = WorkspaceCreationHelper::create2DWorkspace(1, 100);
    std::vector<int> const workspaceIndices{0, 1};

    EXPECT_CALL(*m_model, plottedWorkspace()).Times(1).WillOnce(Return(workspace));

    EXPECT_CALL(*m_model, plottedWorkspaceIndices()).Times(1).WillOnce(Return(workspaceIndices));
    EXPECT_CALL(*m_view, openExternalPlot(Eq(workspace), workspaceIndices)).Times(1);

    m_presenter->notifyExternalPlotClicked();
  }

  void test_that_notifyExternalPlotClicked_will_not_open_external_plot_if_workspace_is_null() {
    EXPECT_CALL(*m_model, plottedWorkspace()).Times(1).WillOnce(Return(nullptr));

    // Assert not called
    EXPECT_CALL(*m_model, plottedWorkspaceIndices()).Times(0);
    EXPECT_CALL(*m_view, openExternalPlot(_, _)).Times(0);

    m_presenter->notifyExternalPlotClicked();
  }

  void test_that_calculateEstimate_is_not_called_when_data_is_not_extracted() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(false));

    // Assert no call to calculateEstimate
    EXPECT_CALL(*m_model, calculateEstimate(_)).Times(0);

    m_presenter->notifyResetClicked();
  }

  void test_that_calculateEstimate_is_called_as_expected() {
    expectCalculateEstimate(m_workspace);
    m_presenter->notifyResetClicked();
  }

  void test_numberOfTubes_will_call_the_model_method() {
    auto const nTubes(2u);
    EXPECT_CALL(*m_model, numberOfTubes()).Times(1).WillOnce(Return(nTubes));

    TS_ASSERT_EQUALS(nTubes, m_presenter->numberOfTubes());
  }

  void test_clear_will_clear_the_two_theta_in_the_model_and_update_the_view() {
    EXPECT_CALL(*m_model, clear()).Times(1);

    EXPECT_CALL(*m_model, extractedWorkspace()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_view, addSpectrum(_)).Times(1);

    EXPECT_CALL(*m_model, averageTwoTheta()).Times(1).WillOnce(Return(m_averageTwoTheta));
    EXPECT_CALL(*m_model, allTwoThetas()).Times(1).WillOnce(Return(m_allTwoTheta));
    EXPECT_CALL(*m_view, setAverageTwoTheta(m_averageTwoTheta, m_allTwoTheta)).Times(1);

    m_presenter->clear();
  }

  void test_notifyCropWorkspaceComplete_triggers_the_model_to_calculate_an_estimate_peak() {
    EXPECT_CALL(*m_model, calculateEstimate(_)).Times(1);
    EXPECT_CALL(*m_view, enable()).Times(1);

    m_presenter->notifyCropWorkspaceComplete(nullptr);
  }

  void test_notifyFitComplete_will_update_the_model_and_then_the_view() {
    std::string const fitStatus("Fit success");

    EXPECT_CALL(*m_model, setFitResult(Eq(m_workspace), Eq(m_function), fitStatus)).Times(1);
    EXPECT_CALL(*m_model, fitWorkspace()).Times(1).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_view, addFitSpectrum(Eq(m_workspace))).Times(1);
    expectUpdatePeakCentreInViewFromModel();
    expectUpdateRotationAngleCalled();
    EXPECT_CALL(*m_view, enable()).Times(1);

    m_presenter->notifyFitComplete(m_workspace, m_function, fitStatus);
  }

  void test_notifyAlgorithmError_will_display_a_message_in_the_view() {
    std::string const message("This is a warning message");

    EXPECT_CALL(*m_view, enable()).Times(1);
    EXPECT_CALL(*m_view, displayWarning(message)).Times(1);

    m_presenter->notifyAlgorithmError(message);
  }

private:
  void expectCalculateEstimate(MatrixWorkspace_sptr const &workspace) {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(workspace != nullptr));
    if (workspace) {
      EXPECT_CALL(*m_view, disable("Calculating estimate parameters")).Times(1);
      EXPECT_CALL(*m_view, getRange()).Times(1).WillRepeatedly(Return(m_range));

      EXPECT_CALL(*m_model, cropWorkspaceProperties(m_range))
          .Times(1)
          .WillOnce(Return(ByMove(std::move(m_algProperties))));
      EXPECT_CALL(*m_algorithmManager, cropWorkspace(NotNull())).Times(1);
    } else {
      expectUpdatePlotInViewFromModel();
    }
  }

  void expectUpdatePlotInViewFromModel() {
    EXPECT_CALL(*m_model, extractedWorkspace()).Times(1).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_view, addSpectrum(Eq(m_workspace))).Times(1);
  }

  void expectUpdatePeakCentreInViewFromModel() {
    EXPECT_CALL(*m_model, background()).Times(1).WillOnce(Return(m_background));
    EXPECT_CALL(*m_model, getPeakCopy()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_view, setPeak(_, m_background)).Times(1);

    EXPECT_CALL(*m_model, fitStatus()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(*m_view, setPeakCentreStatus("")).Times(1);

    EXPECT_CALL(*m_view, removeFitSpectrum()).Times(1);
    EXPECT_CALL(*m_view, replot()).Times(1);
  }

  void expectUpdateRotationAngleCalled() {
    std::optional<double> const angle(1.20003);
    EXPECT_CALL(*m_model, rotationAngle()).Times(1).WillOnce(Return(angle));
    EXPECT_CALL(*m_view, setRotationAngle(angle)).Times(1);
  }

  void expectUpdateRotationAngleNotCalled() {
    // Assert these functions are not called
    EXPECT_CALL(*m_model, rotationAngle()).Times(0);
    EXPECT_CALL(*m_view, setRotationAngle(_)).Times(0);
  }

  std::unique_ptr<AlgorithmRuntimeProps> m_algProperties;

  Mantid::API::MatrixWorkspace_sptr m_workspace;
  Mantid::API::IFunction_sptr m_function;
  std::pair<double, double> m_range;
  double m_peakCentre;
  double m_background;
  std::vector<double> m_allTwoTheta;
  std::optional<double> m_averageTwoTheta;

  NiceMock<MockALFAlgorithmManager> *m_algorithmManager;
  NiceMock<MockALFAnalysisModel> *m_model;
  std::unique_ptr<NiceMock<MockALFAnalysisView>> m_view;
  std::unique_ptr<ALFAnalysisPresenter> m_presenter;
};
