#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPauseCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorWhiteList.h"

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets;
using namespace testing;
using Runs = std::vector<std::map<QString, QString>>;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorOneLevelTreeManagerTest : public CxxTest::TestSuite {

private:
  // Return a reflectometry whitelist
  DataProcessorWhiteList reflWhitelist() {

    // Reflectometry white list
    DataProcessorWhiteList whitelist;
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

    ws->addColumn("str", "Run(s)");
    ws->addColumn("str", "Angle");
    ws->addColumn("str", "Transmission Run(s)");
    ws->addColumn("str", "Q min");
    ws->addColumn("str", "Q max");
    ws->addColumn("str", "dQ/Q");
    ws->addColumn("str", "Scale");
    ws->addColumn("str", "Options");

    TableRow row = ws->appendRow();
    row << "12345"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "12346"
        << "1.5"
        << ""
        << "1.4"
        << "2.9"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "24681"
        << "0.5"
        << ""
        << "0.1"
        << "1.6"
        << "0.04"
        << "1"
        << "";
    row = ws->appendRow();
    row << "24682"
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
  static DataProcessorOneLevelTreeManagerTest *createSuite() {
    return new DataProcessorOneLevelTreeManagerTest();
  }
  static void destroySuite(DataProcessorOneLevelTreeManagerTest *suite) {
    delete suite;
  }

  void test_publish_commands() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter,
                                             DataProcessorWhiteList());

    auto comm = manager.publishCommands();

    TS_ASSERT_EQUALS(comm.size(), 23);
    TS_ASSERT(dynamic_cast<DataProcessorOpenTableCommand *>(comm[0].get()));
    TS_ASSERT(dynamic_cast<DataProcessorNewTableCommand *>(comm[1].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSaveTableCommand *>(comm[2].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSaveTableAsCommand *>(comm[3].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[4].get()));
    TS_ASSERT(dynamic_cast<DataProcessorImportTableCommand *>(comm[5].get()));
    TS_ASSERT(dynamic_cast<DataProcessorExportTableCommand *>(comm[6].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[7].get()));
    TS_ASSERT(dynamic_cast<DataProcessorOptionsCommand *>(comm[8].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[9].get()));
    TS_ASSERT(dynamic_cast<DataProcessorProcessCommand *>(comm[10].get()));
    TS_ASSERT(dynamic_cast<DataProcessorPauseCommand *>(comm[11].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[12].get()));
    TS_ASSERT(dynamic_cast<DataProcessorPlotRowCommand *>(comm[13].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[14].get()));
    TS_ASSERT(dynamic_cast<DataProcessorAppendRowCommand *>(comm[15].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[16].get()));
    TS_ASSERT(dynamic_cast<DataProcessorCopySelectedCommand *>(comm[17].get()));
    TS_ASSERT(dynamic_cast<DataProcessorCutSelectedCommand *>(comm[18].get()));
    TS_ASSERT(
        dynamic_cast<DataProcessorPasteSelectedCommand *>(comm[19].get()));
    TS_ASSERT(
        dynamic_cast<DataProcessorClearSelectedCommand *>(comm[20].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[21].get()));
    TS_ASSERT(dynamic_cast<DataProcessorDeleteRowCommand *>(comm[22].get()));
  }

  void test_append_row() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.appendRow());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_append_group() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());
    TS_ASSERT_THROWS_ANYTHING(manager.appendGroup());
  }

  void test_delete_row_when_table_is_empty() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);

    TS_ASSERT_THROWS_NOTHING(manager.deleteRow());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_delete_row_with_populated_table() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(3)
        .WillOnce(Return(std::set<int>{0, 1}))
        .WillOnce(Return(std::set<int>{0}))
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);

    TS_ASSERT_THROWS_NOTHING(manager.deleteRow());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }


  void test_delete_group() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());
    TS_ASSERT_THROWS_ANYTHING(manager.deleteGroup());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_expand_selection() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());
    TS_ASSERT_THROWS_ANYTHING(manager.expandSelection());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_clear_selected() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.clearSelected());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_copy_selected() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.copySelected());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_paste_selected() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());
    EXPECT_CALL(presenter, selectedParents()).Times(0);
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    TS_ASSERT_THROWS_NOTHING(manager.pasteSelected(""));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));
  }

  void test_new_table() {
    NiceMock<MockDataProcessorPresenter> presenter;
    auto table = reflTable();
    auto whitelist = reflWhitelist();
    DataProcessorOneLevelTreeManager manager(&presenter, whitelist);
    TS_ASSERT_THROWS_NOTHING(manager.newTable(table, whitelist));

    QStringList firstRow = {"12345", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    QStringList secondRow = {"12346", "1.5", "", "1.4", "2.9", "0.04", "1", ""};
    QStringList thirdRow = {"24681", "0.5", "", "0.1", "1.6", "0.04", "1", ""};
    QStringList fourthRow = {"24682", "1.5", "", "1.4", "2.9", "0.04", "1", ""};

    // Check that runs have been transferred correctly
    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data.size(), 4);
    TS_ASSERT_EQUALS(data[0][0], firstRow);
    TS_ASSERT_EQUALS(data[1][1], secondRow);
    TS_ASSERT_EQUALS(data[2][2], thirdRow);
    TS_ASSERT_EQUALS(data[3][3], fourthRow);
  }

  void test_transfer_fails_wrong_whitelist() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Group", "0"}, {"Runs", "12345"}}};
    TS_ASSERT_THROWS_ANYTHING(manager.transfer(runs, DataProcessorWhiteList()));
  }

  void test_transfer_good_data() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    Runs runs = {{{"Run(s)", "12345"},
                  {"Angle", "0.5"},
                  {"Transmission Run(s)", "20000"},
                  {"Q min", "0.1"},
                  {"Q max", "0.2"},
                  {"dQ/Q", "0.04"},
                  {"Scale", "5"},
                  {"Options", "CorrectDetectorPositions=1"}},
                 {{"Run(s)", "12346"},
                  {"Angle", "0.6"},
                  {"Transmission Run(s)", "20001"},
                  {"Q min", "0.1"},
                  {"Q max", "0.2"},
                  {"dQ/Q", "0.04"},
                  {"Scale", "4"},
                  {"Options", "CorrectDetectorPositions=0"}},
                 {{"Run(s)", "12347"},
                  {"Angle", "0.7"},
                  {"Transmission Run(s)", "20003"},
                  {"Q min", "0.3"},
                  {"Q max", "0.4"},
                  {"dQ/Q", "0.01"},
                  {"Scale", "3"},
                  {"Options", ""}},
                 {{"Run(s)", "12348"},
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
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data.size(), 4);
    QStringList firstRow = {"12345", "0.5",                       "20000",
                            "0.1",   "0.2",                       "0.04",
                            "5",     "CorrectDetectorPositions=1"};
    QStringList secondRow = {"12346", "0.6",                       "20001",
                             "0.1",   "0.2",                       "0.04",
                             "4",     "CorrectDetectorPositions=0"};
    QStringList thirdRow = {"12347", "0.7",  "20003", "0.3",
                            "0.4",   "0.01", "3",     ""};
    QStringList fourthRow = {"12348", "0.8",  "20004", "0.4",
                             "0.5",   "0.02", "2",     ""};
    TS_ASSERT_EQUALS(data[0][0], firstRow);
    TS_ASSERT_EQUALS(data[1][1], secondRow);
    TS_ASSERT_EQUALS(data[2][2], thirdRow);
    TS_ASSERT_EQUALS(data[3][3], fourthRow);
  }

  void test_update() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorOneLevelTreeManager manager(&presenter, reflWhitelist());

    QStringList newRow = {"0", "1", "2", "3", "4", "5", "6", "7"};

    TS_ASSERT_THROWS_NOTHING(manager.newTable(reflTable(), reflWhitelist()));
    TS_ASSERT_THROWS_NOTHING(manager.update(0, 0, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(1, 0, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(2, 0, newRow));
    TS_ASSERT_THROWS_NOTHING(manager.update(3, 0, newRow));
    // Check that runs have been updated correctly
    EXPECT_CALL(presenter, selectedParents())
        .Times(1)
        .WillOnce(Return(std::set<int>()));
    EXPECT_CALL(presenter, selectedChildren()).Times(0);
    auto data = manager.selectedData(false);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&presenter));

    TS_ASSERT_EQUALS(data[0][0], newRow);
    TS_ASSERT_EQUALS(data[1][1], newRow);
    TS_ASSERT_EQUALS(data[2][2], newRow);
    TS_ASSERT_EQUALS(data[3][3], newRow);
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H */
