// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectDataTablePresenter.h"
#include "IndirectFittingModel.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;
using namespace MantidQt::CustomInterfaces;

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

/// Mock object to mock the model
class MockIndirectDataTableModel : public IIndirectFitDataModel {
public:
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));
  MOCK_CONST_METHOD1(getWorkspace, Mantid::API::MatrixWorkspace_sptr(TableDatasetIndex index));
  MOCK_CONST_METHOD1(getSpectra, FunctionModelSpectra(TableDatasetIndex index));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(numberOfWorkspaces, TableDatasetIndex());
  MOCK_CONST_METHOD1(getNumberOfSpectra, size_t(TableDatasetIndex index));
  MOCK_CONST_METHOD0(getNumberOfDomains, size_t());
  MOCK_CONST_METHOD2(getDomainIndex, FitDomainIndex(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(getQValuesForData, std::vector<double>());
  MOCK_CONST_METHOD0(getResolutionsForFit, std::vector<std::pair<std::string, size_t>>());
  MOCK_CONST_METHOD0(getWorkspaceNames, std::vector<std::string>());

  MOCK_METHOD2(setSpectra, void(const std::string &spectra, TableDatasetIndex dataIndex));
  MOCK_METHOD2(setSpectra, void(FunctionModelSpectra &&spectra, TableDatasetIndex dataIndex));
  MOCK_METHOD2(setSpectra, void(const FunctionModelSpectra &spectra, TableDatasetIndex dataIndex));
  MOCK_METHOD1(addWorkspace, void(const std::string &workspaceName));
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const std::string &spectra));
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const FunctionModelSpectra &spectra));
  MOCK_METHOD2(addWorkspace, void(Mantid::API::MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra));
  MOCK_METHOD1(removeWorkspace, void(TableDatasetIndex index));
  MOCK_METHOD1(removeDataByIndex, void(FitDomainIndex fitDomainIndex));
  MOCK_METHOD0(clear, void());

  MOCK_CONST_METHOD2(getFittingRange,
                     std::pair<double, double>(TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(TableDatasetIndex dataIndex, IDA::WorkspaceIndex index));
  MOCK_CONST_METHOD2(getExcludeRegionVector,
                     std::vector<double>(TableDatasetIndex dataIndex, IDA::WorkspaceIndex index));
  MOCK_METHOD3(setStartX, void(double startX, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_METHOD2(setStartX, void(double startX, TableDatasetIndex dataIndex));
  MOCK_METHOD3(setEndX, void(double endX, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_METHOD2(setEndX, void(double endX, TableDatasetIndex dataIndex));

  MOCK_METHOD3(setExcludeRegion,
               void(const std::string &exclude, TableDatasetIndex dataIndex, IDA::WorkspaceIndex spectrum));
  MOCK_METHOD2(setResolution, void(const std::string &name, TableDatasetIndex index));
  MOCK_METHOD2(setExcludeRegion, void(const std::string &exclude, FitDomainIndex index));

  MOCK_CONST_METHOD1(getWorkspace, Mantid::API::MatrixWorkspace_sptr(FitDomainIndex index));
  MOCK_CONST_METHOD1(getFittingRange, std::pair<double, double>(FitDomainIndex index));
  MOCK_CONST_METHOD1(getSpectrum, size_t(FitDomainIndex index));
  MOCK_CONST_METHOD1(getExcludeRegionVector, std::vector<double>(FitDomainIndex index));
  MOCK_CONST_METHOD1(getExcludeRegion, std::string(FitDomainIndex index));

  MOCK_CONST_METHOD1(getSubIndices, std::pair<TableDatasetIndex, IDA::WorkspaceIndex>(FitDomainIndex));

  MOCK_METHOD0(switchToSingleInputMode, void());
  MOCK_METHOD0(switchToMultipleInputMode, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectDataTablePresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectDataTablePresenterTest() { FrameworkManager::Instance(); }

  static IndirectDataTablePresenterTest *createSuite() { return new IndirectDataTablePresenterTest(); }

  static void destroySuite(IndirectDataTablePresenterTest *suite) { delete suite; }

  void setUp() override {
    m_model = std::make_unique<NiceMock<MockIndirectDataTableModel>>();
    m_table = createEmptyTableWidget(5, 5);
    m_presenter = std::make_unique<IndirectDataTablePresenter>(std::move(m_model.get()), std::move(m_table.get()));

    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(5));
    m_model->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_table.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset();
    m_model.reset();
    m_table.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_model_has_been_instantiated_correctly() {
    ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

    m_model->isMultiFit();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals call the correct methods
  ///----------------------------------------------------------------------

  void test_that_the_cellChanged_signal_will_set_the_models_startX_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model, setStartX(2.0, TableDatasetIndex(0), IDA::WorkspaceIndex(0))).Times(1);
    m_table->item(0, START_X_COLUMN)->setText("2.0");
  }

  void test_that_the_cellChanged_signal_will_set_the_models_endX_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model, setEndX(2.0, TableDatasetIndex(0), IDA::WorkspaceIndex(0))).Times(1);
    m_table->item(0, END_X_COLUMN)->setText("2.0");
  }

  void test_that_the_cellChanged_signal_will_set_the_models_excludeRegion_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model, setExcludeRegion("0-4", TableDatasetIndex(0), IDA::WorkspaceIndex(0))).Times(1);
    m_table->item(0, EXCLUDE_REGION_COLUMN)->setText("0-4");
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots of the presenter
  ///----------------------------------------------------------------------

private:
  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(value, getTableItem(row, column));
  }

  void assertValueIsNotGlobal(int valueRow, int column, TableItem const &value) const {
    TS_ASSERT_EQUALS(value.asString(), getTableItem(valueRow, column));

    for (auto row = 0; row < m_table->rowCount(); ++row)
      if (row != valueRow)
        TS_ASSERT_DIFFERS(value, getTableItem(row, column));
  }

  std::string getTableItem(int row, int column) const { return m_table->item(row, column)->text().toStdString(); }

  std::unique_ptr<QTableWidget> m_table;
  std::unique_ptr<MockIndirectDataTableModel> m_model;
  std::unique_ptr<IndirectDataTablePresenter> m_presenter;
};
