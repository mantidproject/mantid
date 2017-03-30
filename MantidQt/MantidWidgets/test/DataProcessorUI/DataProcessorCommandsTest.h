#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGroupRowsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorMockObjects.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPresenter.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorWorkspaceCommand.h"

using namespace MantidQt::MantidWidgets;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorCommandsTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorCommandsTest *createSuite() {
    return new DataProcessorCommandsTest();
  }
  static void destroySuite(DataProcessorCommandsTest *suite) { delete suite; }

  void test_open_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DataProcessorOpenTableCommand command(&mockPresenter);

    // The presenter should be notified with the OpenTableFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::OpenTableFlag))
        .Times(Exactly(0));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_new_table_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DataProcessorNewTableCommand command(&mockPresenter);

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
    DataProcessorSaveTableCommand command(&mockPresenter);

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
    DataProcessorSaveTableAsCommand command(&mockPresenter);

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
    DataProcessorImportTableCommand command(&mockPresenter);

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
    DataProcessorExportTableCommand command(&mockPresenter);

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
    DataProcessorOptionsCommand command(&mockPresenter);

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
    DataProcessorProcessCommand command(&mockPresenter);

    // The presenter should be notified with the ProcessFlag
    EXPECT_CALL(mockPresenter, notify(DataProcessorPresenter::ProcessFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_expand_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DataProcessorExpandCommand command(&mockPresenter);

    // The presenter should be notified with the ExpandSelectionFlag
    EXPECT_CALL(mockPresenter,
                notify(DataProcessorPresenter::ExpandSelectionFlag))
        .Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_plot_row_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DataProcessorPlotRowCommand command(&mockPresenter);

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
    DataProcessorPlotGroupCommand command(&mockPresenter);

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
    DataProcessorAppendRowCommand command(&mockPresenter);

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
    DataProcessorAppendGroupCommand command(&mockPresenter);

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
    DataProcessorGroupRowsCommand command(&mockPresenter);

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
    DataProcessorCopySelectedCommand command(&mockPresenter);

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
    DataProcessorCutSelectedCommand command(&mockPresenter);

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
    DataProcessorPasteSelectedCommand command(&mockPresenter);

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
    DataProcessorClearSelectedCommand command(&mockPresenter);

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
    DataProcessorDeleteRowCommand command(&mockPresenter);

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
    DataProcessorDeleteGroupCommand command(&mockPresenter);

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
    DataProcessorSeparatorCommand command(&mockPresenter);

    // The presenter should not be notified with any of the flags
    EXPECT_CALL(mockPresenter, notify(_)).Times(Exactly(0));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }

  void test_workspace_command() {
    NiceMock<MockDataProcessorPresenter> mockPresenter;
    DataProcessorWorkspaceCommand command(&mockPresenter, "workspace");

    // The presenter should set the name of the ws
    EXPECT_CALL(mockPresenter, setModel("workspace")).Times(Exactly(1));
    // Execute the command
    command.execute();
    // Verify expectations
    TS_ASSERT(Mock::VerifyAndClearExpectations(&mockPresenter));
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORCOMMANDSTEST_H */
