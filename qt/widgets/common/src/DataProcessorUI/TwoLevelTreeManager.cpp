// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/TwoLevelTreeManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/AppendRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ClearSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CollapseGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CopySelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/CutSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DeleteGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/DeleteRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExpandCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExpandGroupsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ExportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/GroupRowsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ImportTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/NewTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OpenTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/OptionsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PasteSelectedCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PauseCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PlotGroupCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/PlotRowCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ProcessCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableAsCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SaveTableCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/SeparatorCommand.h"
#include "MantidQtWidgets/Common/DataProcessorUI/ToStdStringMap.h"
#include "MantidQtWidgets/Common/ParseNumerics.h"
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
TwoLevelTreeManager::TwoLevelTreeManager(
    DataProcessorPresenter *presenter, Mantid::API::ITableWorkspace_sptr table,
    const WhiteList &whitelist)
    : m_presenter(presenter),
      m_model(new QTwoLevelTreeModel(table, whitelist)) {}

/**
 * Constructor (no table workspace given)
 * @param presenter :: [input] The DataProcessor presenter
 * @param whitelist :: [input] A whitelist containing the number of columns
 */
TwoLevelTreeManager::TwoLevelTreeManager(DataProcessorPresenter *presenter,
                                         const WhiteList &whitelist)
    : TwoLevelTreeManager(presenter, createDefaultWorkspace(whitelist),
                          whitelist) {}

/**
 * Destructor
 */
TwoLevelTreeManager::~TwoLevelTreeManager() {}

bool TwoLevelTreeManager::isMultiLevel() const { return true; }
/**
 * Publishes a list of available commands
 * @return : The list of available commands
 */
std::vector<Command_uptr> TwoLevelTreeManager::publishCommands() {

  std::vector<Command_uptr> commands;

  addCommand(commands, make_unique<OpenTableCommand>(m_presenter));
  addCommand(commands, make_unique<NewTableCommand>(m_presenter));
  addCommand(commands, make_unique<SaveTableCommand>(m_presenter));
  addCommand(commands, make_unique<SaveTableAsCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<ImportTableCommand>(m_presenter));
  addCommand(commands, make_unique<ExportTableCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<OptionsCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<ProcessCommand>(m_presenter));
  addCommand(commands, make_unique<PauseCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<ExpandCommand>(m_presenter));
  addCommand(commands, make_unique<ExpandGroupsCommand>(m_presenter));
  addCommand(commands, make_unique<CollapseGroupsCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<PlotRowCommand>(m_presenter));
  addCommand(commands, make_unique<PlotGroupCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<AppendRowCommand>(m_presenter));
  addCommand(commands, make_unique<AppendGroupCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<GroupRowsCommand>(m_presenter));
  addCommand(commands, make_unique<CopySelectedCommand>(m_presenter));
  addCommand(commands, make_unique<CutSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<PasteSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<ClearSelectedCommand>(m_presenter));
  addCommand(commands, make_unique<SeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DeleteRowCommand>(m_presenter));
  addCommand(commands, make_unique<DeleteGroupCommand>(m_presenter));
  return commands;
}

/** Reset the processed/error state for all rows
 */
void TwoLevelTreeManager::invalidateAllProcessed() {
  forEachGroup(*m_model,
               [this](int group) -> void { setProcessed(false, group); });
  forEachRow(*m_model, [this](int group, int row) -> void {
    setProcessed(false, row, group);
  });
  forEachGroup(*m_model, [this](int group) -> void { setError("", group); });
  forEachRow(*m_model,
             [this](int group, int row) -> void { setError("", row, group); });
}

/**
Insert a row after the last selected row. If a group was selected, the new row
is appended to that group. If nothing was selected, the new row is appended to
the last group in the table.
*/
void TwoLevelTreeManager::appendRow() {

  auto selectedGroups = m_presenter->selectedParents();
  auto selectedRows = m_presenter->selectedChildren();

  if (!selectedRows.empty()) {
    // Some rows were selected
    // Insert a row after last selected row

    int groupId = selectedRows.rbegin()->first;
    int lastSelectedRow = *(selectedRows[groupId].rbegin());
    insertRow(groupId, lastSelectedRow + 1);

  } else if (!selectedGroups.empty()) {
    // No rows were selected, but some groups were selected
    // Append row to last selected group

    int lastSelectedGroup = *(selectedGroups.rbegin());
    insertRow(lastSelectedGroup,
              m_model->rowCount(m_model->index(lastSelectedGroup, 0)));

  } else {
    // Nothing was selected

    if (m_model->rowCount() == 0) {
      // Model is empty, we cannot add a row
      return;
    }

    int groupId = m_model->rowCount() - 1;
    int rowId = m_model->rowCount(m_model->index(groupId, 0));

    // Add a new row to last group
    insertRow(groupId, rowId);
  }
}

void TwoLevelTreeManager::appendGroup() {
  auto selectedGroups = m_presenter->selectedParents();

  if (selectedGroups.empty()) {
    // Append group at the end of the table
    insertGroup(m_model->rowCount());
  } else {
    // Append group after last selected group
    insertGroup(*(selectedGroups.rbegin()) + 1);
  }
}

/**
Delete row(s) from the model
*/
void TwoLevelTreeManager::deleteRow() {

  auto selectedRows = m_presenter->selectedChildren();
  for (auto it = selectedRows.rbegin(); it != selectedRows.rend(); ++it) {
    const int groupId = it->first;
    auto rows = it->second;
    for (auto row = rows.rbegin(); row != rows.rend(); ++row) {
      m_model->removeRow(*row, m_model->index(groupId, 0));
    }
  }
}

/**
Delete group(s) from the model
*/
void TwoLevelTreeManager::deleteGroup() {
  auto selectedGroups = m_presenter->selectedParents();
  for (auto group = selectedGroups.rbegin(); group != selectedGroups.rend();
       ++group) {
    m_model->removeRow(*group);
  }
}

/**
Delete all rows and groups from the model
*/
void TwoLevelTreeManager::deleteAll() { m_model->removeAll(); }

/**
Group rows together
*/
void TwoLevelTreeManager::groupRows() {

  // Find if rows belong to the same group
  // If they do, do nothing
  // If they don't, remove rows from their groups and add them to a
  // new group

  const auto selectedRows = m_presenter->selectedChildren();

  if (selectedRows.empty()) {
    // no rows were selected
    return;
  }

  // Append a new group where selected rows will be pasted (this will append a
  // group with an empty row)
  int groupId = m_model->rowCount();
  appendGroup();
  // Append as many rows as the number of selected rows minus one
  const int rowsToAppend =
      std::accumulate(selectedRows.cbegin(), selectedRows.cend(), -1,
                      [](auto sum, const auto &row) {
                        return sum + static_cast<int>(row.second.size());
                      });
  for (int i = 0; i < rowsToAppend; i++)
    insertRow(groupId, i);

  // Now we just have to set the data
  int rowIndex = 0;
  for (const auto &item : selectedRows) {
    int oldGroupId = item.first;
    auto rows = item.second;
    for (const auto &row : rows) {
      for (int col = 0; col < m_model->columnCount() - 1; col++) {
        auto value = m_model->data(
            m_model->index(row, col, m_model->index(oldGroupId, 0)));
        m_model->setData(
            m_model->index(rowIndex, col, m_model->index(groupId, 0)), value);
      }
      rowIndex++;
    }
  }

  // Now delete the rows
  deleteRow();
}

/** Expands the current selection to all the rows in the selected groups, this
 * effectively means selecting the parent item (i.e. the group to which the
 * selected rows belong)
 * @return :: Groups containing selected rows
 */
std::set<int> TwoLevelTreeManager::expandSelection() {
  std::set<int> groupIds;

  auto items = m_presenter->selectedChildren();
  if (items.empty())
    return groupIds;

  for (auto &item : items)
    groupIds.insert(item.first);

  return groupIds;
}

/** Clear the currently selected rows */
void TwoLevelTreeManager::clearSelected() {

  const auto selectedRows = m_presenter->selectedChildren();

  for (const auto &item : selectedRows) {
    int group = item.first;
    auto rows = item.second;
    for (const auto &row : rows) {
      for (auto col = 0; col < m_model->columnCount(); col++)
        m_model->setData(m_model->index(row, col, m_model->index(group, 0)),
                         "");
    }
  }
}

/** Return the currently selected rows as a string */
QString TwoLevelTreeManager::copySelected() {
  QStringList lines;

  const auto selectedRows = m_presenter->selectedChildren();

  if (selectedRows.empty()) {
    return QString();
  }

  for (const auto &item : selectedRows) {
    const int group = item.first;
    auto rows = item.second;

    for (const auto &row : rows) {
      QStringList line;
      line.append(QString::number(group));

      for (int col = 0; col < m_model->columnCount(); ++col) {
        line.append(
            m_model->data(m_model->index(row, col, m_model->index(group, 0)))
                .toString());
      }
      lines.append(line.join("\t"));
    }
  }
  return lines.join("\n");
}

/** Paste the contents of the clipboard into the currently selected rows, or
 * append new rows
 * @param text :: Selected rows to paste as a string
 */
void TwoLevelTreeManager::pasteSelected(const QString &text) {

  if (text.isEmpty())
    return;

  // Contains the data to paste plus the original group index in the first
  // element
  auto lines = text.split("\n");

  // If we have rows selected, we'll overwrite them. If not, we'll append new
  // rows.
  const auto selectedRows = m_presenter->selectedChildren();
  if (selectedRows.empty()) {
    // No rows were selected
    // Use group where rows in clipboard belong and paste new rows to it
    // Add as many new rows as required
    for (auto i = 0; i < lines.size(); ++i) {
      auto values = lines[i].split("\t");
      auto const valuesSizeLessOne = static_cast<int>(values.size()) - 1;

      if (valuesSizeLessOne < 1)
        continue;

      auto groupId = parseDenaryInteger(values.front());
      int rowId = numRowsInGroup(groupId);
      if (!m_model->insertRow(rowId, m_model->index(groupId, 0)))
        return;
      for (int col = 0; col < m_model->columnCount() && col < valuesSizeLessOne;
           col++) {
        m_model->setData(m_model->index(rowId, col, m_model->index(groupId, 0)),
                         values[col + 1]);
      }
    }
  } else {
    // Some rows were selected
    // Iterate over rows and lines simultaneously, stopping when we reach the
    // end of either
    auto lineIt = lines.begin();
    for (const auto &selectedRow : selectedRows) {
      const int groupId = selectedRow.first;
      auto rows = selectedRow.second;
      auto rowIt = rows.begin();
      for (; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++) {
        auto values = (*lineIt).split("\t");
        auto const valuesSizeLessOne = static_cast<int>(values.size()) - 1;

        // Paste as many columns as we can from this line
        for (int col = 0;
             col < m_model->columnCount() && col < valuesSizeLessOne; ++col)
          m_model->setData(
              m_model->index(*rowIt, col, m_model->index(groupId, 0)),
              values[col + 1]);
      }
    }
  }
}

/** Opens a blank table
 * @param whitelist :: A whitelist with the columns for the new table
 */
void TwoLevelTreeManager::newTable(const WhiteList &whitelist) {

  m_model.reset(
      new QTwoLevelTreeModel(createDefaultWorkspace(whitelist), whitelist));
}

/** Opens a given table
 * @param table :: A table to open
 * @param whitelist :: A whitelist with the columns for the new table
 */
void TwoLevelTreeManager::newTable(ITableWorkspace_sptr table,
                                   const WhiteList &whitelist) {

  if (isValidModel(table, whitelist.size())) {
    m_model.reset(new QTwoLevelTreeModel(table, whitelist));
  } else
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");
}

/**
Inserts a new empty row to the specified group in the specified location
@param groupIndex :: The index to insert the new row after
@param rowIndex :: The index to insert the new row after
*/
void TwoLevelTreeManager::insertRow(int groupIndex, int rowIndex) {

  m_model->insertRow(rowIndex, m_model->index(groupIndex, 0));
}

/**
Inserts a new group in the specified location
@param groupIndex :: The index to insert the new row after
*/
void TwoLevelTreeManager::insertGroup(int groupIndex) {

  m_model->insertRow(groupIndex);
}

/** Returns how many rows there are in a given group
@param group : The group to count the rows of
@returns The number of rows in the group
*/
int TwoLevelTreeManager::numRowsInGroup(int group) const {

  return m_model->rowCount(m_model->index(group, 0));
}

/**
 * Returns given row data in a format that the presenter can understand and use
 * @return :: All data as a map where keys are units of post-processing (i.e.
 * group indices) and values are a map of row index in the group to row data
 */
TreeData TwoLevelTreeManager::constructTreeData(ChildItems rows) {
  TreeData tree;
  const int columnNotUsed = 0; // dummy value required to create index
  // Return row data in the format: map<int, set<vector<string>>>, where:
  // int -> group index
  // set<vector<string>> -> set of vectors storing the data. Each set is a row
  // and each element in the vector is a column
  for (const auto &item : rows) {
    int group = item.first;
    for (const auto &row : item.second) {
      tree[group][row] = m_model->rowData(
          m_model->index(row, columnNotUsed, m_model->index(group, 0)));
    }
  }
  return tree;
}

/**
 * Returns selected data in a format that the presenter can understand and use
 * @param prompt :: True if warning messages should be displayed. False othewise
 * @return :: Selected data as a map where keys are units of post-processing and
 * values are
 */
TreeData TwoLevelTreeManager::selectedData(bool prompt) {

  TreeData selectedData;

  auto options = m_presenter->options();

  if (m_model->rowCount() == 0 && prompt) {
    m_presenter->giveUserWarning("Cannot process an empty Table", "Warning");
    return selectedData;
  }

  // Selected groups
  auto groups = m_presenter->selectedParents();
  // Selected rows
  auto rows = m_presenter->selectedChildren();

  if (groups.empty() && rows.empty()) {

    if (options["WarnProcessAll"].toBool() && prompt) {
      if (!m_presenter->askUserYesNo(
              "This will process all rows in the table. Continue?",
              "Process all rows?"))
        return selectedData;
    }

    // They want to process everything
    // Populate all groups with all rows

    for (int group = 0; group < m_model->rowCount(); group++) {

      groups.insert(group);

      const auto nrows = numRowsInGroup(group);
      for (int row = 0; row < nrows; row++)
        rows[group].insert(row);
    }

  } else if (!groups.empty()) {

    // They have selected some groups
    // In this case we want to process and post-process the whole group,
    // so populate group with every row

    for (const auto &group : groups) {
      for (int row = 0; row < numRowsInGroup(group); row++)
        rows[group].insert(row);
    }

  } else {
    // They have selected some rows but no groups

    for (const auto &item : rows) {

      int group = item.first;
      auto rowSet = item.second;

      if (static_cast<int>(rowSet.size()) != numRowsInGroup(group)) {
        // Some groups will not be fully processed

        if (options["WarnProcessPartialGroup"].toBool() && prompt) {
          std::stringstream err;
          err << "Some groups will not be fully processed.";
          err << " Are you sure you want to continue?";
          if (!m_presenter->askUserYesNo(QString::fromStdString(err.str()),
                                         "Continue Processing?"))
            return selectedData;
          else
            break;
        }
      }
    }
  }

  return constructTreeData(rows);
}

/**
 * Returns all data in a format that the presenter can understand and use
 * @param prompt :: True if warning messages should be displayed. False othewise
 * @return :: data as a map
 */
TreeData TwoLevelTreeManager::allData(bool prompt) {

  TreeData allData;

  if (m_model->rowCount() == 0 && prompt) {
    m_presenter->giveUserWarning("Cannot process an empty Table", "Warning");
    return allData;
  }

  // Populate all groups with all rows
  ParentItems groups;
  ChildItems rows;

  for (int group = 0; group < m_model->rowCount(); group++) {
    groups.insert(group);

    const auto nrows = numRowsInGroup(group);
    for (int row = 0; row < nrows; row++)
      rows[group].insert(row);
  }

  return constructTreeData(rows);
}

/** Transfer data to the model
 * @param runs :: [input] Data to transfer as a vector of maps
 */
void TwoLevelTreeManager::transfer(
    const std::vector<std::map<QString, QString>> &runs) {
  m_model->transfer(runs);
}

/** Updates a row with new data
 * @param parent :: the parent item of the row
 * @param child :: the row
 * @param data :: the data
 */
void TwoLevelTreeManager::update(int parent, int child,
                                 const QStringList &data) {

  if (static_cast<int>(data.size()) != m_model->columnCount())
    throw std::invalid_argument("Can't update tree with given data");

  for (int col = 0; col < m_model->columnCount(); col++)
    m_model->setData(m_model->index(child, col, m_model->index(parent, 0)),
                     data[col]);
}

/** Gets the number of groups in the table
 * @return : Number of groups
 */
int TwoLevelTreeManager::rowCount() const { return m_model->rowCount(); }

/** Gets the number of rows of a parent group in the table
 * @param parent : Index of the parent group
 * @return : Number of rows of a group
 */
int TwoLevelTreeManager::rowCount(int parent) const {
  return m_model->rowCount(m_model->index(parent, 0));
}

/** Gets the 'process' status of a group
 * @param position : The row index
 * @return : 'process' status
 */
bool TwoLevelTreeManager::isProcessed(int position) const {
  return m_model->isProcessed(position);
}

/** Gets the 'process' status of a row
 * @param position : The row index
 * @param parent : The parent of the row
 * @return : 'process' status
 */
bool TwoLevelTreeManager::isProcessed(int position, int parent) const {
  return m_model->isProcessed(position, m_model->index(parent, 0));
}

/** Sets the 'process' status of a group
 * @param processed : True to set group as processed, false to set unprocessed
 * @param position : The index of the group to be set
 */
void TwoLevelTreeManager::setProcessed(bool processed, int position) {
  m_model->setProcessed(processed, position);
}

/** Sets the 'process' status of a row
 * @param processed : True to set row as processed, false to set unprocessed
 * @param position : The index of the row to be set
 * @param parent : The parent of the row
 */
void TwoLevelTreeManager::setProcessed(bool processed, int position,
                                       int parent) {
  m_model->setProcessed(processed, position, m_model->index(parent, 0));
}

/** Check whether reduction failed for a group
 * @param position : The row index
 * @return : true if there was an error
 */
bool TwoLevelTreeManager::reductionFailed(int position) const {
  return m_model->reductionFailed(position);
}

/** Check whether reduction failed for a row
 * @param position : The row index
 * @param parent : The parent of the row
 * @return : true if there was an error
 */
bool TwoLevelTreeManager::reductionFailed(int position, int parent) const {
  return m_model->reductionFailed(position, m_model->index(parent, 0));
}

/** Sets the error message of a group
 * @param error : the error message
 * @param position : The index of the group to be set
 */
void TwoLevelTreeManager::setError(const std::string &error, int position) {
  m_model->setError(error, position);
}

/** Sets the error message of a row
 * @param error : The error message
 * @param position : The index of the row to be set
 * @param parent : The parent of the row
 */
void TwoLevelTreeManager::setError(const std::string &error, int position,
                                   int parent) {
  m_model->setError(error, position, m_model->index(parent, 0));
}

/** Return a shared ptr to the model
 * @return :: A shared ptr to the model
 */
boost::shared_ptr<AbstractTreeModel> TwoLevelTreeManager::getModel() {
  return m_model;
}

/** Returns the table workspace containing the data
 * @return :: The table workspace
 */
ITableWorkspace_sptr TwoLevelTreeManager::getTableWorkspace() {

  return m_model->getTableWorkspace();
}

/**
 * Creates a default table using the whitelist supplied to this presenter
 * @param whitelist :: The whitelist that will be used to create a new table
 * @return : A default table
 */
ITableWorkspace_sptr
TwoLevelTreeManager::createDefaultWorkspace(const WhiteList &whitelist) {
  ITableWorkspace_sptr ws =
      Mantid::API::WorkspaceFactory::Instance().createTable();

  // First column is group
  auto column = ws->addColumn("str", "Group");
  column->setPlotType(0);

  for (const auto &columnName : whitelist.names()) {
    column = ws->addColumn("str", columnName.toStdString());
    column->setPlotType(0);
  }
  ws->appendRow();
  return ws;
}

/** Validate a table workspace
 * @param ws :: the table workspace
 * @param whitelistColumns :: the number of columns as specified in a whitelist
 */
void TwoLevelTreeManager::validateModel(ITableWorkspace_sptr ws,
                                        size_t whitelistColumns) const {

  if (!ws)
    throw std::runtime_error("Null pointer");

  // Table workspace must have one extra column, which corresponds to the
  // group
  if (ws->columnCount() != whitelistColumns + 1)
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
bool TwoLevelTreeManager::isValidModel(Workspace_sptr ws,
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
 * @param parentRow : the row index of the parent item
 * @param parentColumn : the column index of the parent item
 * @param value : the new value to populate the cell with
 */
void TwoLevelTreeManager::setCell(int row, int column, int parentRow,
                                  int parentColumn, const std::string &value) {

  m_model->setData(
      m_model->index(row, column, m_model->index(parentRow, parentColumn)),
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
std::string TwoLevelTreeManager::getCell(int row, int column, int parentRow,
                                         int parentColumn) const {

  return m_model
      ->data(
          m_model->index(row, column, m_model->index(parentRow, parentColumn)))
      .toString()
      .toStdString();
}

/**
 * Get number of rows.
 * @return the number of rows.
 */
int TwoLevelTreeManager::getNumberOfRows() { return m_model->rowCount(); }
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
