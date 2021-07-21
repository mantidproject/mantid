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

#include "FunctionTemplateBrowser.h"
#include "IndirectFitPropertyBrowser.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidCurveFitting/Algorithms/ConvolutionFit.h"
#include "MantidCurveFitting/Algorithms/QENSFitSequential.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FitOptionsBrowser.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"
#include "ParameterEstimation.h"

using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::DataObjects;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace MantidQt::MantidWidgets;
using namespace testing;

using ConvolutionFitSequential = Algorithms::ConvolutionFit<Algorithms::QENSFitSequential>;

namespace {
TableWorkspace_sptr createTableWorkspace(std::size_t const &size) { return std::make_shared<TableWorkspace>(size); }
} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockFunctionTemplateBrowser : public FunctionTemplateBrowser {
public:
  void emitFunctionStructureChanged() { emit functionStructureChanged(); }
  // public methods
  MOCK_METHOD1(setFunction, void(const QString &funStr));
  MOCK_CONST_METHOD0(getGlobalFunction, IFunction_sptr());
  MOCK_CONST_METHOD0(getFunction, IFunction_sptr());
  MOCK_METHOD1(setNumberOfDatasets, void(int));
  MOCK_CONST_METHOD0(getNumberOfDatasets, int());
  MOCK_METHOD1(setDatasets, void(const QList<FunctionModelDataset> &datasets));
  MOCK_CONST_METHOD0(getGlobalParameters, QStringList());
  MOCK_CONST_METHOD0(getLocalParameters, QStringList());
  MOCK_METHOD1(setGlobalParameters, void(const QStringList &globals));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const IFunction &fun));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const ITableWorkspace &paramTable));
  MOCK_METHOD1(updateParameters, void(const IFunction &fun));
  MOCK_METHOD1(setCurrentDataset, void(int i));
  MOCK_METHOD0(getCurrentDataset, int());
  MOCK_METHOD1(updateParameterNames, void(const QMap<int, QString> &parameterNames));
  MOCK_METHOD1(setErrorsEnabled, void(bool enabled));
  MOCK_METHOD0(clear, void());
  MOCK_METHOD1(updateParameterEstimationData, void(DataForParameterEstimationCollection &&data));
  MOCK_METHOD0(estimateFunctionParameters, void());
  MOCK_METHOD1(setBackgroundA0, void(double value));
  MOCK_METHOD2(setResolution, void(std::string const &name, WorkspaceID const &index));
  MOCK_METHOD1(setResolution, void(const std::vector<std::pair<std::string, size_t>> &fitResolutions));
  MOCK_METHOD1(setQValues, void(const std::vector<double> &qValues));
  // protected Slots
  MOCK_METHOD1(popupMenu, void(const QPoint &));
  MOCK_METHOD3(globalChanged, void(QtProperty *, const QString &, bool));
  MOCK_METHOD1(parameterChanged, void(QtProperty *));
  MOCK_METHOD1(parameterButtonClicked, void(QtProperty *));
  // Private methods
  MOCK_METHOD0(createBrowser, void());
  MOCK_METHOD0(createProperties, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitPropertyBrowserTest : public CxxTest::TestSuite {
public:
  /// WorkflowAlgorithms do not appear in the FrameworkManager without this line
  IndirectFitPropertyBrowserTest() { FrameworkManager::Instance(); }

  static IndirectFitPropertyBrowserTest *createSuite() { return new IndirectFitPropertyBrowserTest(); }

  static void destroySuite(IndirectFitPropertyBrowserTest *suite) { delete suite; }

  void setUp() override {
    m_browser = std::make_unique<IndirectFitPropertyBrowser>();
    m_fitOptionsBrowser = std::make_unique<FitOptionsBrowser>(nullptr, FittingMode::SEQUENTIAL_AND_SIMULTANEOUS);
    m_browser->init();
    m_templateBrowser = std::make_unique<NiceMock<MockFunctionTemplateBrowser>>();
    EXPECT_CALL(*m_templateBrowser, createBrowser());
    EXPECT_CALL(*m_templateBrowser, createProperties());
    m_browser->setFunctionTemplateBrowser(m_templateBrowser.get());
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    m_browser.reset();
    m_fitOptionsBrowser.reset();
    m_templateBrowser.reset();
  }

  void test_setFunctionTemplateBrowser_sets_up_FunctionTemplateBrowser() {
    TS_ASSERT_EQUALS(m_templateBrowser->objectName(), "templateBrowser");
  }

  void test_setFunction_sets_function_in_template() {
    QString funString = "FunctionString";
    EXPECT_CALL(*m_templateBrowser, setFunction(funString)).Times(Exactly(1));
    m_browser->setFunction(funString);
  }

  void test_getNumberOfDatasets_returns_value_from_template() {
    ON_CALL(*m_templateBrowser, getNumberOfDatasets()).WillByDefault(Return(5));
    EXPECT_CALL(*m_templateBrowser, getNumberOfDatasets());
    TS_ASSERT_EQUALS(m_browser->getNumberOfDatasets(), 5);
  }

  void test_getSingleFunctionString_returns_from_template() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templateBrowser, getFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templateBrowser, getFunction()).Times(Exactly(1));
    m_browser->getSingleFunctionStr();
  }

  void test_getFitFunction_returns_modified_multi_domain_function_if_domains_0() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templateBrowser, getFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templateBrowser, getFunction());
    ON_CALL(*m_templateBrowser, getNumberOfDatasets()).WillByDefault(Return(0));
    EXPECT_CALL(*m_templateBrowser, getNumberOfDatasets());

    auto returnFun = m_browser->getFitFunction();

    auto multiDomainFunction = std::make_shared<MultiDomainFunction>();
    multiDomainFunction->addFunction(fun);
    multiDomainFunction->setDomainIndex(0, 0);
    TS_ASSERT_EQUALS(returnFun->asString(), multiDomainFunction->asString());
  }

  void test_getFitFunction_returns_modified_multi_domain_function_if_domains_1() {
    auto fun = FunctionFactory::Instance().createInitialized("name=LinearBackground,A0=0,A1=0");
    ON_CALL(*m_templateBrowser, getGlobalFunction()).WillByDefault(Return(fun));
    EXPECT_CALL(*m_templateBrowser, getGlobalFunction());
    ON_CALL(*m_templateBrowser, getNumberOfDatasets()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templateBrowser, getNumberOfDatasets());

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
    EXPECT_CALL(*m_templateBrowser, updateParameters(_)).Times(Exactly(1));
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

    auto browser = std::make_unique<IndirectFitPropertyBrowser>();
    auto templateBrowser = std::make_unique<MockFunctionTemplateBrowser>();
    browser->setFunctionTemplateBrowser(templateBrowser.get());
    browser->init();

    std::vector<std::string> status = {"success", "success"};
    std::vector<double> chisq = {1.0, 2.0};
    browser->updateFitStatusData(status, chisq);
  }

  void test_setCurrentDataset_calls_to_template() {
    ON_CALL(*m_templateBrowser, getNumberOfDatasets()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templateBrowser, getNumberOfDatasets()).Times(Exactly(1));
    EXPECT_CALL(*m_templateBrowser, setCurrentDataset(1)).Times(Exactly(1));
    m_browser->setCurrentDataset(FitDomainIndex{1});
  }

  void test_currentDataset_returns_from_template() {
    ON_CALL(*m_templateBrowser, getCurrentDataset()).WillByDefault(Return(1));
    EXPECT_CALL(*m_templateBrowser, getCurrentDataset()).Times(Exactly(1));
    TS_ASSERT_EQUALS(m_browser->currentDataset(), FitDomainIndex{static_cast<size_t>(1)});
  }

  void test_updateFunctionBrowserData_calls_template_correctly() {
    int nData = 2;
    QList<FunctionModelDataset> datasets;
    for (auto i = 0u; i < 2; ++i) {
      WorkspaceID workspaceID{i};

      auto const name = "wsName" + std::to_string(i);
      datasets.append(FunctionModelDataset(QString::fromStdString(name), FunctionModelSpectra("0")));
    }
    std::vector<double> qValues = {0.0, 1.0};
    std::vector<std::pair<std::string, size_t>> fitResolutions(1, std::make_pair<std::string, size_t>("resWS", 0));
    EXPECT_CALL(*m_templateBrowser, setNumberOfDatasets(nData)).Times(Exactly(1));
    EXPECT_CALL(*m_templateBrowser, setQValues(qValues)).Times(Exactly(1));
    EXPECT_CALL(*m_templateBrowser, setResolution(fitResolutions)).Times(Exactly(1));
    m_browser->updateFunctionBrowserData(nData, datasets, qValues, fitResolutions);
  }

  void test_setErrorsEnabled_calls_to_template() {
    EXPECT_CALL(*m_templateBrowser, setErrorsEnabled(false)).Times(Exactly(1));
    EXPECT_CALL(*m_templateBrowser, setErrorsEnabled(true)).Times(Exactly(1));
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
    EXPECT_CALL(*m_templateBrowser, estimateFunctionParameters()).Times(Exactly(1));
    m_browser->estimateFunctionParameters();
  }

  void test_setBackgroundA0_calls_template() {
    EXPECT_CALL(*m_templateBrowser, setBackgroundA0(1.0)).Times(Exactly(1));
    m_browser->setBackgroundA0(1.0);
  }

private:
  std::unique_ptr<IndirectFitPropertyBrowser> m_browser;
  std::unique_ptr<MockFunctionTemplateBrowser> m_templateBrowser;
  std::unique_ptr<FitOptionsBrowser> m_fitOptionsBrowser;
};