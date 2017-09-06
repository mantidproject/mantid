#include "MantidQtWidgets/Common/DataProcessorUI/GenericTwoLevelCommandProvider.h"
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
#include <stdexcept>

namespace MantidQt {
namespace MantidWidgets {
GenericTwoLevelCommandProvider::GenericTwoLevelCommandProvider(
    GenericDataProcessorPresenter &presenter)
    : CommandProviderBase(presenter) {
  addTableCommands();
  addEditCommands();
}

void GenericTwoLevelCommandProvider::addTableCommands() {
  addTableCommand<DataProcessorOpenTableCommand>();
  addTableCommand<DataProcessorNewTableCommand>();
  addTableCommand<DataProcessorSaveTableCommand>();
  addTableCommand<DataProcessorSaveTableAsCommand>();
  addTableCommand<DataProcessorSeparatorCommand>();
  addTableCommand<DataProcessorImportTableCommand>();
  addTableCommand<DataProcessorExportTableCommand>();
  addTableCommand<DataProcessorSeparatorCommand>();
  addTableCommand<DataProcessorOptionsCommand>();
}

void GenericTwoLevelCommandProvider::addEditCommands() {
  addEditCommand<DataProcessorProcessCommand>();
  addEditCommand<DataProcessorPauseCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorExpandCommand>();
  addEditCommand<DataProcessorExpandGroupsCommand>();
  addEditCommand<DataProcessorCollapseGroupsCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorPlotRowCommand>();
  addEditCommand<DataProcessorPlotGroupCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorAppendRowCommand>();
  addEditCommand<DataProcessorAppendGroupCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorGroupRowsCommand>();
  addEditCommand<DataProcessorCopySelectedCommand>();
  addEditCommand<DataProcessorCutSelectedCommand>();
  addEditCommand<DataProcessorPasteSelectedCommand>();
  addEditCommand<DataProcessorClearSelectedCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorDeleteRowCommand>();
  addEditCommand<DataProcessorDeleteGroupCommand>();
}

typename GenericTwoLevelCommandProvider::CommandIndex
GenericTwoLevelCommandProvider::indexOfCommand(TableAction action) const {
  switch (action) {
  case TableAction::OPEN_TABLE:
    return 0;
  case TableAction::NEW_TABLE:
    return 1;
  case TableAction::SAVE_TABLE:
    return 2;
  case TableAction::SAVE_TABLE_AS:
    return 3;
  case TableAction::IMPORT_TBL_FILE:
    return 5;
  case TableAction::EXPORT_TBL_FILE:
    return 6;
  case TableAction::OPTIONS:
    return 8;
  default:
    throw std::logic_error("Unknown table action for two level provider specified.");
  }
}

typename GenericTwoLevelCommandProvider::CommandIndices
GenericTwoLevelCommandProvider::getModifyingTableCommands() const {
  return CommandIndices{};
}

typename GenericTwoLevelCommandProvider::CommandIndex
GenericTwoLevelCommandProvider::indexOfCommand(EditAction action) const {
  switch (action) {
  case EditAction::PROCESS:
    return 0;
  case EditAction::PAUSE:
    return 1;
  case EditAction::SELECT_GROUP:
    return 3;
  case EditAction::EXPAND_GROUP:
    return 4;
  case EditAction::COLAPSE_GROUP:
    return 5;
  case EditAction::PLOT_RUNS:
    return 7;
  case EditAction::PLOT_GROUP:
    return 8;
  case EditAction::INSERT_ROW_AFTER:
    return 10;
  case EditAction::INSERT_GROUP_AFTER:
    return 11;
  case EditAction::GROUP_SELECTED:
    return 13;
  case EditAction::COPY_SELECTED:
    return 14;
  case EditAction::CUT_SELECTED:
    return 15;
  case EditAction::PASTE_SELECTED:
    return 16;
  case EditAction::CLEAR_SELECTED:
    return 17;
  case EditAction::DELETE_ROW:
    return 19;
  case EditAction::DELETE_GROUP:
    return 20;
  case EditAction::WHATS_THIS:
    return 21;
  default:
    throw std::logic_error("Unknown action for two level manager specified.");
  }
}

typename GenericTwoLevelCommandProvider::CommandIndices
GenericTwoLevelCommandProvider::getPausingEditCommands() const {
  return CommandIndices({indexOfCommand(EditAction::PAUSE)});
}

typename GenericTwoLevelCommandProvider::CommandIndices
GenericTwoLevelCommandProvider::getProcessingEditCommands() const {
  return CommandIndices({indexOfCommand(EditAction::PROCESS)});
}

typename GenericTwoLevelCommandProvider::CommandIndices
GenericTwoLevelCommandProvider::getModifyingEditCommands() const {
  return CommandIndices{};
}
}
}
