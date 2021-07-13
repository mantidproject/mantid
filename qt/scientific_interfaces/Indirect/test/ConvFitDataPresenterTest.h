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

  /// Public slots
  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

/// Mock object to mock the model
class MockConvFitModel : public ConvFitModel {};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class ConvFitDataPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  ConvFitDataPresenterTest() { FrameworkManager::Instance(); }

  static ConvFitDataPresenterTest *createSuite() { return new ConvFitDataPresenterTest(); }

  static void destroySuite(ConvFitDataPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockConvFitDataView>>();
    m_model = std::make_unique<NiceMock<MockConvFitModel>>();

    m_dataTable = createEmptyTableWidget(6, 5);
    ON_CALL(*m_view, getDataTable()).WillByDefault(Return(m_dataTable.get()));

    m_presenter = std::make_unique<ConvFitDataPresenter>(std::move(m_model.get()), std::move(m_view.get()));

    SetUpADSWithWorkspace m_ads("WorkspaceName", createWorkspace(6));
    m_model->addWorkspace("WorkspaceName");
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

  void test_that_the_model_contains_the_correct_number_of_workspace_after_instantiation() {
    TS_ASSERT_EQUALS(m_model->getNumberOfWorkspaces(), WorkspaceID{1});
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

private:
  std::unique_ptr<QTableWidget> m_dataTable;

  std::unique_ptr<MockConvFitDataView> m_view;
  std::unique_ptr<MockConvFitModel> m_model;
  std::unique_ptr<ConvFitDataPresenter> m_presenter;
};
