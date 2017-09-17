#include "MantidQtWidgets/Common/DataProcessorUI/GenericOneLevelCommandProvider.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExpandCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorImportTableCommand.h"
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

namespace MantidQt {
namespace MantidWidgets {
GenericOneLevelCommandProvider::GenericOneLevelCommandProvider(
    GenericDataProcessorPresenter &presenter)
    : CommandProviderBase(presenter) {
  addEditCommands();
  addTableCommands();
}

void GenericOneLevelCommandProvider::addTableCommands() {
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

void GenericOneLevelCommandProvider::addEditCommands() {
  addEditCommand<DataProcessorProcessCommand>();
  addEditCommand<DataProcessorPauseCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorPlotRowCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorAppendRowCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorCopySelectedCommand>();
  addEditCommand<DataProcessorCutSelectedCommand>();
  addEditCommand<DataProcessorPasteSelectedCommand>();
  addEditCommand<DataProcessorClearSelectedCommand>();
  addEditCommand<DataProcessorSeparatorCommand>();
  addEditCommand<DataProcessorDeleteRowCommand>();
}

typename GenericOneLevelCommandProvider::CommandIndex
GenericOneLevelCommandProvider::indexOfCommand(TableAction action) const {
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
    throw std::logic_error("Unknown edit action.");
  }
}

typename GenericOneLevelCommandProvider::CommandIndices
GenericOneLevelCommandProvider::getModifyingTableCommands() const {
  return getModifyingCommands(getTableCommands());
}

typename GenericOneLevelCommandProvider::CommandIndex
GenericOneLevelCommandProvider::indexOfCommand(EditAction action) const {
  switch (action) {
  case EditAction::PROCESS:
    return 0;
  case EditAction::PAUSE:
    return 1;
  case EditAction::PLOT_RUNS:
    return 3;
  case EditAction::INSERT_ROW_AFTER:
    return 5;
  case EditAction::COPY_SELECTED:
    return 7;
  case EditAction::CUT_SELECTED:
    return 8;
  case EditAction::PASTE_SELECTED:
    return 9;
  case EditAction::CLEAR_SELECTED:
    return 10;
  case EditAction::DELETE_ROW:
    return 12;
  default:
    throw std::logic_error("Unknown edit action.");
  }
}

typename GenericOneLevelCommandProvider::CommandIndices GenericOneLevelCommandProvider::getPausingEditCommands() const {
  return CommandIndices({indexOfCommand(EditAction::PAUSE)});
}

typename GenericOneLevelCommandProvider::CommandIndices
GenericOneLevelCommandProvider::getProcessingEditCommands() const {
  return CommandIndices({indexOfCommand(EditAction::PROCESS)});
}

typename GenericOneLevelCommandProvider::CommandIndices
GenericOneLevelCommandProvider::getModifyingEditCommands() const {
  return getModifyingCommands(getEditCommands());
}
}
}
