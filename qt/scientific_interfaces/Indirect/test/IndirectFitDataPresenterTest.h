// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IIndirectFitDataView.h"
#include "IndirectFitDataPresenter.h"
#include "IndirectFitDataTableModel.h"
#include "IndirectFitDataTablePresenterTest.h"
#include "IndirectFitDataView.h"
#include "IndirectFittingModel.h"
#include "ParameterEstimation.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIIndirectFitDataView : public IIndirectFitDataView {
public:
  /// Public Methods
  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_METHOD1(validate, UserInputValidator &(UserInputValidator &validator));

  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

/// Mock object to mock the model
class MockIndirectFitDataTableModel : public IIndirectFittingModel {
public:
  MockIndirectFitDataTableModel() : m_fitDataModel(std::make_unique<MockIndirectDataTableModel>()) {}

  /// Public Methods
  MOCK_CONST_METHOD2(isPreviouslyFit, bool(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(isInvalidFunction, boost::optional<std::string>());
  MOCK_CONST_METHOD0(getFitParameterNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getFitFunction, Mantid::API::MultiDomainFunction_sptr());
  MOCK_CONST_METHOD2(getParameterValues,
                     std::unordered_map<std::string, IDA::ParameterValue>(TableDatasetIndex dataIndex,
                                                                          IDA::WorkspaceIndex spectrum));
  MOCK_METHOD1(setFitFunction, void(Mantid::API::MultiDomainFunction_sptr function));
  MOCK_METHOD3(setDefaultParameterValue, void(const std::string &name, double value, IDA::TableDatasetIndex dataIndex));

  MOCK_CONST_METHOD2(getFitParameters,
                     std::unordered_map<std::string, IDA::ParameterValue>(TableDatasetIndex dataIndex,
                                                                          IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getDefaultParameters,
                     std::unordered_map<std::string, IDA::ParameterValue>(TableDatasetIndex dataIndex));
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(TableDatasetIndex index));
  MOCK_CONST_METHOD1(getSpectra, FunctionModelSpectra(TableDatasetIndex index));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, TableDatasetIndex());
  MOCK_CONST_METHOD1(getNumberOfSpectra, size_t(TableDatasetIndex index));
  MOCK_CONST_METHOD0(getNumberOfDomains, size_t());
  MOCK_CONST_METHOD2(getDomainIndex, FitDomainIndex(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(getQValuesForData, std::vector<double>());
  MOCK_CONST_METHOD0(getResolutionsForFit, std::vector<std::pair<std::string, size_t>>());
  MOCK_METHOD0(clearWorkspaces, void());
  MOCK_METHOD0(clear, void());
  MOCK_METHOD2(setSpectra, void(const std::string &spectra, TableDatasetIndex dataIndex));
  MOCK_METHOD2(setSpectra, void(FunctionModelSpectra &&spectra, TableDatasetIndex dataIndex));
  MOCK_METHOD2(setSpectra, void(const FunctionModelSpectra &spectra, TableDatasetIndex dataIndex));

  MOCK_METHOD1(addWorkspace, void(std::string const &workspaceName));
  MOCK_METHOD2(addWorkspace, void(std::string const &workspaceName, std::string const &spectra));
  MOCK_METHOD2(addWorkspace, void(std::string const &workspaceName, FunctionModelSpectra const &spectra));
  MOCK_METHOD2(addWorkspace, void(Mantid::API::MatrixWorkspace_sptr workspace, FunctionModelSpectra const &spectra));
  MOCK_METHOD1(removeWorkspace, void(TableDatasetIndex index));

  MOCK_CONST_METHOD2(getFittingRange,
                     std::pair<double, double>(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(TableDatasetIndex dataIndex, IDA::WorkspaceIndex index));
  MOCK_METHOD3(setStartX, void(double startX, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_METHOD2(setStartX, void(double startX, TableDatasetIndex dataIndex));
  MOCK_METHOD3(setEndX, void(double endX, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_METHOD2(setEndX, void(double endX, TableDatasetIndex dataIndex));
  MOCK_METHOD3(setExcludeRegion,
               void(const std::string &exclude, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));

  MOCK_METHOD3(addSingleFitOutput, void(const Mantid::API::IAlgorithm_sptr &fitAlgorithm, TableDatasetIndex index,
                                        IDA::WorkspaceIndex spectrum));
  MOCK_METHOD1(addOutput, void(Mantid::API::IAlgorithm_sptr fitAlgorithm));

  MOCK_METHOD0(switchToSingleInputMode, void());
  MOCK_METHOD0(switchToMultipleInputMode, void());
  MOCK_METHOD1(setFittingMode, void(FittingMode mode));
  MOCK_CONST_METHOD0(getFittingMode, FittingMode());
  MOCK_METHOD1(setFitTypeString, void(const std::string &fitType));
  MOCK_CONST_METHOD2(getResultLocation,
                     boost::optional<ResultLocationNew>(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));

  MOCK_CONST_METHOD0(getResultWorkspace, Mantid::API::WorkspaceGroup_sptr());
  MOCK_CONST_METHOD0(getResultGroup, Mantid::API::WorkspaceGroup_sptr());
  MOCK_CONST_METHOD0(getFittingAlgorithm, Mantid::API::IAlgorithm_sptr());
  MOCK_CONST_METHOD2(getSingleFit,
                     Mantid::API::IAlgorithm_sptr(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD2(getSingleFunction,
                     Mantid::API::IFunction_sptr(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));

  MOCK_CONST_METHOD0(getOutputBasename, std::string());
  MOCK_CONST_METHOD1(createDisplayName, std::string(TableDatasetIndex dataIndex));
  MOCK_METHOD1(cleanFailedRun, void(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm));
  MOCK_METHOD2(cleanFailedSingleRun,
               void(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm, TableDatasetIndex index));
  MOCK_CONST_METHOD1(getDataForParameterEstimation,
                     DataForParameterEstimationCollection(const EstimationDataSelector &selector));

  MOCK_METHOD0(removeFittingData, void());

  IIndirectFitDataTableModel *getFitDataModel() override { return m_fitDataModel.get(); }

protected:
  std::unique_ptr<IIndirectFitDataTableModel> m_fitDataModel;

private:
  std::string sequentialFitOutputName() const { return ""; };
  std::string simultaneousFitOutputName() const { return ""; };
  std::string singleFitOutputName(TableDatasetIndex index, IDA::WorkspaceIndex spectrum) const {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };
};
MATCHER_P(NoCheck, selector, "") { return arg != selector; }

EstimationDataSelector getEstimationDataSelector() {
  return [](const std::vector<double> &x, const std::vector<double> &y,
            const std::pair<double, double> range) -> DataForParameterEstimation {
    // Find data thats within range
    double xmin = range.first;
    double xmax = range.second;

    // If the two points are equal return empty data
    if (fabs(xmin - xmax) < 1e-7) {
      return DataForParameterEstimation{};
    }

    const auto startItr =
        std::find_if(x.cbegin(), x.cend(), [xmin](const double &val) -> bool { return val >= (xmin - 1e-7); });
    auto endItr = std::find_if(x.cbegin(), x.cend(), [xmax](const double &val) -> bool { return val > xmax; });

    if (std::distance(startItr, endItr - 1) < 2)
      return DataForParameterEstimation{};

    size_t first = std::distance(x.cbegin(), startItr);
    size_t end = std::distance(x.cbegin(), endItr);
    size_t m = first + (end - first) / 2;

    return DataForParameterEstimation{{x[first], x[m]}, {y[first], y[m]}};
  };
}

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectFitDataPresenterTest() { FrameworkManager::Instance(); }

  static IndirectFitDataPresenterTest *createSuite() { return new IndirectFitDataPresenterTest(); }

  static void destroySuite(IndirectFitDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockIIndirectFitDataView>>();
    m_model = std::make_unique<NiceMock<MockIndirectFitDataTableModel>>();
    m_table = createEmptyTableWidget(5, 5);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_table.get()));
    m_presenter = std::make_unique<IndirectFitDataPresenter>(std::move(m_model.get()), std::move(m_view.get()));

    TS_ASSERT(m_model->getFitDataModel());

    SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(5));
    m_model->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    deleteSetup();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful mock object instantiation
  ///----------------------------------------------------------------------

  void test_that_the_model_has_been_instantiated_correctly() {
    ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

    m_model->isMultiFit();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_that_setSampleWSSuffices_will_set_the_sample_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setSampleWSSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getSampleWSSuffices(), suffices);
  }

  void test_that_setSampleFBSuffices_will_set_the_sample_file_browser_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setSampleFBSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getSampleFBSuffices(), suffices);
  }

  void test_that_setResolutionWSSuffices_will_set_the_resolution_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setResolutionWSSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getResolutionWSSuffices(), suffices);
  }

  void test_that_setResolutionFBSuffices_will_set_the_resolution_file_browser_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setResolutionFBSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getResolutionFBSuffices(), suffices);
  }

  void test_that_setStartX_with_TableDatasetIndex_alters_endX_column() {
    double startX = 1.0;
    m_presenter->setStartX(startX, TableDatasetIndex{0});
    assertValueIsGlobal(START_X_COLUMN, startX);
  }

  void test_that_setStartX_with_TableDatasetIndex_and_WorkspaceIndex_alters_endX_column() {
    double startX = 1.0;
    m_presenter->setStartX(startX, TableDatasetIndex{0}, IDA::WorkspaceIndex{0});
    assertValueIsGlobal(START_X_COLUMN, startX);
  }

  void test_that_setEndX_with_TableDatasetIndex_alters_endX_column() {
    double endX = 1.0;
    m_presenter->setEndX(endX, TableDatasetIndex{0});
    assertValueIsGlobal(END_X_COLUMN, endX);
  }

  void test_that_setEndX_with_TableDatasetIndex_and_WorkspaceIndex_alters_endX_column() {
    double endX = 1.0;
    m_presenter->setEndX(endX, TableDatasetIndex{0}, IDA::WorkspaceIndex{0});
    assertValueIsGlobal(END_X_COLUMN, endX);
  }

  void test_that_the_setExcludeRegion_slot_will_alter_the_relevant_excludeRegion_column_in_the_table() {
    TableItem const excludeRegion("2-3");

    m_presenter->setExclude(excludeRegion.asString(), TableDatasetIndex{0}, IDA::WorkspaceIndex{0});

    assertValueIsGlobal(EXCLUDE_REGION_COLUMN, excludeRegion);
  }

  void test_getDataForParameterEstimation_uses_selector_to_get_from_model() {
    EstimationDataSelector selector = getEstimationDataSelector();

    EXPECT_CALL(*m_model, getDataForParameterEstimation(NoCheck(nullptr))).Times(Exactly(1));

    m_presenter->getDataForParameterEstimation(selector);
  }

private:
  void deleteSetup() {
    m_presenter.reset();
    m_model.reset();
    m_view.reset();

    m_table.reset();
  }

  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(value.asString(), getTableItem(row, column));
  }

  std::string getTableItem(int row, int column) const { return m_table->item(row, column)->text().toStdString(); }

  std::unique_ptr<QTableWidget> m_table;

  std::unique_ptr<MockIIndirectFitDataView> m_view;
  std::unique_ptr<MockIndirectFitDataTableModel> m_model;
  std::unique_ptr<IndirectFitDataPresenter> m_presenter;

  SetUpADSWithWorkspace *m_ads;
};
