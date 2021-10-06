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
#include "IndirectFitDataModel.h"
#include "IndirectFitDataPresenter.h"
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

namespace {

std::unique_ptr<QTableWidget> createEmptyTableWidget(int columns, int rows) {
  auto table = std::make_unique<QTableWidget>(columns, rows);
  for (auto column = 0; column < columns; ++column)
    for (auto row = 0; row < rows; ++row)
      table->setItem(row, column, new QTableWidgetItem("item"));
  return table;
}

struct TableItem {
  TableItem(std::string const &value) : m_str(value), m_dbl(0.0) {}
  TableItem(double const &value) : m_str(QString::number(value, 'g', 16).toStdString()), m_dbl(value) {}

  std::string const &asString() const { return m_str; }
  QString asQString() const { return QString::fromStdString(m_str); }
  double const &asDouble() const { return m_dbl; }

  bool operator==(std::string const &value) const { return this->asString() == value; }

private:
  std::string m_str;
  double m_dbl;
};
} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIIndirectFitDataView : public IIndirectFitDataView {
public:
  /// Public Methods
  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_METHOD1(validate, UserInputValidator &(UserInputValidator &validator));
  MOCK_METHOD2(addTableEntry, void(size_t row, FitDataRow newRow));
  MOCK_CONST_METHOD0(workspaceIndexColumn, int());
  MOCK_CONST_METHOD0(startXColumn, int());
  MOCK_CONST_METHOD0(endXColumn, int());
  MOCK_CONST_METHOD0(excludeColumn, int());
  MOCK_METHOD0(clearTable, void());
  MOCK_CONST_METHOD2(getText, QString(int row, int column));
  MOCK_CONST_METHOD0(getSelectedIndexes, QModelIndexList());

  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

/// Mock object to mock the model
class MockIndirectFitDataModel : public IIndirectFitDataModel {
public:
  /// Public Methods
  MOCK_METHOD0(getFittingData, std::vector<IndirectFitData> *());
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const std::string &spectra));
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const FunctionModelSpectra &spectra));
  MOCK_METHOD2(addWorkspace, void(MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(FitDomainIndex index));
  MOCK_CONST_METHOD0(getWorkspaceNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, WorkspaceID());
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));

  MOCK_METHOD2(setSpectra, void(const std::string &spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(FunctionModelSpectra &&spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(const FunctionModelSpectra &spectra, WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectra, FunctionModelSpectra(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectrum, size_t(FitDomainIndex index));
  MOCK_CONST_METHOD1(getNumberOfSpectra, size_t(WorkspaceID workspaceID));

  MOCK_METHOD0(clear, void());

  MOCK_CONST_METHOD0(getNumberOfDomains, size_t());
  MOCK_CONST_METHOD2(getDomainIndex, FitDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getSubIndices, std::pair<WorkspaceID, WorkspaceIndex>(FitDomainIndex));

  MOCK_CONST_METHOD0(getQValuesForData, std::vector<double>());
  MOCK_CONST_METHOD0(getResolutionsForFit, std::vector<std::pair<std::string, size_t>>());
  MOCK_CONST_METHOD1(createDisplayName, std::string(WorkspaceID workspaceID));

  MOCK_METHOD1(removeWorkspace, void(WorkspaceID workspaceID));
  MOCK_METHOD1(removeDataByIndex, void(FitDomainIndex fitDomainIndex));

  MOCK_METHOD3(setStartX, void(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setStartX, void(double startX, WorkspaceID workspaceID));
  MOCK_METHOD3(setEndX, void(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setEndX, void(double endX, WorkspaceID workspaceID));
  MOCK_METHOD3(setExcludeRegion, void(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setExcludeRegion, void(const std::string &exclude, FitDomainIndex index));
  MOCK_METHOD1(setResolution, void(const std::string &name));
  MOCK_METHOD2(setResolution, void(const std::string &name, WorkspaceID workspaceID));
  MOCK_CONST_METHOD2(getFittingRange, std::pair<double, double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getFittingRange, std::pair<double, double>(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegion, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegionVector, std::vector<double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegionVector, std::vector<double>(FitDomainIndex index));
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
    m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();
    m_table = createEmptyTableWidget(5, 5);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_table.get()));
    m_presenter = std::make_unique<IndirectFitDataPresenter>(std::move(m_model.get()), std::move(m_view.get()));
    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    deleteSetup();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_addWorkspace_with_spectra_calls_to_model() {
    EXPECT_CALL(*m_model, addWorkspace("WorkspaceName", "0-3")).Times(Exactly(1));
    m_presenter->addWorkspace("WorkspaceName", "0-3");
  }

  void test_setResolution_calls_to_model() {
    EXPECT_CALL(*m_model, setResolution("WorkspaceName")).Times(Exactly(1));
    m_presenter->setResolution("WorkspaceName");
  }

  void test_that_setSampleWSSuffices_will_set_the_sample_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setSampleWSSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getSampleWSSuffices(), suffices);
  }

  void test_that_setSampleFBSuffices_will_set_the_sample_file_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setSampleFBSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getSampleFBSuffices(), suffices);
  }

  void test_that_setResolutionWSSuffices_will_set_the_Resolution_workspace_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setResolutionWSSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getResolutionWSSuffices(), suffices);
  }

  void test_that_setResolutionFBSuffices_will_set_the_Resolution_file_suffices_in_the_view() {
    QStringList const suffices{"suffix1", "suffix2"};
    m_presenter->setResolutionFBSuffices(suffices);
    TS_ASSERT_EQUALS(m_presenter->getResolutionFBSuffices(), suffices);
  }

  void test_getResolutionsForFit_calls_from_model() {
    std::vector<std::pair<std::string, size_t>> resolutions = {{"string", 1}};
    EXPECT_CALL(*m_model, getResolutionsForFit()).Times(Exactly(1)).WillOnce(Return(resolutions));
    TS_ASSERT_EQUALS(m_presenter->getResolutionsForFit(), resolutions)
  }

  void test_updateTableFromModel_clears_table_and_adds_new_row_for_each_entry() {
    EXPECT_CALL(*m_view, clearTable()).Times(Exactly(1));
    EXPECT_CALL(*m_model, getNumberOfDomains()).Times(Exactly(4)).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(0))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(1))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(2))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    FitDataRow newRow;
    EXPECT_CALL(*m_view, addTableEntry(0, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(1, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(2, _)).Times(Exactly(1));

    m_presenter->updateTableFromModel();
  }

  void test_getNumberOfDomains_calls_from_model() {
    size_t noDomains = 1;
    EXPECT_CALL(*m_model, getNumberOfDomains()).Times(Exactly(1)).WillOnce(Return(noDomains));
    TS_ASSERT_EQUALS(m_presenter->getNumberOfDomains(), noDomains)
  }

  void test_getQValuesForData_calls_from_model() {
    std::vector<double> qValues = {1.0, 2.0, 2.5, -1.5};
    EXPECT_CALL(*m_model, getQValuesForData()).Times(Exactly(1)).WillOnce(Return(qValues));
    TS_ASSERT_EQUALS(m_presenter->getQValuesForData(), qValues)
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
  std::unique_ptr<MockIndirectFitDataModel> m_model;
  std::unique_ptr<IndirectFitDataPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
