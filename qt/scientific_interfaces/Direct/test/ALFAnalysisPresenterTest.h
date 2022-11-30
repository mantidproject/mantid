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
  ALFAnalysisPresenterTest() { FrameworkManager::Instance(); }

  static ALFAnalysisPresenterTest *createSuite() { return new ALFAnalysisPresenterTest(); }

  static void destroySuite(ALFAnalysisPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_workspaceName = "test";
    m_range = std::make_pair(0.0, 1.0);
    m_peakCentre = 0.5;
    m_allTwoTheta = std::vector<double>{1.0, 2.3, 3.3};
    m_averageTwoTheta = 2.2;

    auto model = std::make_unique<NiceMock<MockALFAnalysisModel>>();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockALFAnalysisView>>();
    m_presenter = std::make_unique<ALFAnalysisPresenter>(m_view.get(), std::move(model));

    m_view->setPeakCentre(m_peakCentre);
    m_model->setPeakCentre(m_peakCentre);
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

    EXPECT_CALL(*m_model, setExtractedWorkspace(_, twoThetas)).Times(1);

    EXPECT_CALL(*m_model, extractedWorkspace()).Times(1).WillOnce(Return(nullptr));
    EXPECT_CALL(*m_view, addSpectrum(_)).Times(1);

    EXPECT_CALL(*m_model, averageTwoTheta()).Times(1).WillOnce(Return(m_averageTwoTheta));
    EXPECT_CALL(*m_model, allTwoThetas()).Times(1).WillOnce(Return(m_allTwoTheta));
    EXPECT_CALL(*m_view, setAverageTwoTheta(m_averageTwoTheta, m_allTwoTheta)).Times(1);

    m_presenter->setExtractedWorkspace(nullptr, twoThetas);
  }

  void test_notifyPeakCentreEditingFinished_sets_the_peak_centre_in_the_model_and_fit_status_in_the_view() {
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_model, setPeakCentre(m_peakCentre)).Times(1);

    EXPECT_CALL(*m_model, fitStatus()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(*m_view, setPeakCentreStatus("")).Times(1);

    m_presenter->notifyPeakCentreEditingFinished();
  }

  void test_notifyFitClicked_will_display_a_warning_when_data_is_not_extracted() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_view, displayWarning("Need to have extracted data to do a fit or estimate.")).Times(1);

    m_presenter->notifyFitClicked();
  }

  void test_notifyFitClicked_will_display_a_warning_when_the_peak_centre_is_outside_the_fit_range() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(-1.0));
    EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(m_range));
    EXPECT_CALL(*m_view, displayWarning("The Peak Centre provided is outside the fit range.")).Times(1);

    m_presenter->notifyFitClicked();
  }

  void test_notifyFitClicked_will_perform_a_fit_when_the_workspace_and_peak_centre_is_valid() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_view, getRange()).Times(2).WillRepeatedly(Return(m_range));

    EXPECT_CALL(*m_model, doFit(m_range)).Times(1);

    m_presenter->notifyFitClicked();
  }

  void test_that_calculateEstimate_is_not_called_when_data_is_not_extracted() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(false));
    EXPECT_CALL(*m_view, displayWarning("Need to have extracted data to do a fit or estimate.")).Times(1);

    m_presenter->notifyUpdateEstimateClicked();
  }

  void test_that_calculateEstimate_is_not_called_when_the_peak_centre_is_invalid() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(-1.0));
    EXPECT_CALL(*m_view, getRange()).Times(1).WillOnce(Return(m_range));
    EXPECT_CALL(*m_view, displayWarning("The Peak Centre provided is outside the fit range.")).Times(1);

    m_presenter->notifyUpdateEstimateClicked();
  }

  void test_that_calculateEstimate_is_called_as_expected() {
    EXPECT_CALL(*m_model, isDataExtracted()).Times(1).WillOnce(Return(true));
    EXPECT_CALL(*m_view, peakCentre()).Times(1).WillOnce(Return(m_peakCentre));
    EXPECT_CALL(*m_view, getRange()).Times(2).WillRepeatedly(Return(m_range));

    EXPECT_CALL(*m_model, calculateEstimate(m_range)).Times(1);

    m_presenter->notifyUpdateEstimateClicked();
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

private:
  std::string m_workspaceName;
  std::pair<double, double> m_range;
  double m_peakCentre;
  std::vector<double> m_allTwoTheta;
  std::optional<double> m_averageTwoTheta;

  NiceMock<MockALFAnalysisModel> *m_model;
  std::unique_ptr<NiceMock<MockALFAnalysisView>> m_view;
  std::unique_ptr<ALFAnalysisPresenter> m_presenter;
};
