#include "MantidQtWidgets/Common/DataProcessorUI/DataProcessorOneLevelTreeManager.h"
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
#include "MantidQtWidgets/Common/DataProcessorUI/QDataProcessorOneLevelTreeModel.h"
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
  while(!selectedRows.empty()) {
    // Remove a row
    auto row = *selectedRows.begin();
    m_model->removeRow(row);

    // Once one row has been deleted the selcted row
    // indices are not valid any longer. We need
    // to update them again.
    selectedRows = m_presenter->selectedParents();
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
QString DataProcessorOneLevelTreeManager::copySelected() {
  const auto selectedRows = m_presenter->selectedParents();

  QStringList lines;
  for (const auto &row : selectedRows) {
    QStringList line;
    for (int col = 0; col < m_model->columnCount(); col++) {
      line.append(m_model->data(m_model->index(row, col)).toString());
    }
    lines.append(line.join("\t"));
  }
  return lines.join("\n");
}

/** Paste the contents of the clipboard into the currently selected rows, or
* append new rows
* @param text :: Selected rows to paste as a string
*/
void DataProcessorOneLevelTreeManager::pasteSelected(const QString &text) {

  if (text.isEmpty())
    return;

  auto lines = text.split("\n");

  // If we have rows selected, we'll overwrite them.
  // If not, we'll append new rows to write to.
  std::set<int> rows = m_presenter->selectedParents();
  if (rows.empty()) {
    // Add as many new rows as required
    for (auto i = 0; i < lines.size(); ++i) {
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
    auto values = (*lineIt).split("\t");

    // Paste as many columns as we can from this line
    for (int col = 0;
         col < m_model->columnCount() && col < static_cast<int>(values.size());
         ++col)
      m_model->setData(m_model->index(*rowIt, col), values[col]);
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

    QStringList data;
    for (int i = 0; i < m_model->columnCount(); i++)
      data.append(m_model->data(m_model->index(row, i)).toString());
    selectedData[row][row] = data;
  }
  return selectedData;
}

/** Transfer data to the model
* @param runs :: [input] Data to transfer as a vector of maps
* @param whitelist :: [input] Whitelist containing number of columns
*/
void DataProcessorOneLevelTreeManager::transfer(
    const std::vector<std::map<QString, QString>> &runs,
    const DataProcessorWhiteList &whitelist) {

  ITableWorkspace_sptr ws = m_model->getTableWorkspace();

  if (ws->rowCount() == 1) {
    // If the table only has one row, check if it is empty and if so, remove it.
    // This is to make things nicer when transferring, as the default table has
    // one empty row
    auto cols = ws->columnCount();
    bool emptyTable = true;
    for (auto i = 0u; i < cols; i++) {
      if (!ws->String(0, i).empty())
        emptyTable = false;
    }
    if (emptyTable)
      ws->removeRow(0);
  }

  // Loop over the rows (vector elements)
  for (const auto &row : runs) {

    TableRow newRow = ws->appendRow();

    for (auto i = 0; i < static_cast<int>(whitelist.size()); i++) {
      const QString columnName = whitelist.colNameFromColIndex(i);
      if (row.count(columnName)) {
        newRow << (row.at(columnName)).toStdString();
      } else {
        newRow << std::string();
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
void DataProcessorOneLevelTreeManager::update(int parent, int child,
                                              const QStringList &data) {

  UNUSED_ARG(child);

  if (static_cast<int>(data.size()) != m_model->columnCount())
    throw std::invalid_argument("Can't update tree with given data");

  for (int col = 0; col < m_model->columnCount(); col++)
    m_model->setData(m_model->index(parent, col), data[col]);
}

/** Gets the number of rows in the table
* @return : Number of rows
*/
int DataProcessorOneLevelTreeManager::rowCount() const {
  return m_model->rowCount();
}

/** Gets the number of rows in the table
* @param parent : The parent of the row
* @return : Number of rows
*/
int DataProcessorOneLevelTreeManager::rowCount(int parent) const {
  UNUSED_ARG(parent);
  return m_model->rowCount();
}

/** Gets the 'process' status of a row
* @param position : The row index
* @return : 'process' status
*/
bool DataProcessorOneLevelTreeManager::isProcessed(int position) const {
  return m_model->isProcessed(position);
}

/** Gets the 'process' status of a row
* @param position : The row index
* @param parent : The parent of the row
* @return : 'process' status
*/
bool DataProcessorOneLevelTreeManager::isProcessed(int position,
                                                   int parent) const {
  UNUSED_ARG(parent);
  return m_model->isProcessed(position);
}

/** Sets the 'process' status of a row
* @param processed : True to set row as processed, false to set unprocessed
* @param position : The index of the row to be set
*/
void DataProcessorOneLevelTreeManager::setProcessed(bool processed,
                                                    int position) {
  m_model->setProcessed(processed, position);
}

/** Sets the 'process' status of a row
* @param processed : True to set row as processed, false to set unprocessed
* @param position : The index of the row to be set
* @param parent : The parent of the row
*/
void DataProcessorOneLevelTreeManager::setProcessed(bool processed,
                                                    int position, int parent) {
  UNUSED_ARG(parent);
  m_model->setProcessed(processed, position);
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
    auto column =
        ws->addColumn("str", whitelist.colNameFromColIndex(col).toStdString());
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
    auto ncols = ws->columnCount();
    for (auto i = 0u; i < ncols; i++)
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

/** Sets a value in a cell
 *
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent item (unused)
 * @param parentColumn : the column index of the parent item (unused)
 * @param value : the new value to populate the cell with
*/
void DataProcessorOneLevelTreeManager::setCell(int row, int column,
                                               int parentRow, int parentColumn,
                                               const std::string &value) {

  UNUSED_ARG(parentRow);
  UNUSED_ARG(parentColumn);

  m_model->setData(m_model->index(row, column),
                   QVariant(QString::fromStdString(value)));
}

/** Returns the value in a cell as a string
 *
 * @param row : the row index
 * @param column : the column index
 * @param parentRow : the row index of the parent item (unused)
 * @param parentColumn : the column index of the parent item (unused)
 * @return : the value in the cell as a string
*/
std::string DataProcessorOneLevelTreeManager::getCell(int row, int column,
                                                      int parentRow,
                                                      int parentColumn) {
  UNUSED_ARG(parentRow);
  UNUSED_ARG(parentColumn);

  return m_model->data(m_model->index(row, column)).toString().toStdString();
}

/**
 * Gets the number of rows.
 * @return : the number of rows.
 */
int DataProcessorOneLevelTreeManager::getNumberOfRows() {
  return m_model->rowCount();
}

}
}
