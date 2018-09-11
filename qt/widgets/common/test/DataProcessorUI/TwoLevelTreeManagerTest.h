#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CollapseGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DeleteGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExpandCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExpandGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GroupRowsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/MockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PauseCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PlotGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SeparatorCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TwoLevelTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/WhiteList.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace testing;
using Runs = std::vector<std::map<QString, QString>>;

//=====================================================================================
// Functional tests
//=====================================================================================
class TwoLevelTreeManagerTest : public CxxTest::TestSuite {

private:
  // Return a reflectometry whitelist
  WhiteList reflWhitelist() {

    // Reflectometry white list
    WhiteList whitelist;
    whitelist.addElement("Run(s)", "InputWorkspace", "", true, "TOF_");
    whitelist.addElement("Angle", "ThetaIn", "");
    whitelist.addElement("Transmission Run(s)", "FirstTransmissionRun", "",
                         true, "TRANS_");
    whitelist.addElement("Q min", "MomentumTransferMinimum", "");
    whitelist.addElement("Q max", "MomentumTransferMaximum", "");
    whitelist.addElement("dQ/Q", "MomentumTransferStep", "");
    whitelist.addElement("Scale", "ScaleFactor", "");
    whitelist.addElement("Options", "Options", "");
    return whitelist;
  }

  ITableWorkspace_sptr reflTable() {
    ITableWorkspace_sptr ws = WorkspaceFactory::Instance().createTable();

    ws->addColumn("str", "Group");
    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "Angle");
    ws->addColumn("str", "Transmission Run(s)");
    ws->addColumn("str", "Q min");
    ws->addColumn("str", "Q max");
    ws->addColumn("str", "dQ/Q");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");

    TableRow row = ws->appendRow();
    row << "0"
        << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "0"
        << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "1"
        << "24682"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    return ws;
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static TwoLevelTreeManagerTest *createSuite() {
    return new TwoLevelTreeManagerTest();
  }
  static void destroySuite(TwoLevelTreeManagerTest *suite) { delete suite; }

  void test_publish_commands() {
    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, WhiteList());

    auto comm = manager.publishCommands();

    TS_ASSERT_EQUALS(comm.size(), 31);
    TS_ASSERT(dynamic_cast<OpenTableCommand *>(comm[0].get()));
    TS_ASSERT(dynamic_cast<NewTableCommand *>(comm[1].get()));
    TS_ASSERT(dynamic_cast<SaveTableCommand *>(comm[2].get()));
    TS_ASSERT(dynamic_cast<SaveTableAsCommand *>(comm[3].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[4].get()));
    TS_ASSERT(dynamic_cast<ImportTableCommand *>(comm[5].get()));
    TS_ASSERT(dynamic_cast<ExportTableCommand *>(comm[6].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[7].get()));
    TS_ASSERT(dynamic_cast<OptionsCommand *>(comm[8].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[9].get()));
    TS_ASSERT(dynamic_cast<ProcessCommand *>(comm[10].get()));
    TS_ASSERT(dynamic_cast<PauseCommand *>(comm[11].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[12].get()));
    TS_ASSERT(dynamic_cast<ExpandCommand *>(comm[13].get()));
    TS_ASSERT(dynamic_cast<ExpandGroupsCommand *>(comm[14].get()));
    TS_ASSERT(dynamic_cast<CollapseGroupsCommand *>(comm[15].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[16].get()));
    TS_ASSERT(dynamic_cast<PlotRowCommand *>(comm[17].get()));
    TS_ASSERT(dynamic_cast<PlotGroupCommand *>(comm[18].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[19].get()));
    TS_ASSERT(dynamic_cast<AppendRowCommand *>(comm[20].get()));
    TS_ASSERT(dynamic_cast<AppendGroupCommand *>(comm[21].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[22].get()));
    TS_ASSERT(dynamic_cast<GroupRowsCommand *>(comm[23].get()));
    TS_ASSERT(dynamic_cast<CopySelectedCommand *>(comm[24].get()));
    TS_ASSERT(dynamic_cast<CutSelectedCommand *>(comm[25].get()));
    TS_ASSERT(dynamic_cast<PasteSelectedCommand *>(comm[26].get()));
    TS_ASSERT(dynamic_cast<ClearSelectedCommand *>(comm[27].get()));
    TS_ASSERT(dynamic_cast<SeparatorCommand *>(comm[28].get()));
    TS_ASSERT(dynamic_cast<DeleteRowCommand *>(comm[29].get()));
    TS_ASSERT(dynamic_cast<DeleteGroupCommand *>(comm[30].get()));
  }

  void test_append_row() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.deleteGroup());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_delete_all() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.deleteAll());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_expand_selection() {
    // This is well tested in GenericDataProcessorPresenterTest, hence just
    // checking that the presenter is called

    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.pasteSelected(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_new_table() {
    NiceMock<MockDataProcessorPresenter> presenter;
    auto table = reflTable();
    auto whitelist = reflWhitelist();
    TwoLevelTreeManager manager(&presenter, whitelist);
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
    TS_ASSERT_EQUALS(data[0][0]->data(), firstRow);
    TS_ASSERT_EQUALS(data[0][1]->data(), secondRow);
    TS_ASSERT_EQUALS(data[1][0]->data(), thirdRow);
    TS_ASSERT_EQUALS(data[1][1]->data(), fourthRow);
  }

  void test_new_table_clears_model() {
    NiceMock<MockDataProcessorPresenter> presenter;
    auto table = reflTable();
    auto whitelist = reflWhitelist();
    TwoLevelTreeManager manager(&presenter, whitelist);

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
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Runs", "12345"}}};
    TS_ASSERT_THROWS_ANYTHING(manager.transfer(runs));
  }

  void test_transfer_nothing_transferred() {
    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Group", "0"}, {"Runs", "12345"}}};
    TS_ASSERT_THROWS_NOTHING(manager.transfer(runs));
  }

  void test_transfer_good_data() {
    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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
    TS_ASSERT_THROWS_NOTHING(manager.transfer(runs));

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

    TS_ASSERT_EQUALS(data[0][0]->data(), firstRow);
    TS_ASSERT_EQUALS(data[0][1]->data(), secondRow);
    TS_ASSERT_EQUALS(data[1][0]->data(), thirdRow);
    TS_ASSERT_EQUALS(data[1][1]->data(), fourthRow);
  }

  void test_update() {
    NiceMock<MockDataProcessorPresenter> presenter;
    TwoLevelTreeManager manager(&presenter, reflWhitelist());

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

    TS_ASSERT_EQUALS(data[0][0]->data(), newRow);
    TS_ASSERT_EQUALS(data[0][1]->data(), newRow);
    TS_ASSERT_EQUALS(data[1][0]->data(), newRow);
    TS_ASSERT_EQUALS(data[1][1]->data(), newRow);
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H */
