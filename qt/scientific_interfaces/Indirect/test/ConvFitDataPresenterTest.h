// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "ConvFitDataPresenter.h"
#include "ConvFitModel.h"
#include "IIndirectFitDataView.h"

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
} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockConvFitDataView : public IIndirectFitDataView {
public:
  /// Signals
  void emitResolutionLoaded(QString const &workspaceName) { emit resolutionLoaded(workspaceName); }

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

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ConvFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  ConvFitDataPresenterTest() { FrameworkManager::Instance(); }

  static ConvFitDataPresenterTest *createSuite() { return new ConvFitDataPresenterTest(); }

  static void destroySuite(ConvFitDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockConvFitDataView>>();
    m_model = std::make_unique<NiceMock<MockIndirectFitDataModel>>();

    m_dataTable = createEmptyTableWidget(6, 6);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));
    m_presenter = std::make_unique<ConvFitDataPresenter>(std::move(m_model.get()), std::move(m_view.get()));

    m_workspace = createWorkspace(6);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
    m_model->addWorkspace("WorkspaceName", "0-5");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset();
    m_model.reset();
    m_view.reset();
    m_dataTable.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful mock object instantiation
  ///----------------------------------------------------------------------

  void test_that_the_presenter_and_mock_objects_have_been_created() {
    TS_ASSERT(m_presenter);
    TS_ASSERT(m_model);
    TS_ASSERT(m_view);
  }

  void test_that_the_data_table_is_the_size_specified() {
    TS_ASSERT_EQUALS(m_dataTable->rowCount(), 6);
    TS_ASSERT_EQUALS(m_dataTable->columnCount(), 6);
  }

  void test_updateTableFromModel_clears_table_and_adds_new_row_for_each_entry() {
    EXPECT_CALL(*m_view, clearTable()).Times(Exactly(1));
    EXPECT_CALL(*m_model, getNumberOfDomains()).Times(Exactly(4)).WillRepeatedly(Return(3));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(0))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(1))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    EXPECT_CALL(*m_model, getWorkspace(FitDomainIndex(2))).Times(Exactly(1)).WillOnce(Return(m_workspace));
    std::vector<std::pair<std::string, size_t>> resolutionsForFit(
        {{"Workspace", 1}, {"Workspace", 1}, {"Workspace", 1}});
    EXPECT_CALL(*m_model, getResolutionsForFit()).Times(Exactly(3)).WillRepeatedly(Return(resolutionsForFit));
    FitDataRow newRow;
    EXPECT_CALL(*m_view, addTableEntry(0, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(1, _)).Times(Exactly(1));
    EXPECT_CALL(*m_view, addTableEntry(2, _)).Times(Exactly(1));

    m_presenter->updateTableFromModel();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

private:
  std::unique_ptr<QTableWidget> m_dataTable;

  std::unique_ptr<MockConvFitDataView> m_view;
  std::unique_ptr<MockIndirectFitDataModel> m_model;
  std::unique_ptr<ConvFitDataPresenter> m_presenter;
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
