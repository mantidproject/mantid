#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorOneLevelTreeModel.h"
#include "MantidKernel/make_unique.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {

/**
* Constructor
* @param presenter :: a pointer to the presenter
* @param table :: a table workspace
* @param whitelist :: a whitelist
*/
DataProcessorOneLevelTreeManager::DataProcessorOneLevelTreeManager(
    DataProcessorPresenter *presenter, Mantid::API::ITableWorkspace_sptr table,
    const DataProcessorWhiteList &whitelist)
    : m_presenter(presenter),
      m_model(new QDataProcessorOneLevelTreeModel(table, whitelist)),
      m_ws(table) {}

/**
* Constructor (no table workspace given)
* @param presenter :: [input] The DataProcessor presenter
* @param whitelist :: [input] A whitelist containing the number of columns
*/
DataProcessorOneLevelTreeManager::DataProcessorOneLevelTreeManager(
    DataProcessorPresenter *presenter, const DataProcessorWhiteList &whitelist)
    : DataProcessorOneLevelTreeManager(
          presenter, createDefaultWorkspace(whitelist), whitelist) {}

/**
* Destructor
*/
DataProcessorOneLevelTreeManager::~DataProcessorOneLevelTreeManager() {}

/**
* Publishes a list of available commands
* @return : The list of available commands
*/
std::vector<DataProcessorCommand_uptr>
DataProcessorOneLevelTreeManager::publishCommands() {

  std::vector<DataProcessorCommand_uptr> commands;

  addCommand(commands, make_unique<DataProcessorOpenTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorNewTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSaveTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSaveTableAsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorImportTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorExportTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorOptionsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorProcessCommand>(m_presenter));
  // This makes no sense if there are no groups
  //addCommand(commands, make_unique<DataProcessorExpandCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotRowCommand>(m_presenter));
  // This makes no sense if there are no groups
  //addCommand(commands, make_unique<DataProcessorPlotGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorAppendRowCommand>(m_presenter));
  // This makes no sense if there are no groups
  //addCommand(commands, make_unique<DataProcessorAppendGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  // This makes no sense if there are no groups
  //addCommand(commands, make_unique<DataProcessorGroupRowsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorCopySelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorCutSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPasteSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorClearSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorDeleteRowCommand>(m_presenter));
  // This makes no sense if there are no groups
  //addCommand(commands, make_unique<DataProcessorDeleteGroupCommand>(m_presenter));
  return commands;
}

/**
Insert a row after the last selected row. If a group was selected, the new row
is appended to that group. If nothing was selected, the new row is appended to
the last group in the table.
*/
void DataProcessorOneLevelTreeManager::appendRow() {

  auto selectedRows = m_presenter->selectedParents();

  // TODO: Deal with selected rows
}

void DataProcessorOneLevelTreeManager::appendGroup() {

  // This method should never be called
  throw std::runtime_error("Can't append group to table");
}

/**
Delete row(s) from the model
*/
void DataProcessorOneLevelTreeManager::deleteRow() {

  auto selectedRows = m_presenter->selectedParents();

  // TODO: Deal with selected rows
}

/**
Delete group(s) from the model
*/
void DataProcessorOneLevelTreeManager::deleteGroup() {

  // This method should never be called
  throw std::runtime_error("Can't delete group");
}

/**
Group rows together
*/
void DataProcessorOneLevelTreeManager::groupRows() {

  // This method should never be called
  throw std::runtime_error("Can't group rows");
}

/** Expands the current selection to all the rows in the selected groups, this
* effectively means selecting the parent item (i.e. the group to which the
* selected rows belong)
* @return :: Groups containing selected rows
*/
std::set<int> DataProcessorOneLevelTreeManager::expandSelection() {

  // This method should never be called
  throw std::runtime_error("Can't expand selection");
}

/** Clear the currently selected rows */
void DataProcessorOneLevelTreeManager::clearSelected() {

  const auto selectedRows = m_presenter->selectedParents();

  // TODO: deal with selected rows
}

/** Return the currently selected rows as a string */
std::string DataProcessorOneLevelTreeManager::copySelected() {

  const auto selectedRows = m_presenter->selectedParents();

  // TODO: deal with selected rows
}

/** Paste the contents of the clipboard into the currently selected rows, or
* append new rows
* @param text :: Selected rows to paste as a string
*/
void DataProcessorOneLevelTreeManager::pasteSelected(const std::string &text) {

	const auto selectedRows = m_presenter->selectedParents();

	// TODO: deal with selected rows
}

/**
Inserts a new row to the specified group in the specified location
@param rowIndex :: The index to insert the new row after
*/
void DataProcessorOneLevelTreeManager::insertRow(int rowIndex) {

  m_model->insertRow(rowIndex);
}

/**
* Returns selected data in a format that the presenter can understand and use
* @param prompt :: True if warning messages should be displayed. False othewise
* @return :: Selected data as a map where keys are units of post-processing and
* values are
*/
std::map<int, std::set<std::vector<std::string>>>
DataProcessorOneLevelTreeManager::selectedData(bool prompt) {

  // Selected rows
  auto rows = m_presenter->selectedParents();

  // TODO: return selected data
}

/** Transfer data to the model
* @param runs :: [input] Data to transfer as a vector of maps
* @param whitelist :: [input] Whitelist containing number of columns
*/
void DataProcessorOneLevelTreeManager::transfer(
    const std::vector<std::map<std::string, std::string>> &runs,
    const DataProcessorWhiteList &whitelist) {

  // Loop over the rows (vector elements)
  for (const auto &row : runs) {

    if (row.size() != whitelist.size())
      throw std::invalid_argument(
          "Data cannot be transferred to the processing table.");

    TableRow newRow = m_ws->appendRow();

    for (int i = 0; i < static_cast<int>(whitelist.size()); i++)
      newRow << row.at(whitelist.colNameFromColIndex(i));
  }

  m_model.reset(new QDataProcessorOneLevelTreeModel(m_ws, whitelist));
}

/** Return a shared ptr to the model
* @return :: A shared ptr to the model
*/
boost::shared_ptr<QAbstractItemModel>
DataProcessorOneLevelTreeManager::getModel() {
  return m_model;
}

/** Returns the table workspace containing the data
* @return :: The table workspace
*/
ITableWorkspace_sptr DataProcessorOneLevelTreeManager::getTableWorkspace() {
  return m_ws;
}

/**
* Creates a default table using the whitelist supplied to this presenter
* @param whitelist :: The whitelist that will be used to create a new table
* @return : A default table
*/
ITableWorkspace_sptr DataProcessorOneLevelTreeManager::createDefaultWorkspace(
    const DataProcessorWhiteList &whitelist) {
  ITableWorkspace_sptr ws =
      Mantid::API::WorkspaceFactory::Instance().createTable();

  for (auto col = 0; col < whitelist.size(); col++) {
    // The columns provided to this presenter
    auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
    column->setPlotType(0);
  }
  ws->appendRow();
  return ws;
}

void validateModel(ITableWorkspace_sptr ws, size_t whitelistColumns) {

  if (!ws)
    throw std::runtime_error("Null pointer");

  // Table workspace must have one extra column, which corresponds to the
  // group
  if (ws->columnCount() != whitelistColumns)
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");

  try {
    size_t ncols = ws->columnCount();
    for (size_t i = 0; i < ncols; i++)
      ws->String(0, i);
  } catch (const std::runtime_error &) {
    throw std::runtime_error("Selected table does not meet the specifications "
                             "to become a model for this interface");
  }
}

/**
* Validates the number of columns in a model
* @param ws : [input] The workspace to validate
* @param whitelistColumns : [input] The number of columns in the whitelist
* @throws std::runtime_error if the number of columns in the table is incorrect
*/
bool DataProcessorOneLevelTreeManager::isValidModel(
    Workspace_sptr ws, size_t whitelistColumns) const {

  try {
    validateModel(boost::dynamic_pointer_cast<ITableWorkspace>(ws),
                  whitelistColumns);
  } catch (...) {
    return false;
  }
  return true;
}

}
}
