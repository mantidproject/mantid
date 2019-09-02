// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/OneLevelTreeManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"

#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PauseCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QOneLevelTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SeparatorCommand.h"
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::MantidWidgets;

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

/**
 * Constructor
 * @param presenter :: a pointer to the presenter
 * @param table :: a table workspace
 * @param whitelist :: a whitelist
 */
OneLevelTreeManager::OneLevelTreeManager(
    DataProcessorPresenter *presenter, Mantid::API::ITableWorkspace_sptr table,
    const WhiteList &whitelist)
    : m_presenter(presenter),
      m_model(new QOneLevelTreeModel(table, whitelist)) {}

/**
 * Constructor (no table workspace given)
 * @param presenter :: [input] The DataProcessor presenter
 * @param whitelist :: [input] A whitelist containing the number of columns
 */
OneLevelTreeManager::OneLevelTreeManager(DataProcessorPresenter *presenter,
                                         const WhiteList &whitelist)
    : OneLevelTreeManager(presenter, createDefaultWorkspace(whitelist),
                          whitelist) {}

/**
 * Destructor
 */
OneLevelTreeManager::~OneLevelTreeManager() {}

bool OneLevelTreeManager::isMultiLevel() const { return false; }

/**
 * Publishes a list of available commands
 * @return : The list of available commands
 */
std::vector<Command_uptr> OneLevelTreeManager::publishCommands() {

  std::vector<Command_uptr> commands;

  addCommand(commands, std::make_unique<OpenTableCommand>(m_presenter));
  addCommand(commands, std::make_unique<NewTableCommand>(m_presenter));
  addCommand(commands, std::make_unique<SaveTableCommand>(m_presenter));
  addCommand(commands, std::make_unique<SaveTableAsCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<ImportTableCommand>(m_presenter));
  addCommand(commands, std::make_unique<ExportTableCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<OptionsCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<ProcessCommand>(m_presenter));
  addCommand(commands, std::make_unique<PauseCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<PlotRowCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<AppendRowCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<CopySelectedCommand>(m_presenter));
  addCommand(commands, std::make_unique<CutSelectedCommand>(m_presenter));
  addCommand(commands, std::make_unique<PasteSelectedCommand>(m_presenter));
  addCommand(commands, std::make_unique<ClearSelectedCommand>(m_presenter));
  addCommand(commands, std::make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, std::make_unique<DeleteRowCommand>(m_presenter));
  return commands;
}

/**
Insert a row after the last selected row. If nothing was selected, the new row
is appended to
to the last row the table.
*/
void OneLevelTreeManager::appendRow() {

  auto selectedRows = m_presenter->selectedParents();

  if (selectedRows.empty()) {
    m_model->insertRow(m_model->rowCount());
  } else {
    m_model->insertRow(*(selectedRows.rbegin()));
  }
}

/** Appends a group.
 */
void OneLevelTreeManager::appendGroup() {

  // This method should never be called
  throw std::runtime_error("Can't append group to table");
}

/**
Delete row(s) from the model
*/
void OneLevelTreeManager::deleteRow() {
  auto selectedRows = m_presenter->selectedParents();
  while (!selectedRows.empty()) {
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
void OneLevelTreeManager::deleteGroup() {

  // This method should never be called
  throw std::runtime_error("Can't delete group");
}

/**
Delete all rows from the model
*/
void OneLevelTreeManager::deleteAll() { m_model->removeAll(); }

/**
Group rows together
*/
void OneLevelTreeManager::groupRows() {

  // This method should never be called
  throw std::runtime_error("Can't group rows");
}

/** Expands the current selection to all the rows in the selected groups
 * @return :: Groups containing selected rows
 */
std::set<int> OneLevelTreeManager::expandSelection() {

  // This method should never be called
  throw std::runtime_error("Can't expand selection");
}

/** Clear the currently selected rows */
void OneLevelTreeManager::clearSelected() {

  const auto selectedRows = m_presenter->selectedParents();
  for (const auto &row : selectedRows) {
    for (int column = 0; column < m_model->columnCount(); column++)
      m_model->setData(m_model->index(row, column), QString());
  }
}

/** Return the currently selected rows as a string */
QString OneLevelTreeManager::copySelected() {
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
void OneLevelTreeManager::pasteSelected(const QString &text) {

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
void OneLevelTreeManager::newTable(const WhiteList &whitelist) {

  m_model.reset(
      new QOneLevelTreeModel(createDefaultWorkspace(whitelist), whitelist));
}

/** Opens a given table
 * @param table :: A table to open
 * @param whitelist :: A whitelist with the columns for the new table
 */
void OneLevelTreeManager::newTable(ITableWorkspace_sptr table,
                                   const WhiteList &whitelist) {

  if (isValidModel(table, whitelist.size())) {
    m_model.reset(new QOneLevelTreeModel(table, whitelist));
  } else
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");
}

/**
Inserts a new row to the specified group in the specified location
@param rowIndex :: The index to insert the new row after
*/
void OneLevelTreeManager::insertRow(int rowIndex) {

  m_model->insertRow(rowIndex);
}

TreeData OneLevelTreeManager::handleEmptyTable(bool prompt) {
  if (prompt)
    m_presenter->giveUserWarning("Cannot process an empty Table", "Warning");
  return TreeData();
}

bool OneLevelTreeManager::isEmptyTable() const {
  return m_model->rowCount() == 0;
}

bool OneLevelTreeManager::askUserIfShouldProcessAll() const {
  return m_presenter->askUserYesNo(
      "This will process all rows in the table. Continue?",
      "Process all rows?");
}

bool OneLevelTreeManager::shouldProcessAll() const {
  auto askBeforeProcessingAll =
      m_presenter->options().find("WarnProcessAll")->second;
  if (askBeforeProcessingAll.toBool()) {
    return askUserIfShouldProcessAll();
  } else {
    return true;
  }
}

std::set<int> OneLevelTreeManager::allRows() const {
  std::set<int> allRows;
  for (int row = 0; row < m_model->rowCount(); row++) {
    allRows.insert(row);
  }
  return allRows;
}

std::set<int> OneLevelTreeManager::noRows() const { return std::set<int>(); }

std::set<int> OneLevelTreeManager::getRowsToProcess(bool shouldPrompt) const {
  auto rows = m_presenter->selectedParents();
  if (rows.empty()) {
    if (shouldPrompt && !shouldProcessAll())
      return noRows();
    else
      return allRows();
  } else {
    return rows;
  }
}

/**
 * Returns given row data in a format that the presenter can understand and use
 * @return :: All data as a map where keys are units of post-processing (i.e.
 * group indices) and values are a map of row index in the group to row data
 */
TreeData OneLevelTreeManager::constructTreeData(std::set<int> rows) {

  // Return data in the format: map<int, set<RowData_sptr>>, where:
  // int -> row index
  // set<RowData_sptr> -> set of vectors storing the data. Each set is a row
  // and each element is the row data containing column values and metadata
  TreeData tree;
  const int columnNotUsed = 0; // dummy value required to create index
  for (const auto &row : rows) {
    tree[row][row] = m_model->rowData(m_model->index(row, columnNotUsed));
  }
  return tree;
}

/**
 * Returns selected data in a format that the presenter can understand and use
 * @param prompt :: True if warning messages should be displayed. False othewise
 * @return :: All data as a map where keys are units of post-processing (i.e.
 * group indices) and values are a map of row index in the group to row data
 */
TreeData OneLevelTreeManager::selectedData(bool prompt) {
  if (isEmptyTable()) {
    return handleEmptyTable(prompt);
  } else {
    return constructTreeData(getRowsToProcess(prompt));
  }
}

/**
 * Returns all data in a format that the presenter can understand and use
 * @param prompt :: True if warning messages should be displayed. False othewise
 * @return :: All data as a map where keys are units of post-processing (i.e.
 * group indices) and values are a map of row index in the group to row data
 */
TreeData OneLevelTreeManager::allData(bool prompt) {
  if (isEmptyTable()) {
    return handleEmptyTable(prompt);
  } else {
    return constructTreeData(allRows());
  }
}

/** Transfer data to the model
 * @param runs :: [input] Data to transfer as a vector of maps
 */
void OneLevelTreeManager::transfer(
    const std::vector<std::map<QString, QString>> &runs) {
  m_model->transfer(runs);
}

/** Updates a row with new data
 * @param parent :: the parent item of the row
 * @param child :: the row
 * @param data :: the data
 */
void OneLevelTreeManager::update(int parent, int child,
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
int OneLevelTreeManager::rowCount() const { return m_model->rowCount(); }

/** Gets the number of rows in the table
 * @param parent : The parent of the row
 * @return : Number of rows
 */
int OneLevelTreeManager::rowCount(int parent) const {
  UNUSED_ARG(parent);
  return m_model->rowCount();
}

/** Gets the 'process' status of a row
 * @param position : The row index
 * @return : 'process' status
 */
bool OneLevelTreeManager::isProcessed(int position) const {
  return m_model->isProcessed(position);
}

/** Gets the 'process' status of a row
 * @param position : The row index
 * @param parent : The parent of the row
 * @return : 'process' status
 */
bool OneLevelTreeManager::isProcessed(int position, int parent) const {
  UNUSED_ARG(parent);
  return m_model->isProcessed(position);
}

/** Sets the 'process' status of a row
 * @param processed : True to set row as processed, false to set unprocessed
 * @param position : The index of the row to be set
 */
void OneLevelTreeManager::setProcessed(bool processed, int position) {
  m_model->setProcessed(processed, position);
}

/** Sets the 'process' status of a row
 * @param processed : True to set row as processed, false to set unprocessed
 * @param position : The index of the row to be set
 * @param parent : The parent of the row
 */
void OneLevelTreeManager::setProcessed(bool processed, int position,
                                       int parent) {
  UNUSED_ARG(parent);
  m_model->setProcessed(processed, position);
}

/** Check whether reduction failed for a group
 * @param position : The row index
 * @return : true if there was an error
 */
bool OneLevelTreeManager::reductionFailed(int position) const {
  return m_model->reductionFailed(position);
}

/** Check whether reduction failed for a group
 * @param position : The row index
 * @param parent : The parent of the row
 * @return : true if there was an error
 */
bool OneLevelTreeManager::reductionFailed(int position, int parent) const {
  UNUSED_ARG(parent);
  return m_model->reductionFailed(position);
}

/** Sets the error for a group
 * @param error : the error message
 * @param position : The index of the row to be set
 */
void OneLevelTreeManager::setError(const std::string &error, int position) {
  m_model->setError(error, position);
}

/** Sets the error message for a row
 * @param error : the error message
 * @param position : The index of the row to be set
 * @param parent : The parent of the row
 */
void OneLevelTreeManager::setError(const std::string &error, int position,
                                   int parent) {
  UNUSED_ARG(parent);
  m_model->setError(error, position);
}

/** Clear the processed/error state for all rows
 */
void OneLevelTreeManager::invalidateAllProcessed() {
  for (auto i = 0; i < m_model->rowCount(); i++) {
    setProcessed(false, i);
    setError("", i);
  }
}

/** Return a shared ptr to the model
 * @return :: A shared ptr to the model
 */
boost::shared_ptr<AbstractTreeModel> OneLevelTreeManager::getModel() {
  return m_model;
}

/** Returns the table workspace containing the data
 * @return :: The table workspace
 */
ITableWorkspace_sptr OneLevelTreeManager::getTableWorkspace() {

  return m_model->getTableWorkspace();
}

/**
 * Creates a default table using the whitelist supplied to this presenter
 * @param whitelist :: The whitelist that will be used to create a new table
 * @return : A default table
 */
ITableWorkspace_sptr
OneLevelTreeManager::createDefaultWorkspace(const WhiteList &whitelist) {
  ITableWorkspace_sptr ws =
      Mantid::API::WorkspaceFactory::Instance().createTable();

  for (auto const &columnName : whitelist.names()) {
    // The columns provided to this presenter
    auto column = ws->addColumn("str", columnName.toStdString());
    column->setPlotType(0);
  }
  ws->appendRow();
  return ws;
}

/** Validate a table workspace
 * @param ws :: the table workspace
 * @param whitelistColumns :: the number of columns as specified in a whitelist
 */
void OneLevelTreeManager::validateModel(ITableWorkspace_sptr ws,
                                        size_t whitelistColumns) const {

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
bool OneLevelTreeManager::isValidModel(Workspace_sptr ws,
                                       size_t whitelistColumns) const {

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
void OneLevelTreeManager::setCell(int row, int column, int parentRow,
                                  int parentColumn, const std::string &value) {

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
std::string OneLevelTreeManager::getCell(int row, int column, int parentRow,
                                         int parentColumn) const {
  UNUSED_ARG(parentRow);
  UNUSED_ARG(parentColumn);

  return m_model->data(m_model->index(row, column)).toString().toStdString();
}

/**
 * Gets the number of rows.
 * @return : the number of rows.
 */
int OneLevelTreeManager::getNumberOfRows() { return m_model->rowCount(); }
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
