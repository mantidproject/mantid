#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CollapseGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorPresenter.h"
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
#include "MantidQtWidgets/Common/DataProcessorUI/WorkspaceCommand.h"

using namespace MantidQt::MantidWidgets;
using namespace MantidQt::MantidWidgets::DataProcessor;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class CommandsTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CommandsTest *createSuite() { return new CommandsTest(); }
  static void destroySuite(CommandsTest *suite) { delete suite; }

  void test_open_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    OpenTableCommand command(&mockPresenter);

    // The presenter should not be notified with the OpenTableFlag because
    // this is a submenu rather than an action
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::OpenTableFlag))
        .Times(Exactly(0));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_new_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    NewTableCommand command(&mockPresenter);

    // The presenter should be notified with the NewTableFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::NewTableFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_save_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    SaveTableCommand command(&mockPresenter);

    // The presenter should be notified with the SaveFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::SaveFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_save_table_as_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    SaveTableAsCommand command(&mockPresenter);

    // The presenter should be notified with the SaveAsFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::SaveAsFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_import_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ImportTableCommand command(&mockPresenter);

    // The presenter should be notified with the ImportTableFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::ImportTableFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_export_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ExportTableCommand command(&mockPresenter);

    // The presenter should be notified with the ExportTableFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::ExportTableFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_options_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    OptionsCommand command(&mockPresenter);

    // The presenter should be notified with the OptionsDialogFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::OptionsDialogFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_process_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ProcessCommand command(&mockPresenter);

    // The presenter should be notified with the ProcessFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::ProcessFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_pause_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    PauseCommand command(&mockPresenter);

    // The presenter should be notified with the PauseFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::PauseFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_expand_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ExpandCommand command(&mockPresenter);

    // The presenter should be notified with the ExpandSelectionFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::ExpandSelectionFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_expand_groups_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ExpandGroupsCommand command(&mockPresenter);

    // The presenter should be notified with the ExpandAllGroupsFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::ExpandAllGroupsFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_collapse_groups_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    CollapseGroupsCommand command(&mockPresenter);

    // The presenter should be notified with the CollapseAllGroupsFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::CollapseAllGroupsFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_plot_row_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    PlotRowCommand command(&mockPresenter);

    // The presenter should be notified with the PlotRowFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::PlotRowFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_plot_group_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    PlotGroupCommand command(&mockPresenter);

    // The presenter should be notified with the PlotGroupFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::PlotGroupFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_append_row_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    AppendRowCommand command(&mockPresenter);

    // The presenter should be notified with the AppendRowFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::AppendRowFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_append_group_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    AppendGroupCommand command(&mockPresenter);

    // The presenter should be notified with the AppendRowFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::AppendGroupFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_group_rows_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    GroupRowsCommand command(&mockPresenter);

    // The presenter should be notified with the GroupRowsFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::GroupRowsFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_copy_selected_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    CopySelectedCommand command(&mockPresenter);

    // The presenter should be notified with the CopySelectedFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::CopySelectedFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_cut_selected_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    CutSelectedCommand command(&mockPresenter);

    // The presenter should be notified with the CutSelectedFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::CutSelectedFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_paste_selected_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    PasteSelectedCommand command(&mockPresenter);

    // The presenter should be notified with the PasteSelectedFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::PasteSelectedFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_clear_selected_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    ClearSelectedCommand command(&mockPresenter);

    // The presenter should be notified with the ClearSelectedFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::ClearSelectedFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_delete_row_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DeleteRowCommand command(&mockPresenter);

    // The presenter should be notified with the DeleteRowFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::DeleteRowFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_delete_group_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DeleteGroupCommand command(&mockPresenter);

    // The presenter should be notified with the DeleteRowFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::DeleteGroupFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_separator_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    SeparatorCommand command(&mockPresenter);

    // The presenter should not be notified with any of the flags
    EXPECT_CALL(mockPresenter, notify(_)).Times(Exactly(0));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_workspace_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    WorkspaceCommand command(&mockPresenter, "workspace");

    // The presenter should set the name of the ws
    EXPECT_CALL(mockPresenter, setModel(QString("workspace")))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H */
