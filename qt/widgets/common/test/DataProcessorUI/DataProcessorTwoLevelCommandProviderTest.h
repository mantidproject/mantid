#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCollapseGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorDeleteGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExpandCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExpandGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorGroupRowsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPauseCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPlotGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorWhiteList.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;
using Runs = std::vector<std::map<QString, QString>>;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorTwoLevelTreeManagerTest : public CxxTest::TestSuite {
public:
  NiceMock<MockDataProcessorPresenter> m_presenter;
  DataProcessorTwoLevelTreeManager m_manager;
    
  void setUp() override {
    m_manager = DataProcessorTwoLevelTreeManager();
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_presenter));
  }

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorTwoLevelTreeManagerTest *createSuite() {
    return new DataProcessorTwoLevelTreeManagerTest();
  }
  static void destroySuite(DataProcessorTwoLevelTreeManagerTest *suite) {
    delete suite;
  }

  template <typename T>
  bool notNullAndHasType(DataProcessorCommand_uptr const &ptr) const {
    return dynamic_cast<T *>(ptr.get()) != nullptr;
  }

  int indexOf(EditAction action) {
    return m_manager.indexOfCommand(action);
  }

  void test_get_table_commands() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter,
                                             DataProcessorWhiteList());

    auto &commands = manager.getTableCommands();

    TS_ASSERT(commands.size() > 8);
    TS_ASSERT(notNullAndHasType<DataProcessorOpenTableCommand>(commands[0]));
    TS_ASSERT(notNullAndHasType<DataProcessorNewTableCommand>(commands[1]));
    TS_ASSERT(notNullAndHasType<DataProcessorSaveTableCommand>(commands[2]));
    TS_ASSERT(notNullAndHasType<DataProcessorSaveTableAsCommand>(commands[3]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[4]));
    TS_ASSERT(notNullAndHasType<DataProcessorImportTableCommand>(commands[5]));
    TS_ASSERT(notNullAndHasType<DataProcessorExportTableCommand>(commands[6]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[7]));
    TS_ASSERT(notNullAndHasType<DataProcessorOptionsCommand>(commands[8]));
  }

  int indexOf()

  void test_get_edit_commands() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter,
                                             DataProcessorWhiteList());

    auto &commands = manager.getEditCommands();

    TS_ASSERT(commands.size() > 12);
    TS_ASSERT(notNullAndHasType<DataProcessorProcessCommand>(commands[0]));
    TS_ASSERT(notNullAndHasType<DataProcessorPauseCommand>(commands[1]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[2]));
    TS_ASSERT(notNullAndHasType<DataProcessorExpandCommand>(commands[3]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorExpandGroupsCommand>(commands[4]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorCollapseGroupsCommand>(commands[5]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[6]));
    TS_ASSERT(notNullAndHasType<DataProcessorPlotRowCommand>(commands[7]));
    TS_ASSERT(notNullAndHasType<DataProcessorPlotGroupCommand>(commands[8]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[9]));
    TS_ASSERT(notNullAndHasType<DataProcessorAppendRowCommand>(commands[10]));
    TS_ASSERT(notNullAndHasType<DataProcessorAppendGroupCommand>(commands[11]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[12]));
    TS_ASSERT(notNullAndHasType<DataProcessorGroupRowsCommand>(commands[13]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorCopySelectedCommand>(commands[14]));
    TS_ASSERT(notNullAndHasType<DataProcessorCutSelectedCommand>(commands[15]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorPasteSelectedCommand>(commands[16]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorClearSelectedCommand>(commands[17]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[18]));
    TS_ASSERT(notNullAndHasType<DataProcessorDeleteRowCommand>(commands[19]));
    TS_ASSERT(notNullAndHasType<DataProcessorDeleteGroupCommand>(commands[20]));
  }

  void test_append_row() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    TS_ASSERT_THROWS_NOTHING(manager.appendRow());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_append_group() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.appendGroup());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_delete_row() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    TS_ASSERT_THROWS_NOTHING(manager.deleteRow());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_delete_group() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.deleteGroup());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_expand_selection() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    TS_ASSERT_THROWS_NOTHING(manager.expandSelection());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_clear_selected() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    TS_ASSERT_THROWS_NOTHING(manager.clearSelected());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_copy_selected() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    TS_ASSERT_THROWS_NOTHING(manager.copySelected());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_paste_selected() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.pasteSelected(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_new_table() {
    NiceMock<MockDataProcessorPresenter> presenter;
    auto table = reflTable();
    auto whitelist = reflWhitelist();
    DataProcessorTwoLevelTreeManager manager(&presenter, whitelist);
    TS_ASSERT_THROWS_NOTHING(manager.newTable(table, whitelist));

    QStringList firstRow = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    QStringList secondRow = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    QStringList thirdRow = {"24681", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    QStringList fourthRow = {"24682", "1.5", "", "1.4", "2.9", "0.04", "1", ""};

    // Check that runs have been transferred correctly
    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data.size(), 2);
    TS_ASSERT_EQUALS(data[0][0], firstRow);
    TS_ASSERT_EQUALS(data[0][1], secondRow);
    TS_ASSERT_EQUALS(data[1][0], thirdRow);
    TS_ASSERT_EQUALS(data[1][1], fourthRow);
  }

  void test_new_table_clears_model() {
    NiceMock<MockDataProcessorPresenter> presenter;
    auto table = reflTable();
    auto whitelist = reflWhitelist();
    DataProcessorTwoLevelTreeManager manager(&presenter, whitelist);

    TS_ASSERT_THROWS_NOTHING(manager.newTable(table, whitelist));
    TS_ASSERT_EQUALS(manager.getTableWorkspace()->rowCount(), 4);

    TS_ASSERT_THROWS_NOTHING(manager.newTable(whitelist));
    auto ws = manager.getTableWorkspace();
    TS_ASSERT_EQUALS(ws->rowCount(), 1);
    TS_ASSERT_EQUALS(ws->columnCount(), whitelist.size() + 1);
    // But the row should be empty
    TS_ASSERT_EQUALS(ws->String(0, 0), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 1), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 2), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 3), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 4), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 5), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 6), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 7), std::string());
    TS_ASSERT_EQUALS(ws->String(0, 8), std::string());
  }

  void test_transfer_fails_no_group() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Runs", "12345"}}};
    TS_ASSERT_THROWS_ANYTHING(manager.transfer(runs, reflWhitelist()));
  }

  void test_transfer_fails_wrong_whitelist() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Group", "0"}, {"Runs", "12345"}}};
    TS_ASSERT_THROWS_ANYTHING(manager.transfer(runs, DataProcessorWhiteList()));
  }

  void test_transfer_nothing_transferred() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Group", "0"}, {"Runs", "12345"}}};
    TS_ASSERT_THROWS_NOTHING(manager.transfer(runs, reflWhitelist()));
  }

  void test_transfer_good_data() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Group", "Group0"},
                  {"Run(s)", "12345"},
                  {"Angle", "0.5"},
                  {"Transmission Run(s)", "20000"},
                  {"Q min", "0.1"},
                  {"Q max", "0.2"},
                  {"dQ/Q", "0.04"},
                  {"Scale", "5"},
                  {"Options", "CorrectDetectorPositions=1"}},
                 {{"Group", "Group0"},
                  {"Run(s)", "12346"},
                  {"Angle", "0.6"},
                  {"Transmission Run(s)", "20001"},
                  {"Q min", "0.1"},
                  {"Q max", "0.2"},
                  {"dQ/Q", "0.04"},
                  {"Scale", "4"},
                  {"Options", "CorrectDetectorPositions=0"}},
                 {{"Group", "Group1"},
                  {"Run(s)", "12347"},
                  {"Angle", "0.7"},
                  {"Transmission Run(s)", "20003"},
                  {"Q min", "0.3"},
                  {"Q max", "0.4"},
                  {"dQ/Q", "0.01"},
                  {"Scale", "3"},
                  {"Options", ""}},
                 {{"Group", "Group1"},
                  {"Run(s)", "12348"},
                  {"Angle", "0.8"},
                  {"Transmission Run(s)", "20004"},
                  {"Q min", "0.4"},
                  {"Q max", "0.5"},
                  {"dQ/Q", "0.02"},
                  {"Scale", "2"},
                  {"Options", ""}}};
    TS_ASSERT_THROWS_NOTHING(manager.transfer(runs, reflWhitelist()));

    // Check that runs have been transferred correctly
    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data.size(), 2);
    QStringList firstRow = {
        "12345", "0.5",  "20000", "0.1",
        "0.2",   "0.04", "5",     "CorrectDetectorPositions=1"};
    QStringList secondRow = {
        "12346", "0.6",  "20001", "0.1",
        "0.2",   "0.04", "4",     "CorrectDetectorPositions=0"};
    QStringList thirdRow = {"12347", "0.7",  "20003", "0.3",
                            "0.4",   "0.01", "3",     ""};
    QStringList fourthRow = {"12348", "0.8",  "20004", "0.4",
                             "0.5",   "0.02", "2",     ""};

    TS_ASSERT_EQUALS(data[0][0], firstRow);
    TS_ASSERT_EQUALS(data[0][1], secondRow);
    TS_ASSERT_EQUALS(data[1][0], thirdRow);
    TS_ASSERT_EQUALS(data[1][1], fourthRow);
  }

  void test_update() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter, reflWhitelist());

    QStringList newRow = {"0", "1", "2", "3", "4", "5", "6", "7"};

    TS_ASSERT_THROWS_NOTHING(manager.newTable(reflTable(), reflWhitelist()));
    TS_ASSERT_THROWS_NOTHING(manager.update(0, 0, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(0, 1, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(1, 0, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(1, 1, newRow));
    // Check that runs have been updated correctly
    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren())
        .Times(1)
        .WillOnce(Return(std::map<int, std::set<int>>()));
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data[0][0], newRow);
    TS_ASSERT_EQUALS(data[0][1], newRow);
    TS_ASSERT_EQUALS(data[1][0], newRow);
    TS_ASSERT_EQUALS(data[1][1], newRow);
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H */
