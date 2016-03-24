#ifndef MANTID_CUSTOMINTERFACES_REFLCOMMANDSTEST_H
#define MANTID_CUSTOMINTERFACES_REFLCOMMANDSTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidAPI/FrameworkManager.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflAppendRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflClearSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCopySelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflCutSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflDeleteRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflExpandCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflExportTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflGroupRowsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflImportTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflNewTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflOpenTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflOptionsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflPasteSelectedCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflPlotGroupCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflPlotRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflPrependRowCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflProcessCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTableAsCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflSaveTableCommand.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflTableViewPresenter.h"


using namespace MantidQt::CustomInterfaces;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

class MockIReflPresenter : IReflTablePresenter {};

//=====================================================================================
// Functional tests
//=====================================================================================
class ReflCommandsTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ReflCommandsTest *createSuite() {
    return new ReflCommandsTest();
  }
  static void destroySuite(ReflCommandsTest *suite) { delete suite; }

	ReflCommandsTest() { FrameworkManager::Instance(); }

  void test_open_table_command() {
		ReflOpenTableCommand command;
  }

	void test_new_table_command() {

	}

	void test_save_table_command() {

	}

	void test_save_table_as_command() {

	}

	void test_import_table_command() {

	}

	void test_export_table_command() {

	}

	void test_options_command() {

	}

	void test_process_command() {

	}

	void test_expand_command() {

	}

	void test_plot_row_command() {

	}

	void test_plot_group_command() {

	}

	void test_append_row_command() {

	}

	void test_prepend_row_command() {

	}

	void test_group_rows_command() {

	}

	void test_copy_selected_command() {

	}

	void test_cut_selected_command() {

	}

	void test_paste_selected_command() {

	}

	void test_clear_selected_command() {

	}

	void test_delete_row_command() {

	}

};
#endif /* MANTID_CUSTOMINTERFACES_REFLCOMMANDSTEST_H */
