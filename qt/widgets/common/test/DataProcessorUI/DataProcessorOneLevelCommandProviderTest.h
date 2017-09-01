#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
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

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorOneLevelTreeManagerTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorOneLevelTreeManagerTest *createSuite() {
    return new DataProcessorOneLevelTreeManagerTest();
  }
  static void destroySuite(DataProcessorOneLevelTreeManagerTest *suite) {
    delete suite;
  }

  template <typename T>
  bool notNullAndHasType(DataProcessorCommand_uptr const &ptr) const {
    return dynamic_cast<T *>(ptr.get()) != nullptr;
  }

  NiceMock<MockDataProcessorPresenter> m_presenter;
  DataProcessorOneLevelTreeManager m_manager;

  void setUp() override {
    m_manager = DataProcessorOneLevelTreeManager(&m_presenter,
                                                 DataProcessorWhiteList());
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_presenter));
  }

  void test_get_edit_commands() {
    auto &commands = m_manager.getEditCommands();

    TS_ASSERT(commands.size() > 12);
    TS_ASSERT(notNullAndHasType<DataProcessorProcessCommand>(commands[0]));
    TS_ASSERT(notNullAndHasType<DataProcessorPauseCommand>(commands[1]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[2]));
    TS_ASSERT(notNullAndHasType<DataProcessorPlotRowCommand>(commands[3]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[4]));
    TS_ASSERT(notNullAndHasType<DataProcessorAppendRowCommand>(commands[5]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[6]));
    TS_ASSERT(notNullAndHasType<DataProcessorCopySelectedCommand>(commands[7]));
    TS_ASSERT(notNullAndHasType<DataProcessorCutSelectedCommand>(commands[8]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorPasteSelectedCommand>(commands[9]));
    TS_ASSERT(
        notNullAndHasType<DataProcessorClearSelectedCommand>(commands[10]));
    TS_ASSERT(notNullAndHasType<DataProcessorSeparatorCommand>(commands[11]));
    TS_ASSERT(notNullAndHasType<DataProcessorDeleteRowCommand>(commands[12]));
  }

  int indexOf(EditAction action) { return m_manager.indexOfCommand(action); }

  void test_index_of_edit_commands() {
    TS_ASSERT(commands.size() > 12);
    TS_ASSERT_EQUALS(0, indexOf(EditAction::PROCESS));
    TS_ASSERT_EQUALS(1, indexOf(EditAction::PAUSE));
    TS_ASSERT_EQUALS(3, indexOf(EditAction::PLOT_ROW));
    TS_ASSERT_EQUALS(5, indexOf(EditAction::APPEND_ROW));
    TS_ASSERT_EQUALS(7, indexOf(EditAction::COPY_SELECTION));
    TS_ASSERT_EQUALS(8, indexOf(EditAction::CUT_SELECTION));
    TS_ASSERT_EQUALS(9, indexOf(EditAction::PASTE_SELECTION));
    TS_ASSERT_EQUALS(10, indexOf(EditAction::CLEAR_SELECTION));
    TS_ASSERT_EQUALS(12, indexOf(EditAction::DELETE_ROW));
  }

  int indexOf(TableAction action) { return m_manager.indexOfCommand(action); }

  void test_get_table_commands() {
    auto &commands = m_manager.getTableCommands();
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

  void test_index_of_table_commands() {
    auto &commands = m_manager.getTableCommands();
    TS_ASSERT(commands.size() > 8);
    TS_ASSERT_EQUALS(0, indexOf(EditAction::OPEN_TABLE));
    TS_ASSERT_EQUALS(1, indexOf(EditAction::NEW_TABLE));
    TS_ASSERT_EQUALS(2, indexOf(EditAction::SAVE_TABLE));
    TS_ASSERT_EQUALS(3, indexOf(EditAction::SAVE_AS_TABLE));
    TS_ASSERT_EQUALS(5, indexOf(EditAction::IMPORT_TBL_FILE));
    TS_ASSERT_EQUALS(6, indexOf(EditAction::EXPORT_TBL_FILE));
    TS_ASSERT_EQUALS(8, indexOf(EditAction::OPTIONS));
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORONELEVELTREEMANAGERTEST_H */
