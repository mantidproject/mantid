// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTDATATABLEPRESENTERTEST_H_
#define MANTIDQT_INDIRECTDATATABLEPRESENTERTEST_H_

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
using IDAWorkspaceIndex = MantidQt::CustomInterfaces::IDA::WorkspaceIndex;

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
  TableItem(double const &value)
      : m_str(QString::number(value, 'g', 16).toStdString()), m_dbl(value) {}

  std::string const &asString() const { return m_str; }
  QString asQString() const { return QString::fromStdString(m_str); }
  double const &asDouble() const { return m_dbl; }

  bool operator==(std::string const &value) const {
    return this->asString() == value;
  }

private:
  std::string m_str;
  double m_dbl;
};

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the model
class MockIndirectDataTableModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD2(getFittingRange,
                     std::pair<double, double>(DatasetIndex dataIndex,
                                               IDAWorkspaceIndex spectrum));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(DatasetIndex dataIndex,
                                                   IDAWorkspaceIndex index));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(numberOfWorkspaces, DatasetIndex());

  MOCK_METHOD3(setStartX, void(double startX, DatasetIndex dataIndex,
                               IDAWorkspaceIndex spectrum));
  MOCK_METHOD3(setEndX, void(double endX, DatasetIndex dataIndex,
                             IDAWorkspaceIndex spectrum));
  MOCK_METHOD3(setExcludeRegion,
               void(std::string const &exclude, DatasetIndex dataIndex,
                    IDAWorkspaceIndex spectrum));
  MOCK_CONST_METHOD2(getDomainIndex,
                     SpectrumRowIndex(DatasetIndex, IDAWorkspaceIndex));

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(DatasetIndex index,
                                  IDAWorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };

  std::vector<std::string> getSpectrumDependentAttributes() const override {
    return {};
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectDataTablePresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectDataTablePresenterTest() { FrameworkManager::Instance(); }

  static IndirectDataTablePresenterTest *createSuite() {
    return new IndirectDataTablePresenterTest();
  }

  static void destroySuite(IndirectDataTablePresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_model = std::make_unique<NiceMock<MockIndirectDataTableModel>>();
    m_table = createEmptyTableWidget(5, 5);

    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(5));
    m_model->addWorkspace("WorkspaceName");
    ON_CALL(*m_model, numberOfWorkspaces())
        .WillByDefault(Return(DatasetIndex{1}));

    m_presenter = std::make_unique<IndirectDataTablePresenter>(
        std::move(m_model.get()), std::move(m_table.get()));
    m_presenter->addData(DatasetIndex{0});
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

  void
  test_that_invoking_setStartX_will_alter_the_relevant_column_in_the_table() {
    TableItem const startX(2.2);
    ON_CALL(*m_model, getDomainIndex(DatasetIndex{0}, IDAWorkspaceIndex{2}))
        .WillByDefault(Return(SpectrumRowIndex{2}));

    m_presenter->setStartX(startX.asDouble(), DatasetIndex{0},
                           IDAWorkspaceIndex{2});

    assertValueIsNotGlobal(2, START_X_COLUMN, startX);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals call the correct methods
  ///----------------------------------------------------------------------

  void
  test_that_the_cellChanged_signal_will_set_the_models_startX_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model, setStartX(2.0, DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .Times(1);
    m_table->item(0, START_X_COLUMN)->setText("2.0");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_endX_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model, setEndX(2.0, DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .Times(1);
    m_table->item(0, END_X_COLUMN)->setText("2.0");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_excludeRegion_when_the_relevant_column_is_changed() {
    EXPECT_CALL(*m_model,
                setExcludeRegion("0-4", DatasetIndex{0}, IDAWorkspaceIndex{0}))
        .Times(1);
    m_table->item(0, EXCLUDE_REGION_COLUMN)->setText("0-4");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_startX_in_every_row_when_the_relevant_column_is_changed() {
    TableItem const startX(1.5);

    m_table->item(0, START_X_COLUMN)->setText(startX.asQString());

    assertValueIsNotGlobal(0, START_X_COLUMN, startX);
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_endX_in_every_row_when_the_relevant_column_is_changed() {
    TableItem const endX(2.5);

    m_table->item(1, END_X_COLUMN)->setText(endX.asQString());

    assertValueIsNotGlobal(1, END_X_COLUMN, endX);
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_excludeRegion_in_every_row_when_the_relevant_column_is_changed() {
    TableItem const excludeRegion("2-4");

    m_table->item(1, EXCLUDE_REGION_COLUMN)->setText(excludeRegion.asQString());

    assertValueIsNotGlobal(1, EXCLUDE_REGION_COLUMN, excludeRegion);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots of the presenter
  ///----------------------------------------------------------------------

  void
  test_that_tableDatasetsMatchModel_returns_false_if_the_number_of_data_positions_is_not_equal_to_the_numberOfWorkspaces() {
    DatasetIndex const numberOfWorkspaces{2};
    ON_CALL(*m_model, numberOfWorkspaces())
        .WillByDefault(Return(numberOfWorkspaces));

    EXPECT_CALL(*m_model, numberOfWorkspaces())
        .Times(1)
        .WillOnce(Return(numberOfWorkspaces));

    TS_ASSERT(!m_presenter->tableDatasetsMatchModel());
  }

  void
  test_that_tableDatasetsMatchModel_returns_true_if_the_table_datasets_match_the_model() {
    EXPECT_CALL(*m_model, numberOfWorkspaces())
        .Times(1)
        .WillOnce(Return(DatasetIndex{1}));
    TS_ASSERT(m_presenter->tableDatasetsMatchModel());
  }

  void
  test_that_the_setStartX_slot_will_alter_the_relevant_startX_column_in_the_table() {
    TableItem const startX(1.1);

    m_presenter->setStartX(startX.asDouble(), SpectrumRowIndex{0});

    assertValueIsNotGlobal(0, START_X_COLUMN, startX);
  }

  void
  test_that_the_setEndX_slot_will_alter_the_relevant_endX_column_in_the_table() {
    TableItem const endX(1.1);

    m_presenter->setEndX(endX.asDouble(), SpectrumRowIndex{0});

    assertValueIsNotGlobal(0, END_X_COLUMN, endX);
  }

  void
  test_that_the_setExcludeRegion_slot_will_alter_the_relevant_excludeRegion_column_in_the_table() {
    TableItem const excludeRegion("2-3");

    m_presenter->setExcludeRegion(excludeRegion.asString(),
                                  SpectrumRowIndex{0});

    assertValueIsGlobal(EXCLUDE_REGION_COLUMN, excludeRegion);
  }

  void
  test_that_setGlobalFittingRange_will_set_the_startX_and_endX_taken_from_the_fitting_range() {
    DatasetIndex const index{0};
    TableItem const startX(1.0);
    TableItem const endX(2.0);
    auto const range = std::make_pair(startX.asDouble(), endX.asDouble());

    ON_CALL(*m_model, getFittingRange(index, IDAWorkspaceIndex{0}))
        .WillByDefault(Return(range));

    EXPECT_CALL(*m_model, getFittingRange(index, IDAWorkspaceIndex{0}))
        .Times(1);

    m_presenter->setGlobalFittingRange(true);

    assertValueIsGlobal(START_X_COLUMN, startX);
    assertValueIsGlobal(END_X_COLUMN, endX);
  }

  void
  test_that_setGlobalFittingRange_will_set_the_excludeRegion_when_passed_true() {
    DatasetIndex const index{0};
    TableItem const excludeRegion("1-2");

    ON_CALL(*m_model, getExcludeRegion(index, IDAWorkspaceIndex{0}))
        .WillByDefault(Return("1-2"));

    EXPECT_CALL(*m_model, getExcludeRegion(index, IDAWorkspaceIndex{0}))
        .Times(1);

    m_presenter->setGlobalFittingRange(true);

    assertValueIsGlobal(EXCLUDE_REGION_COLUMN, excludeRegion);
  }

  void
  test_that_setGlobalFittingRange_will_connect_the_cellChanged_signal_to_updateAllFittingRangeFrom_when_passed_true() {
    TableItem const startX(1.0);

    m_presenter->setGlobalFittingRange(true);
    m_table->item(0, START_X_COLUMN)->setText(startX.asQString());

    assertValueIsGlobal(START_X_COLUMN, startX);
  }

  void
  test_that_setGlobalFittingRange_will_disconnect_the_cellChanged_signal_when_passed_false_so_that_startX_is_not_global() {
    int const row(1);
    TableItem const startX(2.5);

    m_presenter->setGlobalFittingRange(false);
    m_table->item(row, START_X_COLUMN)->setText(startX.asQString());

    assertValueIsNotGlobal(row, START_X_COLUMN, startX);
  }

  void
  test_that_setGlobalFittingRange_will_disconnect_the_cellChanged_signal_when_passed_false_so_that_endX_is_not_global() {
    int const row(0);
    TableItem const endX(3.5);

    m_presenter->setGlobalFittingRange(false);
    m_table->item(row, END_X_COLUMN)->setText(endX.asQString());

    assertValueIsNotGlobal(row, END_X_COLUMN, endX);
  }

  void test_the_enableTable_slot_will_enable_the_table() {
    m_presenter->disableTable();
    TS_ASSERT(!m_table->isEnabled());

    m_presenter->enableTable();
    TS_ASSERT(m_table->isEnabled());
  }

  void test_the_disableTable_slot_will_enable_the_table() {
    m_presenter->enableTable();
    TS_ASSERT(m_table->isEnabled());

    m_presenter->disableTable();
    TS_ASSERT(!m_table->isEnabled());
  }

  void test_that_clearTable_will_clear_the_data_table() {
    m_presenter->clearTable();
    TS_ASSERT_EQUALS(m_table->rowCount(), 0);
  }

private:
  void assertValueIsGlobal(int column, TableItem const &value) const {
    for (auto row = 0; row < m_table->rowCount(); ++row) {
      auto const item = getTableItem(row, column);
      TS_ASSERT_EQUALS(value, item);
    }
  }

  void assertValueIsNotGlobal(int valueRow, int column,
                              TableItem const &value) const {
    TS_ASSERT_EQUALS(value.asString(), getTableItem(valueRow, column));

    for (auto row = 0; row < m_table->rowCount(); ++row)
      if (row != valueRow)
        TS_ASSERT_DIFFERS(value, getTableItem(row, column));
  }

  std::string getTableItem(int row, int column) const {
    return m_table->item(row, column)->text().toStdString();
  }

  std::unique_ptr<QTableWidget> m_table;
  std::unique_ptr<MockIndirectDataTableModel> m_model;
  std::unique_ptr<IndirectDataTablePresenter> m_presenter;
};

#endif