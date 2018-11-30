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

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the model
class MockIndirectDataTableModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD2(getFittingRange,
                     std::pair<double, double>(std::size_t dataIndex,
                                               std::size_t spectrum));
  MOCK_CONST_METHOD2(getExcludeRegion,
                     std::string(std::size_t dataIndex, std::size_t index));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(numberOfWorkspaces, std::size_t());

  MOCK_METHOD3(setStartX, void(double startX, std::size_t dataIndex,
                               std::size_t spectrum));
  MOCK_METHOD3(setEndX,
               void(double endX, std::size_t dataIndex, std::size_t spectrum));
  MOCK_METHOD3(setExcludeRegion,
               void(std::string const &exclude, std::size_t dataIndex,
                    std::size_t spectrum));

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
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
    createEmptyTableWidget(5, 5);
    m_presenter = std::make_unique<IndirectDataTablePresenter>(
        std::move(m_model.get()), std::move(m_table.get()));

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

  void
  test_that_invoking_setStartX_will_alter_the_relevant_column_in_the_table() {
    int const startXColumn(2);

    m_presenter->setStartX(2.2, 0, 0);

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, startXColumn)->text().toStdString(),
                       "2.2")
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals call the correct methods
  ///----------------------------------------------------------------------

  void
  test_that_the_cellChanged_signal_will_set_the_models_startX_when_the_relevant_column_is_changed() {
    int const startXColumn(2);

    EXPECT_CALL(*m_model, setStartX(2.0, 0, 0)).Times(1);

    m_table->item(0, startXColumn)->setText("2.0");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_endX_when_the_relevant_column_is_changed() {
    int const endXColumn(3);

    EXPECT_CALL(*m_model, setEndX(2.0, 0, 0)).Times(1);

    m_table->item(0, endXColumn)->setText("2.0");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_excludeRegion_when_the_relevant_column_is_changed() {
    int const excludeRegionColumn(4);

    EXPECT_CALL(*m_model, setExcludeRegion("0-4", 0, 0)).Times(1);

    m_table->item(0, excludeRegionColumn)->setText("0-4");
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_startX_in_every_row_when_the_relevant_column_is_changed() {
    int const startXColumn(2);

    m_table->item(0, startXColumn)->setText("1.5");

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, startXColumn)->text().toStdString(),
                       "1.5")
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_endX_in_every_row_when_the_relevant_column_is_changed() {
    int const endXColumn(3);

    m_table->item(0, endXColumn)->setText("2.5");

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, endXColumn)->text().toStdString(),
                       "2.5")
  }

  void
  test_that_the_cellChanged_signal_will_set_the_models_excludeRegion_in_every_row_when_the_relevant_column_is_changed() {
    int const excludeRegionColumn(4);

    m_table->item(0, excludeRegionColumn)->setText("2-4");

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(
          m_table->item(row, excludeRegionColumn)->text().toStdString(), "2-4")
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots of the presenter
  ///----------------------------------------------------------------------

  void
  test_that_tableDatasetsMatchModel_returns_false_if_the_number_of_data_positions_is_not_equal_to_the_numberOfWorkspaces() {
    std::size_t const numberOfWorkspaces(2);
    ON_CALL(*m_model, numberOfWorkspaces())
        .WillByDefault(Return(numberOfWorkspaces));

    EXPECT_CALL(*m_model, numberOfWorkspaces())
        .Times(1)
        .WillOnce(Return(numberOfWorkspaces));

    TS_ASSERT(!m_presenter->tableDatasetsMatchModel());
  }

  void
  test_that_tableDatasetsMatchModel_returns_true_if_the_table_datasets_match_the_model() {
    EXPECT_CALL(*m_model, numberOfWorkspaces()).Times(1).WillOnce(Return(0));
    TS_ASSERT(m_presenter->tableDatasetsMatchModel());
  }

  void
  test_that_addData_will_add_new_data_if_the_index_is_smaller_than_the_number_of_data_positions() {
    std::size_t const index(0);

    ON_CALL(*m_model, numberOfWorkspaces()).WillByDefault(Return(2));

    EXPECT_CALL(*m_model, numberOfWorkspaces()).Times(1);

    ExpectationSet getRanges =
        EXPECT_CALL(*m_model, getFittingRange(index, 0)).Times(1);
    for (auto spectrum = 1; spectrum < m_table->rowCount(); ++spectrum)
      getRanges += EXPECT_CALL(*m_model, getFittingRange(index, spectrum))
                       .Times(1)
                       .After(getRanges);

    m_presenter->addData(index);
  }

  void
  test_that_the_setStartX_slot_will_alter_the_relevant_startX_column_in_the_table() {
    int const startXColumn(2);

    m_presenter->setStartX(1.1, 0);

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, startXColumn)->text().toStdString(),
                       "1.1")
  }

  void
  test_that_the_setEndX_slot_will_alter_the_relevant_endX_column_in_the_table() {
    int const endXColumn(3);

    m_presenter->setEndX(1.1, 0);

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, endXColumn)->text().toStdString(),
                       "1.1")
  }

  void
  test_that_the_setExcludeRegion_slot_will_alter_the_relevant_excludeRegion_column_in_the_table() {
    int const excludeRegionColumn(4);

    m_presenter->setExcludeRegion("2-3", 0);

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(
          m_table->item(row, excludeRegionColumn)->text().toStdString(), "2-3")
  }

  void
  test_that_setGlobalFittingRange_will_set_the_startX_and_endX_taken_from_the_fitting_range() {
    std::size_t const index(0);
    int const startXColumn(2);
    int const endXColumn(3);
    auto const range = std::make_pair(1.0, 2.0);

    ON_CALL(*m_model, getFittingRange(index, 0)).WillByDefault(Return(range));

    EXPECT_CALL(*m_model, getFittingRange(index, 0)).Times(1);

    m_presenter->setGlobalFittingRange(true);

    for (auto row = 0; row < m_table->rowCount(); ++row) {
      TS_ASSERT_EQUALS(m_table->item(row, startXColumn)->text().toDouble(), 1.0)
      TS_ASSERT_EQUALS(m_table->item(row, endXColumn)->text().toDouble(), 2.0)
    }
  }

  void
  test_that_setGlobalFittingRange_will_set_the_excludeRegion_when_passed_true() {
    std::size_t const index(0);
    int const excludeRegionColumn(4);

    ON_CALL(*m_model, getExcludeRegion(index, 0)).WillByDefault(Return("1-2"));

    EXPECT_CALL(*m_model, getExcludeRegion(index, 0)).Times(1);

    m_presenter->setGlobalFittingRange(true);

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(
          m_table->item(row, excludeRegionColumn)->text().toStdString(), "1-2");
  }

  void
  test_that_setGlobalFittingRange_will_connect_the_cellChanged_signal_to_updateAllFittingRangeFrom_when_passed_true() {
    int const startXColumn(2);

    m_presenter->setGlobalFittingRange(true);
    m_table->item(0, startXColumn)->setText("1.5");

    for (auto row = 0; row < m_table->rowCount(); ++row)
      TS_ASSERT_EQUALS(m_table->item(row, startXColumn)->text().toStdString(),
                       "1.5")
  }

  void
  test_that_setGlobalFittingRange_will_disconnect_the_cellChanged_signal_when_passed_false_so_that_startX_is_not_global() {
    int const startXColumn(2);
    std::string const startX("2.5");

    m_presenter->setGlobalFittingRange(false);
    m_table->item(0, startXColumn)->setText(QString::fromStdString(startX));

    TS_ASSERT_EQUALS(m_table->item(0, startXColumn)->text().toStdString(),
                     startX);
    for (auto row = 1; row < m_table->rowCount(); ++row)
      TS_ASSERT_DIFFERS(m_table->item(row, startXColumn)->text().toStdString(),
                        startX)
  }

  void
  test_that_setGlobalFittingRange_will_disconnect_the_cellChanged_signal_when_passed_false_so_that_endX_is_not_global() {
    int const endXColumn(3);
    std::string const endX("2.5");

    m_presenter->setGlobalFittingRange(false);
    m_table->item(0, endXColumn)->setText(QString::fromStdString(endX));

    TS_ASSERT_EQUALS(m_table->item(0, endXColumn)->text().toStdString(), endX);
    for (auto row = 1; row < m_table->rowCount(); ++row)
      TS_ASSERT_DIFFERS(m_table->item(row, endXColumn)->text().toStdString(),
                        endX)
  }

  void test_the_enableTable_slot_will_enable_the_table() {
    m_presenter->disableTable();
    m_presenter->enableTable();

    TS_ASSERT(m_table->isEnabled());
  }

  void test_the_disableTable_slot_will_enable_the_table() {
    m_presenter->enableTable();
    m_presenter->disableTable();

    TS_ASSERT(!m_table->isEnabled());
  }

  void test_that_clearTable_will_clear_the_data_table() {
    m_presenter->clearTable();
    TS_ASSERT_EQUALS(m_table->rowCount(), 0);
  }

private:
  /// Used in setup
  void createEmptyTableWidget(int columns, int rows) {
    m_table = std::make_unique<QTableWidget>(columns, rows);
    for (auto column = 0; column < columns; ++column)
      for (auto row = 0; row < rows; ++row)
        m_table->setItem(row, column, new QTableWidgetItem("item"));
  }

  std::unique_ptr<QTableWidget> m_table;
  std::unique_ptr<MockIndirectDataTableModel> m_model;
  std::unique_ptr<IndirectDataTablePresenter> m_presenter;
};

#endif