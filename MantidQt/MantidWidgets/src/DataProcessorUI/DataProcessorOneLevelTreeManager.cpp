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
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPauseCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorOneLevelTreeModel.h"
#include "MantidKernel/make_unique.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

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
      m_model(new QDataProcessorOneLevelTreeModel(table, whitelist)) {}

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
  addCommand(commands,
             make_unique<DataProcessorSaveTableAsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorImportTableCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorExportTableCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorOptionsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorProcessCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPauseCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorAppendRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorCopySelectedCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorCutSelectedCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorPasteSelectedCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorClearSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorDeleteRowCommand>(m_presenter));
  return commands;
}

/**
Insert a row after the last selected row. If nothing was selected, the new row
is appended to
to the last row the table.
*/
void DataProcessorOneLevelTreeManager::appendRow() {

  auto selectedRows = m_presenter->selectedParents();

  if (selectedRows.empty()) {
    m_model->insertRow(m_model->rowCount());
  } else {
    m_model->insertRow(*(selectedRows.rbegin()));
  }
}

/** Appends a group.
*/
void DataProcessorOneLevelTreeManager::appendGroup() {

  // This method should never be called
  throw std::runtime_error("Can't append group to table");
}

/**
Delete row(s) from the model
*/
void DataProcessorOneLevelTreeManager::deleteRow() {

  auto selectedRows = m_presenter->selectedParents();

  if (selectedRows.empty())
    return;

  for (const auto &row : selectedRows) {
    m_model->removeRow(row);
  }
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

/** Expands the current selection to all the rows in the selected groups
* @return :: Groups containing selected rows
*/
std::set<int> DataProcessorOneLevelTreeManager::expandSelection() {

  // This method should never be called
  throw std::runtime_error("Can't expand selection");
}

/** Clear the currently selected rows */
void DataProcessorOneLevelTreeManager::clearSelected() {

  const auto selectedRows = m_presenter->selectedParents();

  if (selectedRows.empty())
    return;

  for (const auto &row : selectedRows) {
    for (int column = 0; column < m_model->columnCount(); column++)
      m_model->setData(m_model->index(row, column), QString());
  }
}

/** Return the currently selected rows as a string */
std::string DataProcessorOneLevelTreeManager::copySelected() {

  const auto selectedRows = m_presenter->selectedParents();

  if (selectedRows.empty())
    return std::string();

  std::vector<std::string> lines;

  for (const auto &row : selectedRows) {
    std::vector<std::string> line;
    for (int col = 0; col < m_model->columnCount(); col++) {
      line.push_back(
          m_model->data(m_model->index(row, col)).toString().toStdString());
    }
    lines.push_back(boost::algorithm::join(line, "\t"));
  }
  return boost::algorithm::join(lines, "\n");
}

/** Paste the contents of the clipboard into the currently selected rows, or
* append new rows
* @param text :: Selected rows to paste as a string
*/
void DataProcessorOneLevelTreeManager::pasteSelected(const std::string &text) {

  if (text.empty())
    return;

  std::vector<std::string> lines;
  boost::split(lines, text, boost::is_any_of("\n"));

  // If we have rows selected, we'll overwrite them.
  // If not, we'll append new rows to write to.
  std::set<int> rows = m_presenter->selectedParents();
  if (rows.empty()) {
    // Add as many new rows as required
    for (size_t i = 0; i < lines.size(); ++i) {
      int index = m_model->rowCount();
      insertRow(index);
      rows.insert(index);
    }
  }

  // Iterate over rows and lines simultaneously, stopping when we reach the end
  // of either
  auto rowIt = rows.begin();
  auto lineIt = lines.begin();
  for (; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++) {
    std::vector<std::string> values;
    boost::split(values, *lineIt, boost::is_any_of("\t"));

    // Paste as many columns as we can from this line
    for (int col = 0;
         col < m_model->columnCount() && col < static_cast<int>(values.size());
         ++col)
      m_model->setData(m_model->index(*rowIt, col),
                       QString::fromStdString(values[col]));
  }
}

/** Opens a blank table
* @param whitelist :: A whitelist with the columns for the new table
*/
void DataProcessorOneLevelTreeManager::newTable(
    const DataProcessorWhiteList &whitelist) {

  m_model.reset(new QDataProcessorOneLevelTreeModel(
      createDefaultWorkspace(whitelist), whitelist));
}

/** Opens a given table
* @param table :: A table to open
* @param whitelist :: A whitelist with the columns for the new table
*/
void DataProcessorOneLevelTreeManager::newTable(
    ITableWorkspace_sptr table, const DataProcessorWhiteList &whitelist) {

  if (isValidModel(table, whitelist.size())) {
    m_model.reset(new QDataProcessorOneLevelTreeModel(table, whitelist));
  } else
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");
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
TreeData DataProcessorOneLevelTreeManager::selectedData(bool prompt) {

  TreeData selectedData;

  auto options = m_presenter->options();

  if (m_model->rowCount() == 0 && prompt) {
    m_presenter->giveUserWarning("Cannot process an empty Table", "Warning");
    return selectedData;
  }

  // Selected rows
  auto rows = m_presenter->selectedParents();

  if (rows.empty()) {

    if (options["WarnProcessAll"].toBool() && prompt) {
      if (!m_presenter->askUserYesNo(
              "This will process all rows in the table. Continue?",
              "Process all rows?"))
        return selectedData;
    }

    // They want to process everything
    // Populate all groups with all rows

    for (int row = 0; row < m_model->rowCount(); row++) {
      rows.insert(row);
    }
  }

  // Return selected data in the format: map<int, set<vector<string>>>, where:
  // int -> row index
  // set<vector<string>> -> set of vectors storing the data. Each set is a row
  // and each element in the vector is a column
  for (const auto &row : rows) {

    std::vector<std::string> data;
    for (int i = 0; i < m_model->columnCount(); i++)
      data.push_back(
          m_model->data(m_model->index(row, i)).toString().toStdString());
    selectedData[row][row] = data;
  }
  return selectedData;
}

/** Transfer data to the model
* @param runs :: [input] Data to transfer as a vector of maps
* @param whitelist :: [input] Whitelist containing number of columns
*/
void DataProcessorOneLevelTreeManager::transfer(
    const std::vector<std::map<std::string, std::string>> &runs,
    const DataProcessorWhiteList &whitelist) {

  ITableWorkspace_sptr ws = m_model->getTableWorkspace();

  if (ws->rowCount() == 1) {
    // If the table only has one row, check if it is empty and if so, remove it.
    // This is to make things nicer when transferring, as the default table has
    // one empty row
    size_t cols = ws->columnCount();
    bool emptyTable = true;
    for (size_t i = 0; i < cols; i++) {
      if (!ws->String(0, i).empty())
        emptyTable = false;
    }
    if (emptyTable)
      ws->removeRow(0);
  }

  // Loop over the rows (vector elements)
  for (const auto &row : runs) {

    TableRow newRow = ws->appendRow();

    for (int i = 0; i < static_cast<int>(whitelist.size()); i++) {
      const std::string columnName = whitelist.colNameFromColIndex(i);
      if (row.count(columnName)) {
        newRow << row.at(columnName);
      } else {
        newRow << "";
      }
    }
  }

  m_model.reset(new QDataProcessorOneLevelTreeModel(ws, whitelist));
}

/** Updates a row with new data
* @param parent :: the parent item of the row
* @param child :: the row
* @param data :: the data
*/
void DataProcessorOneLevelTreeManager::update(
    int parent, int child, const std::vector<std::string> &data) {

  UNUSED_ARG(child);

  if (static_cast<int>(data.size()) != m_model->columnCount())
    throw std::invalid_argument("Can't update tree with given data");

  for (int col = 0; col < m_model->columnCount(); col++)
    m_model->setData(m_model->index(parent, col),
                     QString::fromStdString(data[col]));
}

/** Sets a new row to be highlighted
* @param position : The index of the row to be highlighted
*/
void DataProcessorOneLevelTreeManager::addHighlighted(int position) {
  m_model->addHighlighted(position);
}

/** Sets a new row to be highlighted
* @param position : The index of the row to be highlighted
* @param parent : The parent of the row
*/
void DataProcessorOneLevelTreeManager::addHighlighted(int position,
                                                      int parent) {
  UNUSED_ARG(parent);
  addHighlighted(position);
}

/** Return a shared ptr to the model
* @return :: A shared ptr to the model
*/
boost::shared_ptr<AbstractDataProcessorTreeModel>
DataProcessorOneLevelTreeManager::getModel() {
  return m_model;
}

/** Returns the table workspace containing the data
 * @return :: The table workspace
 */
ITableWorkspace_sptr DataProcessorOneLevelTreeManager::getTableWorkspace() {

  return m_model->getTableWorkspace();
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

  for (int col = 0; col < static_cast<int>(whitelist.size()); col++) {
    // The columns provided to this presenter
    auto column = ws->addColumn("str", whitelist.colNameFromColIndex(col));
    column->setPlotType(0);
  }
  ws->appendRow();
  return ws;
}

/** Validate a table workspace
* @param ws :: the table workspace
* @param whitelistColumns :: the number of columns as specified in a whitelist
*/
void DataProcessorOneLevelTreeManager::validateModel(
    ITableWorkspace_sptr ws, size_t whitelistColumns) const {

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
