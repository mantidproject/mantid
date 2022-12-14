// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "ALFAnalysisModel.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <memory>
#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

class ALFAnalysisModelTest : public CxxTest::TestSuite {
public:
  ALFAnalysisModelTest() { FrameworkManager::Instance(); }

  static ALFAnalysisModelTest *createSuite() { return new ALFAnalysisModelTest(); }

  static void destroySuite(ALFAnalysisModelTest *suite) { delete suite; }

  void setUp() override {
    m_workspace = WorkspaceCreationHelper::create2DWorkspace(1, 100);
    m_workspaceName = "test";
    m_range = std::make_pair<double, double>(0.0, 100.0);
    m_twoThetas = std::vector<double>{29.5, 30.4, 31.0};

    m_model = std::make_unique<ALFAnalysisModel>();
  }

  void tearDown() override { m_model.reset(); }

  void test_that_the_model_is_instantiated_with_a_function_and_empty_fit_status() {
    TS_ASSERT_EQUALS(nullptr, m_model->extractedWorkspace());
    TS_ASSERT_THROWS_NOTHING(m_model->getPeakCopy());
    TS_ASSERT_THROWS_NOTHING(m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
    TS_ASSERT_EQUALS(0u, m_model->numberOfTubes());
    TS_ASSERT_EQUALS(std::nullopt, m_model->averageTwoTheta());
    TS_ASSERT(m_model->allTwoThetas().empty());
  }

  void test_that_doFit_sets_a_successful_fit_status_for_a_good_fit() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    m_model->doFit(m_range);

    TS_ASSERT(!AnalysisDataService::Instance().doesExist("__fit_Workspace"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("__fit_Parameters"));
    TS_ASSERT(!AnalysisDataService::Instance().doesExist("__fit_NormalisedCovarianceWorkspace"));

    TS_ASSERT_EQUALS(0.0, m_model->getPeakCopy()->getParameter("PeakCentre"));
    TS_ASSERT_EQUALS("success", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_zero_peak_centre_if_the_extracted_workspace_is_not_set() {
    AnalysisDataService::Instance().clear();

    m_model->calculateEstimate(m_range);

    TS_ASSERT_EQUALS(0.0, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_an_estimate_if_the_extracted_workspace_is_set() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->calculateEstimate(m_range);

    TS_ASSERT_EQUALS(0.5, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_zero_peak_centre_if_the_crop_range_is_invalid() {
    m_workspace = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 100, 300.0);
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->calculateEstimate(m_range);

    TS_ASSERT_EQUALS(0.0, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_setPeakCentre_will_remove_the_fit_status_and_set_the_peak_centre() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->doFit(m_range);

    m_model->setPeakCentre(1.1);

    TS_ASSERT_EQUALS(1.1, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_averagedTwoTheta_returns_the_average_of_the_two_thetas_in_the_model() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    TS_ASSERT_EQUALS(30.3, *m_model->averageTwoTheta());

    auto const expectedTwoThetas = std::vector<double>{29.5, 30.4, 31.0};
    TS_ASSERT_EQUALS(expectedTwoThetas, m_model->allTwoThetas());
  }

  void test_that_setExtractedWorkspace_will_set_the_two_theta_in_the_model() {
    m_model->setExtractedWorkspace(m_workspace, {29.5});

    TS_ASSERT_EQUALS(29.5, *m_model->averageTwoTheta());
    TS_ASSERT_EQUALS(std::vector<double>{29.5}, m_model->allTwoThetas());
  }

  void test_setPeakParameters_will_update_the_parameters_in_the_gaussian() {
    double height = 1.2;
    double centre = 1.5;
    double sigma = 1.8;

    auto gaussian = FunctionFactory::Instance().createFunction("Gaussian");
    gaussian->setParameter("Height", height);
    gaussian->setParameter("PeakCentre", centre);
    gaussian->setParameter("Sigma", sigma);

    m_model->setPeakParameters(std::dynamic_pointer_cast<IPeakFunction>(gaussian));

    auto const modelPeak = m_model->getPeakCopy();
    TS_ASSERT_EQUALS(height, modelPeak->getParameter("Height"));
    TS_ASSERT_EQUALS(centre, modelPeak->getParameter("PeakCentre"));
    TS_ASSERT_EQUALS(sigma, modelPeak->getParameter("Sigma"));
  }

  void test_that_clear_will_clear_the_two_thetas_and_extracted_workspace_from_the_model() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->doFit(m_range);

    m_model->clear();

    TS_ASSERT_EQUALS(nullptr, m_model->extractedWorkspace());
    TS_ASSERT_EQUALS(std::nullopt, m_model->averageTwoTheta());
    TS_ASSERT(m_model->allTwoThetas().empty());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_numberOfTubes_returns_the_number_of_two_thetas() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    TS_ASSERT_EQUALS(3u, m_model->numberOfTubes());
  }

  void test_rotationAngle_returns_nullopt_if_the_fit_status_is_empty() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    TS_ASSERT_EQUALS(std::nullopt, m_model->rotationAngle());
  }

  void test_rotationAngle_returns_the_correct_value_with_valid_data() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    m_model->setPeakCentre(0.1);
    m_model->doFit(m_range);

    TS_ASSERT_DELTA(-0.0557, *m_model->rotationAngle(), 0.0001);
  }

private:
  MatrixWorkspace_sptr m_workspace;
  std::string m_workspaceName;
  std::pair<double, double> m_range;
  std::vector<double> m_twoThetas;

  std::unique_ptr<ALFAnalysisModel> m_model;
};
