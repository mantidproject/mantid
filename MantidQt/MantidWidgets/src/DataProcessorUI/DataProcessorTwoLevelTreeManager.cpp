#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorTwoLevelTreeManager.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorAppendRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorClearSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCollapseGroupsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCopySelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorCutSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorDeleteRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorGroupRowsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorImportTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorNewTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorExpandGroupsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOpenTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorOptionsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPasteSelectedCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPauseCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotGroupCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorPlotRowCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorProcessCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableAsCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSaveTableCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/DataProcessorSeparatorCommand.h"
#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h"
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
DataProcessorTwoLevelTreeManager::DataProcessorTwoLevelTreeManager(
    DataProcessorPresenter *presenter, Mantid::API::ITableWorkspace_sptr table,
    const DataProcessorWhiteList &whitelist)
    : m_presenter(presenter),
      m_model(new QDataProcessorTwoLevelTreeModel(table, whitelist)) {}

/**
* Constructor (no table workspace given)
* @param presenter :: [input] The DataProcessor presenter
* @param whitelist :: [input] A whitelist containing the number of columns
*/
DataProcessorTwoLevelTreeManager::DataProcessorTwoLevelTreeManager(
    DataProcessorPresenter *presenter, const DataProcessorWhiteList &whitelist)
    : DataProcessorTwoLevelTreeManager(
          presenter, createDefaultWorkspace(whitelist), whitelist) {}

/**
* Destructor
*/
DataProcessorTwoLevelTreeManager::~DataProcessorTwoLevelTreeManager() {}

/**
* Publishes a list of available commands
* @return : The list of available commands
*/
std::vector<DataProcessorCommand_uptr>
DataProcessorTwoLevelTreeManager::publishCommands() {

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
  addCommand(commands, make_unique<DataProcessorExpandCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorExpandGroupsCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorCollapseGroupsCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotRowCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorPlotGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorAppendRowCommand>(m_presenter));
  addCommand(commands,
             make_unique<DataProcessorAppendGroupCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorSeparatorCommand>(m_presenter));
  addCommand(commands, make_unique<DataProcessorGroupRowsCommand>(m_presenter));
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
  addCommand(commands,
             make_unique<DataProcessorDeleteGroupCommand>(m_presenter));
  return commands;
}

/**
Insert a row after the last selected row. If a group was selected, the new row
is appended to that group. If nothing was selected, the new row is appended to
the last group in the table.
*/
void DataProcessorTwoLevelTreeManager::appendRow() {

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

void DataProcessorTwoLevelTreeManager::appendGroup() {
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
void DataProcessorTwoLevelTreeManager::deleteRow() {

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
void DataProcessorTwoLevelTreeManager::deleteGroup() {
  auto selectedGroups = m_presenter->selectedParents();
  for (auto group = selectedGroups.rbegin(); group != selectedGroups.rend();
       ++group) {
    m_model->removeRow(*group);
  }
}

/**
Group rows together
*/
void DataProcessorTwoLevelTreeManager::groupRows() {

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
  int rowsToAppend = -1;
  for (const auto &row : selectedRows)
    rowsToAppend += static_cast<int>(row.second.size());
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
std::set<int> DataProcessorTwoLevelTreeManager::expandSelection() {
  std::set<int> groupIds;

  auto items = m_presenter->selectedChildren();
  if (items.empty())
    return groupIds;

  for (auto group = items.begin(); group != items.end(); ++group)
    groupIds.insert(group->first);

  return groupIds;
}

/** Clear the currently selected rows */
void DataProcessorTwoLevelTreeManager::clearSelected() {

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
std::string DataProcessorTwoLevelTreeManager::copySelected() {
  std::vector<std::string> lines;

  const auto selectedRows = m_presenter->selectedChildren();

  if (selectedRows.empty()) {
    return std::string();
  }

  for (const auto &item : selectedRows) {
    const int group = item.first;
    auto rows = item.second;

    for (const auto &row : rows) {
      std::vector<std::string> line;
      line.push_back(std::to_string(group));

      for (int col = 0; col < m_model->columnCount(); ++col) {
        line.push_back(
            m_model->data(m_model->index(row, col, m_model->index(group, 0)))
                .toString()
                .toStdString());
      }
      lines.push_back(boost::algorithm::join(line, "\t"));
    }
  }
  return boost::algorithm::join(lines, "\n");
}

/** Paste the contents of the clipboard into the currently selected rows, or
* append new rows
* @param text :: Selected rows to paste as a string
*/
void DataProcessorTwoLevelTreeManager::pasteSelected(const std::string &text) {

  if (text.empty())
    return;

  // Contains the data to paste plus the original group index in the first
  // element
  std::vector<std::string> lines;
  boost::split(lines, text, boost::is_any_of("\n"));

  // If we have rows selected, we'll overwrite them. If not, we'll append new
  // rows.
  const auto selectedRows = m_presenter->selectedChildren();
  if (selectedRows.empty()) {
    // No rows were selected
    // Use group where rows in clipboard belong and paste new rows to it
    // Add as many new rows as required
    for (size_t i = 0; i < lines.size(); ++i) {
      std::vector<std::string> values;
      boost::split(values, lines[i], boost::is_any_of("\t"));

      int groupId = boost::lexical_cast<int>(values.front());
      int rowId = numRowsInGroup(groupId);
      if (!m_model->insertRow(rowId, m_model->index(groupId, 0)))
        return;
      for (int col = 0; col < m_model->columnCount(); col++) {
        m_model->setData(m_model->index(rowId, col, m_model->index(groupId, 0)),
                         QString::fromStdString(values[col + 1]));
      }
    }
  } else {
    // Some rows were selected
    // Iterate over rows and lines simultaneously, stopping when we reach the
    // end of either
    auto lineIt = lines.begin();
    for (auto it = selectedRows.begin(); it != selectedRows.end(); ++it) {
      const int groupId = it->first;
      auto rows = it->second;
      auto rowIt = rows.begin();
      for (; rowIt != rows.end() && lineIt != lines.end(); rowIt++, lineIt++) {
        std::vector<std::string> values;
        boost::split(values, *lineIt, boost::is_any_of("\t"));

        // Paste as many columns as we can from this line
        for (int col = 0; col < m_model->columnCount() &&
                              col < static_cast<int>(values.size());
             ++col)
          m_model->setData(
              m_model->index(*rowIt, col, m_model->index(groupId, 0)),
              QString::fromStdString(values[col + 1]));
      }
    }
  }
}

/** Opens a blank table
* @param whitelist :: A whitelist with the columns for the new table
*/
void DataProcessorTwoLevelTreeManager::newTable(
    const DataProcessorWhiteList &whitelist) {

  m_model.reset(new QDataProcessorTwoLevelTreeModel(
      createDefaultWorkspace(whitelist), whitelist));
}

/** Opens a given table
* @param table :: A table to open
* @param whitelist :: A whitelist with the columns for the new table
*/
void DataProcessorTwoLevelTreeManager::newTable(
    ITableWorkspace_sptr table, const DataProcessorWhiteList &whitelist) {

  if (isValidModel(table, whitelist.size())) {
    m_model.reset(new QDataProcessorTwoLevelTreeModel(table, whitelist));
  } else
    throw std::runtime_error("Selected table has the incorrect number of "
                             "columns to be used as a data processor table.");
}

/**
Inserts a new row to the specified group in the specified location
@param groupIndex :: The index to insert the new row after
@param rowIndex :: The index to insert the new row after
*/
void DataProcessorTwoLevelTreeManager::insertRow(int groupIndex, int rowIndex) {

  m_model->insertRow(rowIndex, m_model->index(groupIndex, 0));
}

/**
Inserts a new group in the specified location
@param groupIndex :: The index to insert the new row after
*/
void DataProcessorTwoLevelTreeManager::insertGroup(int groupIndex) {

  m_model->insertRow(groupIndex);
}

/** Returns how many rows there are in a given group
@param group : The group to count the rows of
@returns The number of rows in the group
*/
int DataProcessorTwoLevelTreeManager::numRowsInGroup(int group) const {

  return m_model->rowCount(m_model->index(group, 0));
}

/**
* Returns selected data in a format that the presenter can understand and use
* @param prompt :: True if warning messages should be displayed. False othewise
* @return :: Selected data as a map where keys are units of post-processing and
* values are
*/
TreeData DataProcessorTwoLevelTreeManager::selectedData(bool prompt) {

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
          if (!m_presenter->askUserYesNo(err.str(), "Continue Processing?"))
            return selectedData;
          else
            break;
        }
      }
    }
  }

  // Return selected data in the format: map<int, set<vector<string>>>, where:
  // int -> group index
  // set<vector<string>> -> set of vectors storing the data. Each set is a row
  // and each element in the vector is a column
  for (const auto &item : rows) {

    int group = item.first;

    for (const auto &row : item.second) {

      std::vector<std::string> data;
      for (int i = 0; i < m_model->columnCount(); i++)
        data.push_back(
            m_model->data(m_model->index(row, i, m_model->index(group, 0)))
                .toString()
                .toStdString());
      selectedData[group][row] = data;
    }
  }
  return selectedData;
}

/** Transfer data to the model
* @param runs :: [input] Data to transfer as a vector of maps
* @param whitelist :: [input] Whitelist containing number of columns
*/
void DataProcessorTwoLevelTreeManager::transfer(
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
    try {
      newRow << row.at("Group");
    } catch (std::out_of_range &) {
      throw std::invalid_argument("Data cannot be transferred to the "
                                  "processing table. Group information is "
                                  "missing.");
    }

    try {
      for (int i = 0; i < static_cast<int>(whitelist.size()); i++)
        newRow << row.at(whitelist.colNameFromColIndex(i));
    } catch (std::out_of_range &) {
      // OK, this column will not be populated
      continue;
    }
  }

  m_model.reset(new QDataProcessorTwoLevelTreeModel(ws, whitelist));
}

/** Updates a row with new data
* @param parent :: the parent item of the row
* @param child :: the row
* @param data :: the data
*/
void DataProcessorTwoLevelTreeManager::update(
    int parent, int child, const std::vector<std::string> &data) {

  if (static_cast<int>(data.size()) != m_model->columnCount())
    throw std::invalid_argument("Can't update tree with given data");

  for (int col = 0; col < m_model->columnCount(); col++)
    m_model->setData(m_model->index(child, col, m_model->index(parent, 0)),
                     QString::fromStdString(data[col]));
}

/** Gets the number of groups in the table
* @return : Number of groups
*/
int DataProcessorTwoLevelTreeManager::rowCount() const {
  return m_model->rowCount();
}

/** Gets the number of rows of a parent group in the table
* @param parent : Index of the parent group
* @return : Number of rows of a group
*/
int DataProcessorTwoLevelTreeManager::rowCount(int parent) const {
  return m_model->rowCount(m_model->index(parent, 0));
}

/** Gets the 'process' status of a group
* @param position : The row index
* @return : 'process' status
*/
bool DataProcessorTwoLevelTreeManager::isProcessed(int position) const {
  return m_model->isProcessed(position);
}

/** Gets the 'process' status of a row
* @param position : The row index
* @param parent : The parent of the row
* @return : 'process' status
*/
bool DataProcessorTwoLevelTreeManager::isProcessed(int position,
                                                   int parent) const {
  return m_model->isProcessed(position, m_model->index(parent, 0));
}

/** Sets the 'process' status of a group
* @param processed : True to set group as processed, false to set unprocessed
* @param position : The index of the group to be set
*/
void DataProcessorTwoLevelTreeManager::setProcessed(bool processed,
                                                    int position) {
  m_model->setProcessed(processed, position);
}

/** Sets the 'process' status of a row
* @param processed : True to set row as processed, false to set unprocessed
* @param position : The index of the row to be set
* @param parent : The parent of the row
*/
void DataProcessorTwoLevelTreeManager::setProcessed(bool processed,
                                                    int position, int parent) {
  m_model->setProcessed(processed, position, m_model->index(parent, 0));
}

/** Return a shared ptr to the model
* @return :: A shared ptr to the model
*/
boost::shared_ptr<AbstractDataProcessorTreeModel>
DataProcessorTwoLevelTreeManager::getModel() {
  return m_model;
}

/** Returns the table workspace containing the data
* @return :: The table workspace
*/
ITableWorkspace_sptr DataProcessorTwoLevelTreeManager::getTableWorkspace() {

  return m_model->getTableWorkspace();
}

/**
* Creates a default table using the whitelist supplied to this presenter
* @param whitelist :: The whitelist that will be used to create a new table
* @return : A default table
*/
ITableWorkspace_sptr DataProcessorTwoLevelTreeManager::createDefaultWorkspace(
    const DataProcessorWhiteList &whitelist) {
  ITableWorkspace_sptr ws =
      Mantid::API::WorkspaceFactory::Instance().createTable();

  // First column is group
  auto column = ws->addColumn("str", "Group");
  column->setPlotType(0);

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
void DataProcessorTwoLevelTreeManager::validateModel(
    ITableWorkspace_sptr ws, size_t whitelistColumns) const {

  if (!ws)
    throw std::runtime_error("Null pointer");

  // Table workspace must have one extra column, which corresponds to the
  // group
  if (ws->columnCount() != whitelistColumns + 1)
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
bool DataProcessorTwoLevelTreeManager::isValidModel(
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
