// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidKernel/StringTokenizer.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
using namespace Mantid::API;

// Utility class to hold information about a row
class RowInfo {
public:
  RowInfo() = delete;
  // Constructor taking a column count creates an empty data value list of that
  // size
  RowInfo(const size_t absoluteIndex, const int columnCount)
      : m_absoluteIndex(absoluteIndex),
        m_rowData(std::make_shared<RowData>(columnCount)) {}
  // Constructor taking a list of data values
  RowInfo(const size_t absoluteIndex, QStringList rowDataValues)
      : m_absoluteIndex(absoluteIndex),
        m_rowData(std::make_shared<RowData>(std::move(rowDataValues))) {}

  size_t absoluteIndex() const { return m_absoluteIndex; }
  RowData_sptr rowData() const { return m_rowData; }
  bool isProcessed() const { return m_rowData->isProcessed(); }
  void setProcessed(const bool isProcessed) const {
    m_rowData->setProcessed(isProcessed);
    // Also clear the error if resetting processed state
    if (!isProcessed)
      m_rowData->setError("");
  }
  bool reductionFailed() const { return m_rowData->reductionFailed(); }
  std::string error() const { return m_rowData->error(); }
  void setError(const std::string &error) { m_rowData->setError(error); }
  void setAbsoluteIndex(const size_t absoluteIndex) {
    m_absoluteIndex = absoluteIndex;
  }

private:
  // The row's absolute index in the table
  size_t m_absoluteIndex;
  // The row's data values and metadata
  RowData_sptr m_rowData;
};

// Utility class to hold information about a group
class GroupInfo {
public:
  GroupInfo() : m_name(""), m_isProcessed(false) {}
  explicit GroupInfo(const std::string &name)
      : m_name(name), m_isProcessed(false) {}

  std::string name() const { return m_name; }
  void setName(const std::string &name) { m_name = name; }
  bool isProcessed() const { return m_isProcessed; }
  void setProcessed(const bool isProcessed) { m_isProcessed = isProcessed; }
  bool allRowsProcessed() const {
    return std::all_of(m_rows.cbegin(), m_rows.cend(),
                       [](const auto &row) { return row.isProcessed(); });
  }
  // Get/set error
  std::string error() const {
    // Return the group's error, if set
    if (!m_error.empty())
      return m_error;
    // If the group's error is not set but some row errors are, then
    // report that some rows failed
    if (std::any_of(m_rows.cbegin(), m_rows.cend(),
                    [](const auto &row) { return !row.error().empty(); })) {
      return "Some rows in the group have errors";
    }
    // Return an empty string if there is no error
    return std::string();
  }
  void setError(const std::string &error) { m_error = error; }
  // Return true if reduction failed for the group or any rows within it
  bool reductionFailed() const {
    if (!m_error.empty())
      return true;
    return std::any_of(m_rows.cbegin(), m_rows.cend(),
                       [](auto const &row) { return row.reductionFailed(); });
  }
  // Get the row data for the given row index
  RowData_sptr rowData(const size_t rowIndex) const {
    checkRowIndex(rowIndex);
    return m_rows[rowIndex].rowData();
  }
  // Get the row's processed status for the given row index
  bool isRowProcessed(const size_t rowIndex) const {
    checkRowIndex(rowIndex);
    return m_rows[rowIndex].isProcessed();
  }
  // Check whether a row failed
  bool rowReductionFailed(const size_t rowIndex) const {
    checkRowIndex(rowIndex);
    return m_rows[rowIndex].reductionFailed();
  }
  // Set the row's processed status for the given row index
  void setRowProcessed(const size_t rowIndex, const bool isProcessed) const {
    checkRowIndex(rowIndex);
    m_rows[rowIndex].setProcessed(isProcessed);
  }
  // Get/set an error on a row for the given row index
  std::string rowError(const size_t rowIndex) const {
    checkRowIndex(rowIndex);
    return m_rows[rowIndex].error();
  }
  void setRowError(const size_t rowIndex, const std::string &error) {
    checkRowIndex(rowIndex);
    m_rows[rowIndex].setError(error);
  }
  // Get the row's absolute index for the given row index in the group
  size_t rowAbsoluteIndex(const size_t rowIndex) const {
    checkRowIndex(rowIndex);
    return m_rows[rowIndex].absoluteIndex();
  }
  // Get the last row's absolute index
  size_t lastRowAbsoluteIndex() const { return m_rows.back().absoluteIndex(); }
  // Get the number of rows in the group
  size_t rowCount() const { return m_rows.size(); }
  // Add a new row with the given absolute index and the given number of rows
  void addRow(const size_t absoluteIndex, const int columnCount) {
    m_rows.emplace_back(RowInfo(absoluteIndex, columnCount));
  }
  // Add a new row to the group at the given local row index
  // Note that new rows have an absolute index of 0 which must then
  // be set correctly by the caller.
  void insert(const size_t position, const int numToInsert,
              const int columnCount) {
    m_rows.insert(m_rows.begin() + position, numToInsert,
                  RowInfo(0, columnCount));
  }
  // Remove rows from the group
  void erase(const size_t position, const int numToErase) {
    m_rows.erase(m_rows.begin() + position,
                 m_rows.begin() + position + numToErase);
  }
  // Set the absolute index for a row
  void setRowAbsoluteIndex(const size_t localIndex,
                           const size_t absoluteIndex) {
    checkRowIndex(localIndex);
    m_rows[localIndex].setAbsoluteIndex(absoluteIndex);
  }

private:
  // Check a row index is valid and throw if not
  void checkRowIndex(const size_t rowIndex) const {
    if (rowIndex >= m_rows.size())
      throw std::runtime_error(
          "Attempted to access row index outside group's size");
  }
  // The group's name
  std::string m_name;
  // Whether the group has been processed
  bool m_isProcessed;
  // An error message, if reduction failed for this group
  std::string m_error;
  // The list of rows in this group
  std::vector<RowInfo> m_rows;
};

//----------------------------------------------------------------------------------------------
/** Constructor
@param tableWorkspace : The table workspace to wrap
@param whitelist : A WhiteList containing information about the
columns, their indices and descriptions
*/
QTwoLevelTreeModel::QTwoLevelTreeModel(ITableWorkspace_sptr tableWorkspace,
                                       const WhiteList &whitelist)
    : AbstractTreeModel(tableWorkspace, whitelist) {

  if (tableWorkspace->columnCount() != m_whitelist.size() + 1)
    throw std::invalid_argument("Invalid table workspace. Table workspace must "
                                "have one extra column accounting for groups");

  // Sort the table workspace by group, i.e. first column
  std::vector<std::pair<std::string, bool>> criteria = {
      std::make_pair(tableWorkspace->getColumnNames().at(0), true)};
  m_tWS->sort(criteria);

  setupModelData(tableWorkspace);

  connect(this, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this,
          SLOT(tableDataUpdated(const QModelIndex &, const QModelIndex &)));
}

QTwoLevelTreeModel::~QTwoLevelTreeModel() {}

/** Return the Edit role data
 */
QVariant QTwoLevelTreeModel::getEditRole(const QModelIndex &index) const {
  return getDisplayRole(index);
}

/** Returns true if the given index corresponds to a group; false if it
    corresponds to a row
 */
bool QTwoLevelTreeModel::indexIsGroup(const QModelIndex &index) const {
  return (!parent(index).isValid());
}

/** Return the Display role data
 */
QVariant QTwoLevelTreeModel::getDisplayRole(const QModelIndex &index) const {
  if (indexIsGroup(index)) {
    const auto &group = m_groups.at(index.row());
    // Return the group name only in the first column
    if (index.column() == 0)
      return QString::fromStdString(group.name());
  } else {
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];
    return QString::fromStdString(
        m_tWS->String(group.rowAbsoluteIndex(index.row()), index.column() + 1));
  }

  return QVariant();
}

/** Return the Background role data
 */
QVariant QTwoLevelTreeModel::getBackgroundRole(const QModelIndex &index) const {
  if (indexIsGroup(index)) {
    const auto &group = m_groups.at(index.row());
    // Highlight if this group is processed
    if (group.reductionFailed())
      return QColor(Colour::FAILED);
    else if (group.isProcessed())
      return QColor(Colour::SUCCESS);
    else if (group.allRowsProcessed())
      return QColor(Colour::COMPLETE);
  } else {
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];
    // Highlight if this row is processed (red if failed, green if success)
    if (group.rowReductionFailed(index.row()))
      return QColor(Colour::FAILED);
    else if (group.isRowProcessed(index.row()))
      return QColor(Colour::SUCCESS);
  }

  return QVariant();
}

/** Return the ToolTip role data
 */
QVariant QTwoLevelTreeModel::getToolTipRole(const QModelIndex &index) const {
  if (indexIsGroup(index)) {
    const auto &group = m_groups.at(index.row());
    return QString::fromStdString(group.error());
  } else {
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];
    return QString::fromStdString(group.rowError(index.row()));
  }

  return QVariant();
}

/** Returns data for specified index
 * @param index : The index
 * @param role : The role
 * @return : The data associated with the given index
 */
QVariant QTwoLevelTreeModel::data(const QModelIndex &index, int role) const {

  if (!index.isValid())
    return QVariant();

  switch (role) {
  case Qt::DisplayRole:
    return getDisplayRole(index);
  case Qt::EditRole:
    return getEditRole(index);
  case Qt::BackgroundRole:
    return getBackgroundRole(index);
  case Qt::ToolTipRole:
    return getToolTipRole(index);
  default:
    return QVariant();
  }
}

/** Utility to get the data for a cell from the group/row/column index
 */
std::string QTwoLevelTreeModel::cellValue(int groupIndex, int rowIndex,
                                          int columnIndex) const {
  const auto rowQIndex =
      index(rowIndex, columnIndex, index(groupIndex, columnIndex));
  auto result = data(rowQIndex).toString().toStdString();

  // Treat auto-generated values as empty cells
  auto currentRowData = rowData(groupIndex, rowIndex);
  if (currentRowData->isGenerated(columnIndex))
    result = "";

  return result;
}

/** Returns the column name (header data for given section)
 * @param section : The section (column) index
 * @param orientation : The orientation
 * @param role : The role
 * @return : The column name
 */
QVariant QTwoLevelTreeModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return m_whitelist.name(section);

  if (orientation == Qt::Horizontal && role == Qt::WhatsThisRole)
    return m_whitelist.description(section);

  return QVariant();
}

/** Returns row data struct (which includes metadata about the row)
 * for specified index
 * @param index : The index
 * @return : The data associated with the given index as a RowData class
 */
RowData_sptr QTwoLevelTreeModel::rowData(const QModelIndex &index) const {

  RowData_sptr result;

  if (!index.isValid())
    return result;

  if (indexIsGroup(index)) {
    return result;
  } else {
    // Index corresponds to a row
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];
    return group.rowData(index.row());
  }

  return result;
}

/** Returns row data struct (which includes metadata about the row)
 * for specified index
 * @param groupIndex : The group index
 * @param rowIndex : The row index within the group
 * @return : The data associated with the given index as a RowData class
 */
RowData_sptr QTwoLevelTreeModel::rowData(int groupIndex, int rowIndex) const {
  const auto rowQIndex = index(rowIndex, 0, index(groupIndex, 0));
  return rowData(rowQIndex);
}

/** Returns the index of an element specified by its row, column and parent
 * @param row : The row
 * @param column : The column
 * @param parent : The parent element
 * @return : The index of the element
 */
QModelIndex QTwoLevelTreeModel::index(int row, int column,
                                      const QModelIndex &parent) const {

  return parent.isValid() ? createIndex(row, column, parent.row())
                          : createIndex(row, column, -1);
}

/** Gets the 'processed' status of a data item
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : The 'processed' status
 */
bool QTwoLevelTreeModel::isProcessed(int position,
                                     const QModelIndex &parent) const {

  if (!parent.isValid()) {
    // We have a group item (no parent)

    // Invalid position
    if (position < 0 || position >= rowCount())
      throw std::invalid_argument("Invalid position. Position index must be "
                                  "within the range of the number of groups in "
                                  "this model");

    return m_groups[position].isProcessed();
  } else {
    // We have a row item (parent exists)

    // Invalid position
    if (position < 0 || position >= rowCount(parent))
      throw std::invalid_argument("Invalid position. Position index must be "
                                  "within the range of the number of rows in "
                                  "the given group for this model");

    return m_groups[parent.row()].isRowProcessed(position);
  }
}

/** Check whether the reduction failed for a group/row
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : true if the reduction failed
 */
bool QTwoLevelTreeModel::reductionFailed(int position,
                                         const QModelIndex &parent) const {

  if (!parent.isValid()) {
    // We have a group item (no parent)

    // Invalid position
    if (position < 0 || position >= rowCount())
      throw std::invalid_argument("Invalid position. Position index must be "
                                  "within the range of the number of groups in "
                                  "this model");

    return m_groups[position].reductionFailed();
  } else {
    // We have a row item (parent exists)

    // Invalid position
    if (position < 0 || position >= rowCount(parent))
      throw std::invalid_argument("Invalid position. Position index must be "
                                  "within the range of the number of rows in "
                                  "the given group for this model");

    return m_groups[parent.row()].rowReductionFailed(position);
  }
}

/** Returns the parent of a given index
 * @param index : The index
 * @return : Its parent
 */
QModelIndex QTwoLevelTreeModel::parent(const QModelIndex &index) const {

  int internalIdInt = int(index.internalId());

  return internalIdInt >= 0 ? createIndex(internalIdInt, 0, -1) : QModelIndex();
}

/** Adds elements to the tree
 * @param position : The position where to insert the new elements
 * @param count : The number of elements to insert
 * @param parent : The parent of the set of elements
 * @return : Boolean indicating whether the insertion was successful or not
 */
bool QTwoLevelTreeModel::insertRows(int position, int count,
                                    const QModelIndex &parent) {

  bool success = false;

  if (!parent.isValid()) {
    // Group
    success = insertGroups(position, count);
  } else {
    // Row
    success = insertRows(position, count, parent.row());
  }

  return success;
}

/** Insert new rows as children of a given parent. Parent must exist.
 * @param position : The position where new rows will be added
 * @param count : The number of new rows to insert
 * @param parent : The parent index (as integer)
 * @return : Boolean indicating if the insertion was successful
 */
bool QTwoLevelTreeModel::insertRows(int position, int count, int parent) {

  // Parent does not exist
  if (parent < 0 || parent >= rowCount())
    return false;

  // Incorrect position
  if (position < 0 || position > rowCount(index(parent, 0)))
    return false;

  // Incorrect number of rows
  if (count < 1)
    return false;

  // We need to update the absolute positions of the rows and the table
  // workspace

  beginInsertRows(index(parent, 0), position, position + count - 1);

  // Update the table workspace

  auto &group = m_groups[parent];
  size_t absolutePosition = 0;
  if (group.rowCount() > 0) {
    if (position == static_cast<int>(group.rowCount()))
      absolutePosition = group.rowAbsoluteIndex(position - 1) + 1;
    else
      absolutePosition = group.rowAbsoluteIndex(position);
  } else {
    absolutePosition =
        (parent > 0) ? m_groups[parent - 1].lastRowAbsoluteIndex() + 1 : 0;
  }

  for (int pos = position; pos < position + count; pos++) {
    m_tWS->insertRow(absolutePosition);
    m_tWS->String(absolutePosition, 0) = group.name();
  }

  size_t lastRowIndex = absolutePosition;

  // Insert new elements
  group.insert(position, count, columnCount());

  // Update row indices in the group where we are adding the new rows
  for (int pos = position; pos < rowCount(index(parent, 0)); pos++)
    group.setRowAbsoluteIndex(pos, lastRowIndex++);

  // Update row indices in subsequent groups
  for (int groupIdx = parent + 1; groupIdx < rowCount(); groupIdx++) {
    for (int row = 0; row < rowCount(index(groupIdx, 0)); row++)
      m_groups[groupIdx].setRowAbsoluteIndex(row, lastRowIndex++);
  }

  endInsertRows();

  return true;
}

/** Insert new groups at a given position
 * @param position : The position where new groups will be inserted
 * @param count : The number of groups to insert
 * @return : True if insertion was successful, false otherwise
 */
bool QTwoLevelTreeModel::insertGroups(int position, int count) {

  // Invalid position
  if (position < 0 || position > rowCount())
    return false;

  // Invalid number of groups
  if (count < 1)
    return false;

  beginInsertRows(QModelIndex(), position, position + count - 1);

  // Insert new groups into the list
  m_groups.insert(m_groups.begin() + position, count, GroupInfo());

  for (int pos = position; pos < position + count; pos++) {
    // Add one row to this new group
    insertRows(0, 1, pos);
    //// Update the table workspace
    // m_tWS->insertRow(pos);
  }

  endInsertRows();

  return true;
}

/** Removes elements from the tree
 * @param position : The position of the first element in the set to be removed
 * @param count : The number of elements to remove
 * @param parent : The parent of the set of elements
 * @return : Boolean indicating whether the elements were removed successfully
 * or not
 */
bool QTwoLevelTreeModel::removeRows(int position, int count,
                                    const QModelIndex &parent) {

  bool success = false;

  if (!parent.isValid()) {
    // Group
    success = removeGroups(position, count);
  } else {
    // Row
    success = removeRows(position, count, parent.row());
  }

  return success;
}

/** Removes groups from the tree
 * @param position : The position of the first group that will be removed
 * @param count : The number of groups to remove
 * @return : Boolean indicating whether or not groups were removed
 */
bool QTwoLevelTreeModel::removeGroups(int position, int count) {

  // Invalid position
  if (position < 0 || position >= rowCount())
    return false;

  // Invalid number of groups
  if (count < 1 || position + count > rowCount())
    return false;

  beginRemoveRows(QModelIndex(), position, position + count - 1);

  // Update table workspace

  size_t absolutePosition =
      (m_groups[position].rowCount() > 0)
          ? m_groups[position].rowAbsoluteIndex(0)
          : (position > 0) ? m_groups[position - 1].lastRowAbsoluteIndex() + 1
                           : 0;

  for (int group = position; group < position + count; group++) {
    for (int row = 0; row < rowCount(index(group, 0)); row++)
      m_tWS->removeRow(absolutePosition);
  }

  m_groups.erase(m_groups.begin() + position,
                 m_groups.begin() + position + count);

  if (m_groups.size() > 0) {
    // Update row positions
    for (int group = position; group < rowCount(); group++) {
      for (int row = 0; row < rowCount(index(group, 0)); row++)
        m_groups[group].setRowAbsoluteIndex(row, absolutePosition++);
    }
  } else {
    m_groups.clear();
  }

  endRemoveRows();

  return true;
}

/** Removes rows from a group
 * @param position : The position of the first row that will be removed
 * @param count : The number of rows to remove
 * @param parent : The parent item
 * @return : Boolean indicating whether or not rows were removed
 */
bool QTwoLevelTreeModel::removeRows(int position, int count, int parent) {

  // Parent does not exist
  if (parent < 0 || parent >= rowCount())
    return false;

  // Parent has no children
  if (rowCount(index(parent, 0)) < 1)
    return false;

  // Incorrect position
  if (position < 0 || position >= rowCount(index(parent, 0)))
    return false;

  // Incorrect number of rows
  if (count < 1 || position + count > rowCount(index(parent, 0)))
    return false;

  beginRemoveRows(index(parent, 0), position, position + count - 1);

  // Update the table workspace
  size_t absolutePosition = m_groups[parent].rowAbsoluteIndex(position);
  for (int pos = position; pos < position + count; pos++) {
    m_tWS->removeRow(absolutePosition);
  }

  m_groups[parent].erase(position, count);

  // Update row indices in this group
  for (int row = position; row < rowCount(index(parent, 0)); row++) {
    m_groups[parent].setRowAbsoluteIndex(row, absolutePosition++);
  }

  // Update row indices in subsequent groups
  for (int group = parent + 1; group < rowCount(); group++)
    for (int row = 0; row < rowCount(index(group, 0)); row++)
      m_groups[group].setRowAbsoluteIndex(row, absolutePosition++);

  if (m_groups[parent].rowCount() == 0) {
    removeGroups(parent, 1);
  }

  endRemoveRows();

  return true;
}

/** Remove all rows and groups
 * @return : Boolean indicating whether or not rows were removed
 */
bool QTwoLevelTreeModel::removeAll() {
  beginRemoveRows(QModelIndex(), 0, rowCount() - 1);

  for (int group = 0; group < rowCount(); ++group) {
    for (int row = 0; row < rowCount(index(group, 0)); ++row) {
      m_tWS->removeRow(0);
    }
  }

  m_groups.clear();

  endRemoveRows();

  return true;
}

/** Returns the number of rows of a given parent
 * @param parent : The parent item
 * @return : The number of rows
 */
int QTwoLevelTreeModel::rowCount(const QModelIndex &parent) const {

  // We are counting the number of groups
  if (!parent.isValid())
    return static_cast<int>(m_groups.size());

  // This shouldn't happen
  if (parent.parent().isValid())
    return 0;

  // This group does not exist anymore
  if (parent.row() >= static_cast<int>(m_groups.size()))
    return 0;

  // Group exists, return number of children
  return static_cast<int>(m_groups[parent.row()].rowCount());
}

/** Updates an index with given data
 * @param index : the index
 * @param value : the new value
 * @param role : the role
 */
bool QTwoLevelTreeModel::setData(const QModelIndex &index,
                                 const QVariant &value, int role) {

  if (role != Qt::EditRole)
    return false;

  const std::string newName = value.toString().toStdString();

  if (indexIsGroup(index)) {
    // Index corresponds to a group

    if (index.column() != 0)
      return false;

    if (m_groups[index.row()].name() == newName)
      return false;

    // Update the group name, which means updating:

    // 1. Auxiliary member variables

    m_groups[index.row()].setName(newName);

    // 2. Table workspace

    size_t nrows = m_groups[index.row()].rowCount();
    for (size_t row = 0; row < nrows; row++) {
      m_tWS->String(m_groups[index.row()].rowAbsoluteIndex(row), 0) = newName;
    }
  } else {
    // Index corresponds to a row

    // First we need to find the absolute position of this row in the table
    size_t absolutePosition =
        m_groups[parent(index).row()].rowAbsoluteIndex(index.row());

    if (m_tWS->String(absolutePosition, index.column() + 1) == newName)
      return false;

    m_tWS->String(absolutePosition, index.column() + 1) = newName;
  }

  emit dataChanged(index, index);

  return true;
}

/** Setup the data, initialize member variables using a table workspace and
 * whitelist
 * @param table : A table workspace containing the data
 */
void QTwoLevelTreeModel::setupModelData(ITableWorkspace_sptr table) {

  int nrows = static_cast<int>(table->rowCount());

  int lastIndex = 0;
  std::map<std::string, int> groupIndex;

  for (int r = 0; r < nrows; r++) {

    const std::string &groupName = m_tWS->String(r, 0);

    // If the group doesn't exist yet, add a new group with this name to the
    // groups list
    if (groupIndex.count(groupName) == 0) {
      groupIndex[groupName] = lastIndex++;
      m_groups.emplace_back(GroupInfo(groupName));
    }

    // Add a new row to the group with the correct number of columns
    m_groups[groupIndex[groupName]].addRow(r, columnCount());
  }

  // Update values in the cached group data from the table
  updateAllGroupData();
}

/** Return the underlying data structure, i.e. the table workspace this model is
 * representing
 * @return :: the underlying table workspace
 */
ITableWorkspace_sptr QTwoLevelTreeModel::getTableWorkspace() const {
  return m_tWS;
}

/** Sets the 'processed' status of a data item
 * @param processed : True to set processed, false to set unprocessed
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : Boolean indicating whether process status was set successfully
 */
bool QTwoLevelTreeModel::setProcessed(bool processed, int position,
                                      const QModelIndex &parent) {

  if (!parent.isValid()) {
    // We have a group item (no parent)

    // Invalid position
    if (position < 0 || position >= rowCount())
      return false;

    m_groups[position].setProcessed(processed);
  } else {
    // We have a row item (parent exists)

    // Invalid position
    if (position < 0 || position >= rowCount(parent))
      return false;

    m_groups[parent.row()].setRowProcessed(position, processed);
  }

  return true;
}

/** Sets the error status of a data item
 * @param error : the error message
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : Boolean indicating whether process status was set successfully
 */
bool QTwoLevelTreeModel::setError(const std::string &error, int position,
                                  const QModelIndex &parent) {

  if (!parent.isValid()) {
    // We have a group item (no parent)

    // Invalid position
    if (position < 0 || position >= rowCount())
      return false;

    m_groups[position].setError(error);
  } else {
    // We have a row item (parent exists)

    // Invalid position
    if (position < 0 || position >= rowCount(parent))
      return false;

    m_groups[parent.row()].setRowError(position, error);
  }

  return true;
}

/** Update cached data for all rows in the given group from the table
 * @param groupIdx : the group index to update
 * @param start : the first row index in the group to update
 * @param end : the last row index in the group to update
 */
void QTwoLevelTreeModel::updateGroupData(const int groupIdx, const int start,
                                         const int end) {
  // Loop through all groups and all rows
  auto &group = m_groups[groupIdx];
  for (int row = start; row <= end; ++row) {
    const auto rowData = group.rowData(row);
    // Loop through all columns and update the value in the row data
    for (int col = 0; col < columnCount(); ++col) {
      auto value = data(index(row, col, index(groupIdx, 0))).toString();
      if (value != rowData->value(col))
        rowData->setValue(col, value);
    }
  }
}

void QTwoLevelTreeModel::updateAllGroupData() {
  // Loop through all groups and all rows
  for (int groupIdx = 0; groupIdx < rowCount(); ++groupIdx) {
    updateGroupData(groupIdx, 0, rowCount(index(groupIdx, 0)) - 1);
  }
}

/** Called when the data in the table has changed. Updates the
 * table values in the cached RowData
 */
void QTwoLevelTreeModel::tableDataUpdated(const QModelIndex &topLeft,
                                          const QModelIndex &bottomRight) {
  if (!topLeft.isValid() || !bottomRight.isValid() ||
      !topLeft.parent().isValid() || !bottomRight.parent().isValid())
    return;

  if (topLeft.parent() != bottomRight.parent())
    return;

  const auto group = topLeft.parent().row();
  const auto start = topLeft.row();
  const auto end = bottomRight.row();

  // Reset the processed state for all changed rows and their parent group
  setProcessed(false, group);
  setError("", group);
  for (int i = start; i <= end; ++i) {
    setProcessed(false, i, index(group, 0));
    setError("", i, index(group, 0));
  }

  // Update cached row data from the values in the table
  updateGroupData(group, start, end);
}

int QTwoLevelTreeModel::findOrAddGroup(const std::string &groupName) {
  // Return the index of the group if it exists
  auto groupIter = std::find_if(m_groups.begin(), m_groups.end(),
                                [&groupName](const GroupInfo &group) -> bool {
                                  return group.name() == groupName;
                                });

  if (groupIter == m_groups.end()) {
    // Not found. Add a new group and return its index
    m_groups.emplace_back(GroupInfo(groupName));
    return static_cast<int>(m_groups.size()) - 1;
  } else {
    // Return the existing group's index
    return static_cast<int>(groupIter - m_groups.begin());
  }
}

/** Checks whether the existing row is empty
 * @return : true if all of the values in the row are empty
 */
bool QTwoLevelTreeModel::rowIsEmpty(int row, int parent) const {
  // Loop through all columns and return false if any are not empty
  for (int columnIndex = 0; columnIndex < columnCount(); ++columnIndex) {
    auto value = data(index(row, columnIndex, index(parent, 0)))
                     .toString()
                     .toStdString();
    if (!value.empty())
      return false;
  }

  // All cells in the row were empty
  return true;
}

/** This function checks whether two lists of runs match or partially match. If
 * the original list only contains one of the runs, say '12345', and a
 * subsequent list contained two, e.g. '12345+22345' then we need to identify
 * that this is the same row and that it needs updating with the new run
 * numbers (so it is considered a match but not an exact match).  If the
 * original list contains both run numbers and the new list contains a run that
 * already exists in that row then the rows are considered to be an exact match
 * because no new runs need to be added.
 */
bool QTwoLevelTreeModel::runListsMatch(const std::string &newValue,
                                       const std::string &oldValue,
                                       const bool exactMatch) const {
  // Parse the individual runs from each list and check that they all
  // match, allowing for additional runs in one of the lists.
  auto newRuns = Mantid::Kernel::StringTokenizer(
                     newValue, ",+", Mantid::Kernel::StringTokenizer::TOK_TRIM)
                     .asVector();
  auto oldRuns = Mantid::Kernel::StringTokenizer(
                     oldValue, ",+", Mantid::Kernel::StringTokenizer::TOK_TRIM)
                     .asVector();

  // Loop through all values in the shortest list and check they exist
  // in the longer list (or they all match if they're the same length).
  auto longList = newRuns.size() > oldRuns.size() ? newRuns : oldRuns;
  auto shortList = newRuns.size() > oldRuns.size() ? oldRuns : newRuns;
  for (auto &run : shortList) {
    if (!std::count(longList.cbegin(), longList.cend(), run))
      return false;
  }

  // Ok, the short list only contains items in the long list. If the new
  // list contains additional items that are not in the old list then the
  // row will require updating and this is not an exact match.  However,
  // if the new list contains fewer items then the row does not updating
  // because they already all exist in the old list
  if (exactMatch && newRuns.size() > oldRuns.size())
    return false;

  return true;
}

bool QTwoLevelTreeModel::checkColumnInComparisons(const Column &column,
                                                  const bool exactMatch) const {
  // If looking for exact matches, check all columns
  if (exactMatch)
    return true;

  // If the whitelist does not have any key columns treat them all as key
  // columns,
  // i.e. check all columns
  if (!m_whitelist.hasKeyColumns())
    return true;

  // Otherwise, only check key columns
  return column.isKey();
}

/** Check whether the given row in the model matches the given row values
 * @param groupIndex : the group to check in the model
 * @param rowIndex : the row to check in the model
 * @param rowValues : the cell values to check against
 * @param exactMatch : whether to match the entire row exactly or just
 * the key columns
 * @return : true if the cell matches the given value
 */
bool QTwoLevelTreeModel::rowMatches(int groupIndex, int rowIndex,
                                    const std::map<QString, QString> &rowValues,
                                    const bool exactMatch) const {

  int columnIndex = 0;
  for (auto columnIt = m_whitelist.begin(); columnIt != m_whitelist.end();
       ++columnIt, ++columnIndex) {
    const auto column = *columnIt;

    // Skip if no value for this column is given
    if (!rowValues.count(column.name()))
      continue;

    auto newValue = rowValues.at(column.name()).toStdString();
    auto oldValue = cellValue(groupIndex, rowIndex, columnIndex);

    // Special case for runs column to allows for new runs to be added into
    // rows that already contain a partial list of runs for the same angle
    if (column.name() == "Run(s)") {
      if (!runListsMatch(newValue, oldValue, exactMatch))
        return false;
      continue;
    }

    if (!checkColumnInComparisons(column, exactMatch))
      continue;

    // Ok, compare the values
    if (newValue != oldValue) {
      return false;
    }
  }

  return true;
}

/** Find the index of a row in a group based on row data values.
 * @param groupIndex : the index of the group the row is in
 * @param rowValues : the row values to look for
 * @return : an optional value that is set with the row's index if
 * it was found or is unset if it is not
 */
boost::optional<int> QTwoLevelTreeModel::findRowIndex(
    int groupIndex, const std::map<QString, QString> &rowValues) const {
  boost::optional<int> result;
  // Loop through all existing rows
  for (int rowIndex = 0; rowIndex < rowCount(index(groupIndex, 0));
       ++rowIndex) {
    // Return true if we find any match
    if (rowMatches(groupIndex, rowIndex, rowValues, false)) {
      result = rowIndex;
      return result;
    }
  }

  return result;
}

/**
Inserts a new row with given values to the specified group in the specified
location
@param groupIndex :: The index to insert the new row after
@param rowIndex :: The index to insert the new row after
@param rowValues :: the values to set in the row cells
*/
void QTwoLevelTreeModel::insertRowWithValues(
    int groupIndex, int rowIndex, const std::map<QString, QString> &rowValues) {

  // Add the row into the table
  insertRow(rowIndex, index(groupIndex, 0));

  // Loop through all the cells and update the values
  int colIndex = 0;
  for (auto const &columnName : m_whitelist.names()) {
    if (rowValues.count(columnName)) {
      const auto value = rowValues.at(columnName).toStdString();
      const auto absolutePosition =
          m_groups[groupIndex].rowAbsoluteIndex(rowIndex);
      m_tWS->String(absolutePosition, colIndex + 1) = value;
    }
    ++colIndex;
  }

  // Update cached data from the table
  updateAllGroupData();
}

/** Find the position in a group to insert a row with given values. Maintains
 * sorting within the group by key columns or, if there are no key columns,
 * inserts
 * at the end of the group.
 * @param groupIndex : the group to insert into
 * @param rowValues : the row values as a map of column name to value
 */
int QTwoLevelTreeModel::getPositionToInsertRowInGroup(
    const int groupIndex, const std::map<QString, QString> &rowValues) {

  auto numberOfRowsInGroup = rowCount(index(groupIndex, 0));
  auto group = m_groups[groupIndex];

  for (int rowIndex = 0; rowIndex < numberOfRowsInGroup; ++rowIndex) {
    int columnIndex = 0;
    for (auto columnIt = m_whitelist.begin(); columnIt != m_whitelist.end();
         ++columnIt, ++columnIndex) {
      const auto column = *columnIt;

      // Find the first key column where we have a search value
      if (!column.isKey() || !rowValues.count(column.name()))
        continue;

      auto searchValue = rowValues.at(column.name()).toStdString();
      auto compareValue = cellValue(groupIndex, rowIndex, columnIndex);

      // If the row value is greater than the search value, we'll insert the
      // new row before it
      if (compareValue > searchValue) {
        return rowIndex;
      }

      // Insert at the end of the group
      return numberOfRowsInGroup;
    }
  }

  // If no values were found to compare, insert at the end of the group
  return numberOfRowsInGroup;
}

void QTwoLevelTreeModel::insertRowAndGroupWithValues(
    const std::map<QString, QString> &rowValues) {

  // Get the group index. Create the groups if it doesn't exist
  const auto groupName = rowValues.at("Group").toStdString();
  auto groupIndex = findOrAddGroup(groupName);

  // Find the row index to update. First, check if the row already exists in
  // the group
  auto existingRowIndex = findRowIndex(groupIndex, rowValues);
  int rowIndex = 0;
  if (existingRowIndex) {
    // We'll update the existing row
    rowIndex = existingRowIndex.get();

    // If it is identical to the new values then there is nothing to do
    if (rowMatches(groupIndex, rowIndex, rowValues, true))
      return;

    // Otherwise, we want to reset the row to the new values. Just delete the
    // existing row and then continue below to add the new row.
    removeRows(rowIndex, 1, groupIndex);

    // The group may have been removed it if was left empty; if so, re-add it
    groupIndex = findOrAddGroup(groupName);
  } else {
    // We'll add a new row to the end of the group
    rowIndex = getPositionToInsertRowInGroup(groupIndex, rowValues);
  }

  insertRowWithValues(groupIndex, rowIndex, rowValues);
}

/** Transfer data to the model
 * @param runs :: [input] Data to transfer as a vector of maps
 */
void QTwoLevelTreeModel::transfer(
    const std::vector<std::map<QString, QString>> &runs) {
  // If the table only has one row, check if it is empty and if so, remove it.
  // This is to make things nicer when transferring, as the default table has
  // one empty row
  if (rowCount() == 1 && rowCount(index(0, 0)) == 1 && rowIsEmpty(0, 0))
    removeRows(0, 1);

  for (const auto &rowValues : runs) {
    // The first cell in the row contains the group name. It must be set.
    // (Potentially we could allow it to be empty but it's probably safer to
    // enforce this.)
    if (!rowValues.count("Group") || rowValues.at("Group").isEmpty()) {
      throw std::invalid_argument("Data cannot be transferred to the "
                                  "processing table. Group information is "
                                  "missing.");
    }

    insertRowAndGroupWithValues(rowValues);
  }
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
