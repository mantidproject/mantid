// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MantidQtWidgets/Common/MockUserInputValidator.h"
#include "MockObjects.h"
#include "QENSFitting/FittingPresenter.h"

#include <memory>

using namespace testing;

class FittingPresenterTest : public CxxTest::TestSuite {
public:
  static FittingPresenterTest *createSuite() { return new FittingPresenterTest(); }

  static void destroySuite(FittingPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    auto model = std::make_unique<NiceMock<MockFittingModel>>();
    m_model = model.get();
    m_browser = std::make_unique<NiceMock<MockInelasticFitPropertyBrowser>>();
    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();

    m_presenter =
        std::make_unique<FittingPresenter>(m_tab.get(), m_browser.get(), std::move(model), std::move(algorithmRunner));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_tab.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_browser.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_algorithmRunner));

    m_presenter.reset();
    m_tab.reset();
    m_browser.reset();
  }

  void test_notifyFunctionChanged_calls_the_tab() {
    EXPECT_CALL(*m_tab, handleFunctionChanged()).Times(1);
    m_presenter->notifyFunctionChanged();
  }

  void test_validate_calls_the_model_validate() {
    auto validator = std::make_unique<NiceMock<MockUserInputValidator>>();
    EXPECT_CALL(*m_model, validate(_)).Times(1);

    m_presenter->validate(validator.get());
  }

  void test_setFitFunction_calls_the_model() {
    auto function = std::make_shared<Mantid::API::MultiDomainFunction>();
    EXPECT_CALL(*m_model, setFitFunction(function)).Times(1);

    m_presenter->setFitFunction(function);
  }

  void test_setFitEnabled_calls_the_browser() {
    EXPECT_CALL(*m_browser, setFitEnabled(true)).Times(1);
    m_presenter->setFitEnabled(true);
  }

  void test_setCurrentDataset_calls_the_browser() {
    auto const domainIndex = FitDomainIndex{1};
    EXPECT_CALL(*m_browser, setCurrentDataset(domainIndex)).Times(1);

    m_presenter->setCurrentDataset(domainIndex);
  }

  void test_fitFunction_gets_the_function_from_the_browser() {
    std::string minimizerStr("FABADA");
    ON_CALL(*m_browser, minimizer(false)).WillByDefault(Return(minimizerStr));

    TS_ASSERT_EQUALS(minimizerStr, m_presenter->minimizer());
  }

  void test_estimateFunctionParameters_will_not_estimate_if_there_is_a_previous_fit() {
    auto workspaceID = WorkspaceID(0);
    auto workspaceIndex = WorkspaceIndex(0);

    ON_CALL(*m_model, isPreviouslyFit(workspaceID, workspaceIndex)).WillByDefault(Return(true));
    // Expect not called
    EXPECT_CALL(*m_browser, estimateFunctionParameters()).Times(0);

    m_presenter->estimateFunctionParameters(workspaceID, workspaceIndex);
  }

  void test_estimateFunctionParameters_will_estimate_if_there_is_not_a_previous_fit() {
    auto workspaceID = WorkspaceID(0);
    auto workspaceIndex = WorkspaceIndex(0);

    ON_CALL(*m_model, isPreviouslyFit(workspaceID, workspaceIndex)).WillByDefault(Return(false));
    EXPECT_CALL(*m_browser, estimateFunctionParameters()).Times(1);

    m_presenter->estimateFunctionParameters(workspaceID, workspaceIndex);
  }

  void test_removeFittingData_calls_the_model() {
    EXPECT_CALL(*m_model, removeFittingData()).Times(1);
    m_presenter->removeFittingData();
  }

  void test_addDefaultParameters_calls_the_model() {
    EXPECT_CALL(*m_model, addDefaultParameters()).Times(1);
    m_presenter->addDefaultParameters();
  }

  void test_removeDefaultParameters_calls_the_model() {
    EXPECT_CALL(*m_model, removeDefaultParameters()).Times(1);
    m_presenter->removeDefaultParameters();
  }

  void test_runFit_sets_fitting_mode_and_gets_fitting_algorithm() {
    FittingMode fittingMode = FittingMode::SIMULTANEOUS;

    EXPECT_CALL(*m_browser, getFittingMode()).Times(1).WillOnce(Return(fittingMode));
    EXPECT_CALL(*m_model, setFittingMode(fittingMode)).Times(1);
    EXPECT_CALL(*m_model, getFittingAlgorithm(fittingMode)).Times(1);
    mockExecuteFit();

    m_presenter->runFit();
  }

  void test_runSingleFit_sets_fitting_mode_and_gets_single_fitting_algorithm() {
    FittingMode fittingMode = FittingMode::SIMULTANEOUS;

    EXPECT_CALL(*m_model, setFittingMode(fittingMode)).Times(1);
    EXPECT_CALL(*m_model, getSingleFittingAlgorithm()).Times(1);
    mockExecuteFit();

    m_presenter->runSingleFit();
  }

  void test_getResultWorkspace_calls_model_and_returns_result() {
    Mantid::API::WorkspaceGroup_sptr expectedResult = std::make_shared<Mantid::API::WorkspaceGroup>();

    EXPECT_CALL(*m_model, getResultWorkspace()).Times(1).WillOnce(Return(expectedResult));

    auto result = m_presenter->getResultWorkspace();

    TS_ASSERT_EQUALS(result, expectedResult);
  }

  void test_getOutputBasename_calls_model_and_returns_output_basename() {
    std::string expectedOutputBasename = "output_basename";

    EXPECT_CALL(*m_model, getOutputBasename()).Times(1).WillOnce(Return(expectedOutputBasename));

    auto outputBasename = m_presenter->getOutputBasename();

    TS_ASSERT_EQUALS(outputBasename, expectedOutputBasename);
  }

  void test_getFitDataModel_calls_model_and_returns_fit_data_model() {
    IDataModel *expectedFitDataModel = nullptr;

    EXPECT_CALL(*m_model, getFitDataModel()).Times(1).WillOnce(Return(expectedFitDataModel));

    auto fitDataModel = m_presenter->getFitDataModel();

    TS_ASSERT_EQUALS(fitDataModel, expectedFitDataModel);
  }

  void test_getFitPlotModel_calls_model_and_returns_fit_plot_model() {
    IFitPlotModel *expectedFitPlotModel = nullptr;

    EXPECT_CALL(*m_model, getFitPlotModel()).Times(1).WillOnce(Return(expectedFitPlotModel));

    auto fitPlotModel = m_presenter->getFitPlotModel();

    TS_ASSERT_EQUALS(fitPlotModel, expectedFitPlotModel);
  }

  void test_isPreviouslyFit_calls_model_and_returns_result() {
    WorkspaceID workspaceID{0};
    WorkspaceIndex spectrum{1};
    bool expectedResult = true;

    EXPECT_CALL(*m_model, isPreviouslyFit(workspaceID, spectrum)).Times(1).WillOnce(Return(expectedResult));

    TS_ASSERT_EQUALS(expectedResult, m_presenter->isPreviouslyFit(workspaceID, spectrum));
  }

  void test_setFWHM_calls_model_with_correct_arguments() {
    WorkspaceID workspaceID{0};
    double fwhm = 1.0;

    EXPECT_CALL(*m_model, setFWHM(fwhm, workspaceID)).Times(1);

    m_presenter->setFWHM(workspaceID, fwhm);
  }

  void test_setBackground_calls_model_and_browser_with_correct_arguments() {
    WorkspaceID workspaceID{0};
    double background = 0.5;

    EXPECT_CALL(*m_model, setBackground(background, workspaceID)).Times(1);
    EXPECT_CALL(*m_browser, setBackgroundA0(background)).Times(1);

    m_presenter->setBackground(workspaceID, background);
  }

  void test_notifyBatchComplete_with_no_error() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
    MantidQt::API::IConfiguredAlgorithm_sptr configuredAlgorithm =
        std::make_shared<MantidQt::API::ConfiguredAlgorithm>(nullptr, std::move(properties));

    EXPECT_CALL(*m_browser, setErrorsEnabled(true)).Times(1);
    EXPECT_CALL(*m_model, setFitFunction(_)).Times(1);
    EXPECT_CALL(*m_model, addOutput(_)).Times(1);
    EXPECT_CALL(*m_tab, handleFitComplete(false)).Times(1);

    m_presenter->notifyBatchComplete(configuredAlgorithm, false);
  }

  void test_notifyBatchComplete_with_error() {
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
    MantidQt::API::IConfiguredAlgorithm_sptr configuredAlgorithm =
        std::make_shared<MantidQt::API::ConfiguredAlgorithm>(nullptr, std::move(properties));

    EXPECT_CALL(*m_browser, setErrorsEnabled(false)).Times(1);
    EXPECT_CALL(*m_model, cleanFailedRun(_)).Times(1);
    EXPECT_CALL(*m_tab, handleFitComplete(true)).Times(1);

    m_presenter->notifyBatchComplete(configuredAlgorithm, true);
  }

private:
  void mockExecuteFit() {
    ON_CALL(*m_model, getFittingMode()).WillByDefault(Return(FittingMode::SIMULTANEOUS));
    auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();

    EXPECT_CALL(*m_browser, fitProperties(FittingMode::SIMULTANEOUS)).WillOnce(Return(ByMove(std::move(properties))));
    EXPECT_CALL(*m_algorithmRunner, execute(A<MantidQt::API::IConfiguredAlgorithm_sptr>())).Times(1);
  }

  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  NiceMock<MockFittingModel> *m_model;
  std::unique_ptr<NiceMock<MockInelasticFitPropertyBrowser>> m_browser;
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  std::unique_ptr<FittingPresenter> m_presenter;
};
