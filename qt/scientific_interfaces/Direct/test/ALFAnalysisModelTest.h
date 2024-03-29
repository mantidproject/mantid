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
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

#include <memory>
#include <string>
#include <utility>

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;

namespace {

std::string const NOT_IN_ADS = "not_stored_in_ads";

}

class ALFAnalysisModelTest : public CxxTest::TestSuite {
public:
  ALFAnalysisModelTest() {
    FrameworkManager::Instance();
    m_workspace = WorkspaceCreationHelper::create2DWorkspacePoints(1, 100);
  }

  static ALFAnalysisModelTest *createSuite() { return new ALFAnalysisModelTest(); }

  static void destroySuite(ALFAnalysisModelTest *suite) { delete suite; }

  void setUp() override {
    m_function = std::make_shared<CompositeFunction>();
    m_function->addFunction(FunctionFactory::Instance().createFunction("FlatBackground"));
    m_function->addFunction(FunctionFactory::Instance().createFunction("Gaussian"));

    m_exportWorkspaceName = "ALFView_exported";
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

  void test_that_setFitResult_sets_a_successful_fit_status_for_a_good_fit() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    m_model->setFitResult(m_workspace, m_function, "success");

    TS_ASSERT_EQUALS(0.0, m_model->getPeakCopy()->getParameter("PeakCentre"));
    TS_ASSERT_EQUALS("success", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_zero_peak_centre_if_the_workspace_is_nullptr() {
    AnalysisDataService::Instance().clear();

    m_model->calculateEstimate(nullptr);

    TS_ASSERT_EQUALS(0.0, m_model->peakCentre());
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_calculateEstimate_returns_an_estimate_if_a_valid_workspace_is_provided() {
    // Set a maximum y value at x = 5.0
    m_workspace->mutableY(0)[5] = 3.0;

    m_model->calculateEstimate(m_workspace);

    TS_ASSERT_DELTA(5.0, m_model->peakCentre(), 0.00001);
    TS_ASSERT_EQUALS("", m_model->fitStatus());
  }

  void test_that_setPeakCentre_will_remove_the_fit_status_and_set_the_peak_centre() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->setFitResult(m_workspace, m_function, "success");

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

    m_model->setFitResult(m_workspace, m_function, "success");

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

    m_function->setParameter("f1.PeakCentre", 0.1);
    m_model->setFitResult(m_workspace, m_function, "success");

    TS_ASSERT_DELTA(0.1913, *m_model->rotationAngle(), 0.0001);
  }

  void test_plottedWorkspace_returns_nullptr_data_is_not_extracted() {
    TS_ASSERT_EQUALS(nullptr, m_model->plottedWorkspace());
  }

  void test_plottedWorkspace_returns_a_non_null_value_if_data_is_extracted() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    TS_ASSERT(m_model->plottedWorkspace());
  }

  void test_plottedWorkspaceIndices_returns_zero_if_there_is_no_fitted_workspace() {
    auto const expectedIndices = std::vector<int>{0};
    TS_ASSERT_EQUALS(expectedIndices, m_model->plottedWorkspaceIndices());
  }

  void test_plottedWorkspaceIndices_returns_zero_and_one_if_there_is_a_fitted_workspace() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    m_model->setFitResult(m_workspace, m_function, "success");

    auto const expectedIndices = std::vector<int>{0, 1};
    TS_ASSERT_EQUALS(expectedIndices, m_model->plottedWorkspaceIndices());
  }

  void test_exportWorkspaceCopyToADS_does_not_create_an_exported_workspace_if_data_is_not_extracted() {
    m_model->exportWorkspaceCopyToADS();
    TS_ASSERT(!AnalysisDataService::Instance().doesExist(m_exportWorkspaceName));
  }

  void test_exportWorkspaceCopyToADS_exports_a_workspace_to_the_ADS_when_the_fit_workspace_exists() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);
    m_model->setFitResult(m_workspace, m_function, "success");

    m_model->exportWorkspaceCopyToADS();

    auto &ads = AnalysisDataService::Instance();
    TS_ASSERT(ads.doesExist(m_exportWorkspaceName));
  }

  void test_exportWorkspaceCopyToADS_exports_a_workspace_with_one_spectra_when_fit_workspace_does_not_exist() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    m_model->exportWorkspaceCopyToADS();

    auto &ads = AnalysisDataService::Instance();
    TS_ASSERT(ads.doesExist(m_exportWorkspaceName));

    auto const workspace = ads.retrieveWS<MatrixWorkspace>(m_exportWorkspaceName);
    TS_ASSERT_EQUALS(1u, workspace->getNumberHistograms());
  }

  void test_cropWorkspaceProperties_returns_the_expected_properties() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    auto const range = std::make_pair<double, double>(-12.2, 14.4);
    auto const properties = m_model->cropWorkspaceProperties(range);

    MatrixWorkspace_sptr input = properties->getProperty("InputWorkspace");
    double xMin = properties->getProperty("XMin");
    double xMax = properties->getProperty("XMax");
    std::string output = properties->getProperty("OutputWorkspace");

    TS_ASSERT_EQUALS(m_workspace, input);
    TS_ASSERT_DELTA(xMin, range.first, 0.000001);
    TS_ASSERT_DELTA(xMax, range.second, 0.000001);
    TS_ASSERT_EQUALS(NOT_IN_ADS, output);
  }

  void test_fitProperties_returns_the_expected_properties() {
    m_model->setExtractedWorkspace(m_workspace, m_twoThetas);

    auto const range = std::make_pair<double, double>(-15.2, 15.4);

    // Populate the function member variable with an estimate
    m_model->calculateEstimate(m_workspace);

    auto const properties = m_model->fitProperties(range);

    IFunction_sptr function = properties->getProperty("Function");
    Workspace_sptr input = properties->getProperty("InputWorkspace");
    bool createOutput = properties->getProperty("CreateOutput");
    double startX = properties->getProperty("StartX");
    double endX = properties->getProperty("EndX");

    TS_ASSERT_EQUALS(m_model->getPeakCopy()->asString(), function->getFunction(1)->asString());
    TS_ASSERT_EQUALS(m_workspace, input);
    TS_ASSERT(createOutput);
    TS_ASSERT_DELTA(startX, range.first, 0.000001);
    TS_ASSERT_DELTA(endX, range.second, 0.000001);
  }

  void test_setFitResult_will_set_the_fit_workspace_in_the_model() {
    std::string const fitStatus("success");

    m_model->setFitResult(m_workspace, nullptr, fitStatus);

    TS_ASSERT_EQUALS(m_workspace, m_model->plottedWorkspace());
    TS_ASSERT_EQUALS(fitStatus, m_model->fitStatus());
  }

private:
  MatrixWorkspace_sptr m_workspace;
  CompositeFunction_sptr m_function;
  std::string m_exportWorkspaceName;
  std::pair<double, double> m_range;
  std::vector<double> m_twoThetas;

  std::unique_ptr<ALFAnalysisModel> m_model;
};
