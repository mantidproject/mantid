// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Spectroscopy/MockObjects.h"

#include "QENSFitting/ConvolutionAddWorkspaceDialog.h"
#include "QENSFitting/ConvolutionDataPresenter.h"
#include "QENSFitting/ConvolutionModel.h"
#include "QENSFitting/IFitDataView.h"
#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MockObjects.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
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

class ConvolutionDataPresenterTest : public CxxTest::TestSuite {
public:
  static ConvolutionDataPresenterTest *createSuite() { return new ConvolutionDataPresenterTest(); }

  static void destroySuite(ConvolutionDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    m_view = std::make_unique<NiceMock<MockFitDataView>>();
    m_model = std::make_unique<NiceMock<MockDataModel>>();

    m_dataTable = createEmptyTableWidget(6, 6);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));
    m_presenter = std::make_unique<ConvolutionDataPresenter>(m_tab.get(), m_model.get(), m_view.get());

    m_workspace = createWorkspace(6);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("WorkspaceName", m_workspace);
    m_model->addWorkspace("WorkspaceName", FunctionModelSpectra("0-5"));
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

  void test_addWorkspaceFromDialog_returns_false_if_the_dialog_is_not_Convolution() {
    auto dialog = new MantidQt::MantidWidgets::AddWorkspaceDialog(nullptr);
    TS_ASSERT(!m_presenter->addWorkspaceFromDialog(dialog));
  }

  void test_addWorkspaceFromDialog_returns_true_for_a_valid_dialog() {
    auto dialog = new ConvolutionAddWorkspaceDialog(nullptr);
    TS_ASSERT(m_presenter->addWorkspaceFromDialog(dialog));
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

  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  std::unique_ptr<NiceMock<MockFitDataView>> m_view;
  std::unique_ptr<NiceMock<MockDataModel>> m_model;
  std::unique_ptr<ConvolutionDataPresenter> m_presenter;
  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
