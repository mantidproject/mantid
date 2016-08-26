#ifndef MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H
#define MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
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
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"

using namespace MantidQt::MantidWidgets;
using namespace testing;

//=====================================================================================
// Functional tests
//=====================================================================================
class DataProcessorTwoLevelTreeManagerTest : public CxxTest::TestSuite {

private:
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DataProcessorTwoLevelTreeManagerTest *createSuite() {
    return new DataProcessorTwoLevelTreeManagerTest();
  }
  static void destroySuite(DataProcessorTwoLevelTreeManagerTest *suite) {
    delete suite;
  }

  void test_publish_commands() {
    NiceMock<MockDataProcessorPresenter> presenter;
    DataProcessorTwoLevelTreeManager manager(&presenter);

    auto comm = manager.publishCommands();

    TS_ASSERT_EQUALS(comm.size(), 27);
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
    TS_ASSERT(dynamic_cast<DataProcessorExpandCommand *>(comm[11].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[12].get()));
    TS_ASSERT(dynamic_cast<DataProcessorPlotRowCommand *>(comm[13].get()));
    TS_ASSERT(dynamic_cast<DataProcessorPlotGroupCommand *>(comm[14].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[15].get()));
    TS_ASSERT(dynamic_cast<DataProcessorAppendRowCommand *>(comm[16].get()));
    TS_ASSERT(dynamic_cast<DataProcessorAppendGroupCommand *>(comm[17].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[18].get()));
    TS_ASSERT(dynamic_cast<DataProcessorGroupRowsCommand *>(comm[19].get()));
    TS_ASSERT(dynamic_cast<DataProcessorCopySelectedCommand *>(comm[20].get()));
    TS_ASSERT(dynamic_cast<DataProcessorCutSelectedCommand *>(comm[21].get()));
    TS_ASSERT(
        dynamic_cast<DataProcessorPasteSelectedCommand *>(comm[22].get()));
    TS_ASSERT(
        dynamic_cast<DataProcessorClearSelectedCommand *>(comm[23].get()));
    TS_ASSERT(dynamic_cast<DataProcessorSeparatorCommand *>(comm[24].get()));
    TS_ASSERT(dynamic_cast<DataProcessorDeleteRowCommand *>(comm[25].get()));
    TS_ASSERT(dynamic_cast<DataProcessorDeleteGroupCommand *>(comm[26].get()));
  }
};
#endif /* MANTID_MANTIDWIDGETS_DATAPROCESSORTWOLEVELTREEMANAGERTEST_H */
