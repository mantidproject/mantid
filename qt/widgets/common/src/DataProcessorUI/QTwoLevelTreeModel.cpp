#include "MantidQtWidgets/Common/DataProcessorUI/QTwoLevelTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
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
  }
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
  // Set the row's processed status for the given row index
  void setRowProcessed(const size_t rowIndex, const bool isProcessed) const {
    checkRowIndex(rowIndex);
    m_rows[rowIndex].setProcessed(isProcessed);
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

/** Returns data for specified index
 * @param index : The index
 * @param role : The role
 * @return : The data associated with the given index
 */
QVariant QTwoLevelTreeModel::data(const QModelIndex &index, int role) const {

  if (!index.isValid())
    return QVariant();

  if (!parent(index).isValid()) {
    // Index corresponds to a group
    const auto &group = m_groups.at(index.row());

    if ((role == Qt::DisplayRole || role == Qt::EditRole) &&
        index.column() == 0) {
      // Return the group name only in the first column
      return QString::fromStdString(group.name());
    }
    if (role == Qt::BackgroundRole && group.isProcessed()) {
      // Highlight if this group is processed
      return QColor("#00b300");
    }
  } else {
    // Index corresponds to a row
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
      return QString::fromStdString(m_tWS->String(
          group.rowAbsoluteIndex(index.row()), index.column() + 1));
    }
    if (role == Qt::BackgroundRole && group.isRowProcessed(index.row())) {
      // Highlight if this row is processed
      return QColor("#00b300");
    }
  }

  return QVariant();
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
RowData_sptr QTwoLevelTreeModel::rowData(const QModelIndex &index) {

  RowData_sptr result;

  if (!index.isValid())
    return result;

  if (!parent(index).isValid()) {
    return result;
  } else {
    // Index corresponds to a row
    auto pIndex = parent(index);
    const auto &group = m_groups[pIndex.row()];
    return group.rowData(index.row());
  }

  return result;
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

  if (!parent(index).isValid()) {
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

void QTwoLevelTreeModel::updateAllGroupData() {
  // Loop through all groups and all rows
  for (int groupIdx = 0; groupIdx < rowCount(); ++groupIdx) {
    auto &group = m_groups[groupIdx];
    for (int row = 0; row < rowCount(index(groupIdx, 0)); ++row) {
      const auto &rowData = group.rowData(row);
      // Loop through all columns and update the value in the row data
      for (int col = 0; col < columnCount(); ++col) {
        auto value = data(index(row, col, index(groupIdx, 0))).toString();
        rowData->setValue(col, value);
      }
    }
  }
}

/** Called when the data in the table has changed. Updates the
 * table values in the cached RowData
 */
void QTwoLevelTreeModel::tableDataUpdated(const QModelIndex &,
                                          const QModelIndex &) {
  updateAllGroupData();
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

/**
Inserts a new row with given values to the specified group in the specified
location
@param groupIndex :: The index to insert the new row after
@param rowIndex :: The index to insert the new row after
@param rowValues :: the values to set in the row cells
*/
void QTwoLevelTreeModel::insertRowWithValues(
    int groupIndex, int rowIndex, const std::map<QString, QString> &rowValues) {

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

  updateAllGroupData();
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

  for (const auto &row : runs) {
    // The first cell in the row contains the group name. It must be set.
    // (Potentially we could allow it to be empty but it's probably safer to
    // enforce this.)
    if (!row.count("Group") || row.at("Group").isEmpty()) {
      throw std::invalid_argument("Data cannot be transferred to the "
                                  "processing table. Group information is "
                                  "missing.");
    }
    const auto groupName = row.at("Group").toStdString();
    // Get the group index. Create the groups if it doesn't exist
    const auto groupIndex = findOrAddGroup(groupName);
    // Add a new row with the given values to the end of the group
    const int rowIndex = rowCount(index(groupIndex, 0));
    insertRowWithValues(groupIndex, rowIndex, row);
  }
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
