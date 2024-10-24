// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <utility>

#include "QENSFitting/FunctionBrowser/FunctionTemplateView.h"
#include "QENSFitting/InelasticFitPropertyBrowser.h"
#include "QENSFitting/ParameterEstimation.h"

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MockObjects.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace MantidQt::MantidWidgets;
using namespace testing;

namespace {
TableWorkspace_sptr createTableWorkspace(std::size_t const &size) { return std::make_shared<TableWorkspace>(size); }
} // namespace

class InelasticFitPropertyBrowserTest : public CxxTest::TestSuite {
public:
  static InelasticFitPropertyBrowserTest *createSuite() { return new InelasticFitPropertyBrowserTest(); }

  static void destroySuite(InelasticFitPropertyBrowserTest *suite) { delete suite; }

  void setUp() override {
    m_browser = std::make_unique<InelasticFitPropertyBrowser>();
    m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr, FittingMode::SEQUENTIAL_AND_SIMULTANEOUS);
    m_browser->init();
    m_templateBrowser = std::make_unique<NiceMock<MockFunctionTemplateView>>();
    auto templatePresenter = std::make_unique<NiceMock<MockFunctionTemplatePresenter>>(m_templateBrowser.get());
    m_templatePresenter = templatePresenter.get();
    m_browser->setFunctionTemplatePresenter(std::move(templatePresenter));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_templateBrowser.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_templatePresenter));

    m_browser.reset();
    m_fitOptionsBrowser.reset();
    m_templateBrowser.reset();
  }

  void test_setFunction_sets_function_in_template() {
    std::string funString = "FunctionString";
    EXPECT_CALL(*m_templatePresenter, setFunction(funString)).Times(Exactly(1));
    m_browser->setFunction(funString);
  }

  void test_getNumberOfDatasets_returns_value_from_template() {
    ON_CALL(*m_templatePresenter, getNumberOfDatasets()).WillByDefault(Return(5));
    EXPECT_CALL(*m_templatePresenter, getNumberOfDatasets());
    TS_ASSERT_EQUALS(m_browser->getNumberOfDatasets(), 5);
  }

  void test_getSingleFunctionString_returns_from_template() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templatePresenter, getFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templatePresenter, getFunction()).Times(Exactly(1));
    m_browser->getSingleFunctionStr();
  }

  void test_getFitFunction_returns_modified_multi_domain_function_if_domains_0() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templatePresenter, getFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templatePresenter, getFunction());
    ON_CALL(*m_templatePresenter, getNumberOfDatasets()).WillByDefault(Return(0));
    EXPECT_CALL(*m_templatePresenter, getNumberOfDatasets());

    auto returnFun = m_browser->getFitFunction();

    auto multiDomainFunction = std::make_shared<MultiDomainFunction>();
    multiDomainFunction->addFunction(fun);
    multiDomainFunction->setDomainIndex(0, 0);
    TS_ASSERT_EQUALS(returnFun->asString(), multiDomainFunction->asString());
  }

  void test_getFitFunction_returns_modified_multi_domain_function_if_domains_1() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templatePresenter, getGlobalFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templatePresenter, getGlobalFunction());
    ON_CALL(*m_templatePresenter, getNumberOfDatasets()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templatePresenter, getNumberOfDatasets());

    auto returnFun = m_browser->getFitFunction();

    auto multiDomainFunction = std::make_shared<MultiDomainFunction>();
    multiDomainFunction->addFunction(fun);
    multiDomainFunction->setDomainIndex(0, 0);
    TS_ASSERT_EQUALS(returnFun->asString(), multiDomainFunction->asString());
  }

  void test_minimizer_returns_options_value() {
    auto minimizer = m_fitOptionsBrowser->getProperty("Minimizer").toStdString();

    TS_ASSERT_EQUALS(m_browser->minimizer(), minimizer);
  }

  void test_maxIterations_returns_options_value() {
    auto maxIterations = m_fitOptionsBrowser->getProperty("MaxIterations").toInt();

    TS_ASSERT_EQUALS(m_browser->maxIterations(), maxIterations);
  }

  void test_getPeakRadius_returns_options_value() {
    auto peakRadius = m_fitOptionsBrowser->getProperty("PeakRadius").toInt();

    TS_ASSERT_EQUALS(m_browser->getPeakRadius(), peakRadius);
  }

  void test_costFunction_returns_options_value() {
    auto costFunction = m_fitOptionsBrowser->getProperty("CostFunction").toStdString();

    TS_ASSERT_EQUALS(m_browser->costFunction(), costFunction);
  }

  void test_convolveMembers_returns_corect_value() {
    m_browser->setConvolveMembers(false);
    TS_ASSERT(!m_browser->convolveMembers());

    m_browser->setConvolveMembers(true);
    TS_ASSERT(m_browser->convolveMembers());
  }

  void test_outputCompositeMembers_returns_corect_value() {
    m_browser->setOutputCompositeMembers(false);
    TS_ASSERT(!m_browser->outputCompositeMembers());

    m_browser->setOutputCompositeMembers(true);
    TS_ASSERT(m_browser->outputCompositeMembers());
  }

  void test_fitEvaluationType_returns_options_value() {
    auto evaluationType = m_fitOptionsBrowser->getProperty("EvaluationType").toStdString();

    TS_ASSERT_EQUALS(m_browser->fitEvaluationType(), evaluationType);
  }

  void test_fitType_returns_options_value() {
    auto fitType = m_fitOptionsBrowser->getProperty("FitType").toStdString();

    TS_ASSERT_EQUALS(m_browser->fitType(), fitType);
  }

  void test_ignoreInvalidData_returns_false() { TS_ASSERT(!m_browser->ignoreInvalidData()); }

  void test_updateParameters_calls_to_template() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    EXPECT_CALL(*m_templatePresenter, updateParameters(_)).Times(Exactly(1));
    m_browser->updateParameters(*fun);
  }

  void test_updateMultiDatasetParameters_with_function_does_not_throw() {
    // EXPECT_CALL can not be used with a function becauyse FOR SOMME REASION IT
    // IS BEING PASSED AS AN OBJECT INSTEAD OF A POINTER.
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    m_browser->updateMultiDatasetParameters(*fun);
  }

  void test_updateMultiDatasetParameters_with_table_does_not_throw() {
    auto tableWS = createTableWorkspace(5);
    m_browser->updateMultiDatasetParameters(*tableWS);
  }

  void test_updateFitStatusData_does_not_throw() {

    auto browser = std::make_unique<InelasticFitPropertyBrowser>();
    auto templateBrowser = std::make_unique<NiceMock<MockFunctionTemplateView>>();
    auto templatePresenter = std::make_unique<NiceMock<MockFunctionTemplatePresenter>>(templateBrowser.get());
    browser->setFunctionTemplatePresenter(std::move(templatePresenter));
    browser->init();

    std::vector<std::string> status = {"success", "success"};
    std::vector<double> chisq = {1.0, 2.0};
    browser->updateFitStatusData(status, chisq);
  }

  void test_setCurrentDataset_calls_to_template() {
    ON_CALL(*m_templatePresenter, getNumberOfDatasets()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templatePresenter, getNumberOfDatasets()).Times(Exactly(1));
    EXPECT_CALL(*m_templatePresenter, setCurrentDataset(1)).Times(Exactly(1));
    m_browser->setCurrentDataset(FitDomainIndex{1});
  }

  void test_currentDataset_returns_from_template() {
    ON_CALL(*m_templatePresenter, getCurrentDataset()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templatePresenter, getCurrentDataset()).Times(Exactly(1));
    TS_ASSERT_EQUALS(m_browser->currentDataset(), FitDomainIndex{static_cast<size_t>(1)});
  }

  void test_updateFunctionBrowserData_calls_template_correctly() {
    int nData = 2;
    QList<FunctionModelDataset> datasets;
    for (auto i = 0u; i < 2; ++i) {
      WorkspaceID workspaceID{i};

      auto const name = "wsName" + std::to_string(i);
      datasets.append(FunctionModelDataset(name, FunctionModelSpectra("0")));
    }
    std::vector<double> qValues = {0.0, 1.0};
    std::vector<std::pair<std::string, size_t>> fitResolutions(1, std::make_pair<std::string, size_t>("resWS", 0));
    EXPECT_CALL(*m_templatePresenter, setNumberOfDatasets(nData)).Times(Exactly(1));
    EXPECT_CALL(*m_templatePresenter, setQValues(qValues)).Times(Exactly(1));
    EXPECT_CALL(*m_templatePresenter, setResolution(fitResolutions)).Times(Exactly(1));
    m_browser->updateFunctionBrowserData(nData, datasets, qValues, fitResolutions);
  }

  void test_setErrorsEnabled_calls_to_template() {
    EXPECT_CALL(*m_templatePresenter, setErrorsEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(*m_templatePresenter, setErrorsEnabled(true)).Times(Exactly(1));
    m_browser->setErrorsEnabled(false);
    m_browser->setErrorsEnabled(true);
  }

  void test_updateParameterEstimationData_moves_to_template() {
    auto dataOne = DataForParameterEstimation{{0.0, 1.0, 2.0}, {0.0, 1.0, 2.0}};
    auto dataTwo = DataForParameterEstimation{{0.0, 1.0, 2.0}, {0.0, 1.0, 2.0}};
    auto data = DataForParameterEstimationCollection{dataOne, dataTwo};
    m_browser->updateParameterEstimationData(std::move(data));
  }

  void test_estimateFunctionParameters_calls_template() {
    EXPECT_CALL(*m_templatePresenter, estimateFunctionParameters()).Times(Exactly(1));
    m_browser->estimateFunctionParameters();
  }

  void test_setBackgroundA0_calls_template() {
    EXPECT_CALL(*m_templatePresenter, setBackgroundA0(1.0)).Times(Exactly(1));
    m_browser->setBackgroundA0(1.0);
  }

private:
  std::unique_ptr<InelasticFitPropertyBrowser> m_browser;
  std::unique_ptr<NiceMock<MockFunctionTemplateView>> m_templateBrowser;
  NiceMock<MockFunctionTemplatePresenter> *m_templatePresenter;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
};
