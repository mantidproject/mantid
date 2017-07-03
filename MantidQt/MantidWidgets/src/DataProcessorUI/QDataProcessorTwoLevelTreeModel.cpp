#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTwoLevelTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
@param tableWorkspace : The table workspace to wrap
@param whitelist : A DataProcessorWhiteList containing information about the
columns, their indices and descriptions
*/
QDataProcessorTwoLevelTreeModel::QDataProcessorTwoLevelTreeModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist) {

  if (tableWorkspace->columnCount() != m_whitelist.size() + 1)
    throw std::invalid_argument("Invalid table workspace. Table workspace must "
                                "have one extra column accounting for groups");

  // Sort the table workspace by group, i.e. first column
  std::vector<std::pair<std::string, bool>> criteria = {
      std::make_pair(tableWorkspace->getColumnNames().at(0), true)};
  m_tWS->sort(criteria);

  setupModelData(tableWorkspace);
}

QDataProcessorTwoLevelTreeModel::~QDataProcessorTwoLevelTreeModel() {}

/** Returns the number of columns, i.e. elements in the whitelist
* @return : The number of columns
*/
int QDataProcessorTwoLevelTreeModel::columnCount(
    const QModelIndex & /* parent */) const {
  return static_cast<int>(m_whitelist.size());
}

/** Returns data for specified index
* @param index : The index
* @param role : The role
* @return : The data associated with the given index
*/
QVariant QDataProcessorTwoLevelTreeModel::data(const QModelIndex &index,
                                               int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  if (!parent(index).isValid()) {
    // Index corresponds to a group

    if (index.column() == 0) {
      // Return the group name only in the first column
      return QString::fromStdString(m_groupName.at(index.row()));
    } else {
      return QVariant();
    }
  } else {
    // Index corresponds to a row

    // Absolute position of this row in the table
    int absolutePosition = m_rowsOfGroup[parent(index).row()][index.row()];
    return QString::fromStdString(
        m_tWS->String(absolutePosition, index.column() + 1));
  }
}

Qt::ItemFlags
QDataProcessorTwoLevelTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

/** Returns the column name (header data for given section)
* @param section : The section (column) index
* @param orientation : The orientation
* @param role : The role
* @return : The column name
*/
QVariant QDataProcessorTwoLevelTreeModel::headerData(
    int section, Qt::Orientation orientation, int role) const {

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return QString::fromStdString(m_whitelist.colNameFromColIndex(section));

  if (orientation == Qt::Horizontal && role == Qt::WhatsThisRole)
    return QString::fromStdString(m_whitelist.description(section));

  return QVariant();
}

/** Returns the index of an element specified by its row, column and parent
* @param row : The row
* @param column : The column
* @param parent : The parent element
* @return : The index of the element
*/
QModelIndex
QDataProcessorTwoLevelTreeModel::index(int row, int column,
                                       const QModelIndex &parent) const {

  return parent.isValid() ? createIndex(row, column, parent.row())
                          : createIndex(row, column, -1);
}

/** Returns the parent of a given index
* @param index : The index
* @return : Its parent
*/
QModelIndex
QDataProcessorTwoLevelTreeModel::parent(const QModelIndex &index) const {

  int internalIdInt = int(index.internalId());

  return internalIdInt >= 0 ? createIndex(internalIdInt, 0, -1) : QModelIndex();
}

/** Adds elements to the tree
* @param position : The position where to insert the new elements
* @param count : The number of elements to insert
* @param parent : The parent of the set of elements
* @return : Boolean indicating whether the insertion was successful or not
*/
bool QDataProcessorTwoLevelTreeModel::insertRows(int position, int count,
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
bool QDataProcessorTwoLevelTreeModel::insertRows(int position, int count,
                                                 int parent) {

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

  int absolutePosition =
      (m_rowsOfGroup[parent].size() > 0)
          ? ((position == static_cast<int>(m_rowsOfGroup[parent].size()))
                 ? m_rowsOfGroup[parent][position - 1] + 1
                 : m_rowsOfGroup[parent][position])
          : (parent > 0) ? m_rowsOfGroup[parent - 1].back() + 1 : 0;

  for (int pos = position; pos < position + count; pos++) {
    m_tWS->insertRow(absolutePosition);
    m_tWS->String(absolutePosition, 0) = m_groupName[parent];
  }

  int lastRowIndex = absolutePosition;

  // Insert new elements
  m_rowsOfGroup[parent].insert(m_rowsOfGroup[parent].begin() + position, count,
                               0);

  // Update row indices in the group where we are adding the new rows
  for (int pos = position; pos < rowCount(index(parent, 0)); pos++)
    m_rowsOfGroup[parent][pos] = lastRowIndex++;

  // Update row indices in subsequent groups
  for (int group = parent + 1; group < rowCount(); group++) {
    for (int row = 0; row < rowCount(index(group, 0)); row++)
      m_rowsOfGroup[group][row] = lastRowIndex++;
  }

  endInsertRows();

  return true;
}

/** Insert new groups at a given position
* @param position : The position where new groups will be inserted
* @param count : The number of groups to insert
* @return : True if insertion was successful, false otherwise
*/
bool QDataProcessorTwoLevelTreeModel::insertGroups(int position, int count) {

  // Invalid position
  if (position < 0 || position > rowCount())
    return false;

  // Invalid number of groups
  if (count < 1)
    return false;

  beginInsertRows(QModelIndex(), position, position + count - 1);

  // Update m_rowsOfGroup
  m_rowsOfGroup.insert(m_rowsOfGroup.begin() + position, count,
                       std::vector<int>());
  m_groupName.insert(m_groupName.begin() + position, count, "");

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
* @return : Boolean indicating whether the elements were removed successfully or
* not
*/
bool QDataProcessorTwoLevelTreeModel::removeRows(int position, int count,
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
bool QDataProcessorTwoLevelTreeModel::removeGroups(int position, int count) {

  // Invalid position
  if (position < 0 || position >= rowCount())
    return false;

  // Invalid number of groups
  if (count < 1 || position + count > rowCount())
    return false;

  beginRemoveRows(QModelIndex(), position, position + count - 1);

  // Update group names
  m_groupName.erase(m_groupName.begin() + position,
                    m_groupName.begin() + position + count);

  // Update table workspace

  int absolutePosition =
      (m_rowsOfGroup[position].size() > 0)
          ? m_rowsOfGroup[position][0]
          : (position > 0) ? m_rowsOfGroup[position - 1].back() + 1 : 0;

  for (int group = position; group < position + count; group++) {
    for (int row = 0; row < rowCount(index(group, 0)); row++)
      m_tWS->removeRow(absolutePosition);
  }

  m_rowsOfGroup.erase(m_rowsOfGroup.begin() + position,
                      m_rowsOfGroup.begin() + position + count);

  if (m_groupName.size() > 0) {
    // Update row positions
    for (int group = position; group < rowCount(); group++) {
      for (int row = 0; row < rowCount(index(group, 0)); row++)
        m_rowsOfGroup[group][row] = absolutePosition++;
    }
  } else
    m_rowsOfGroup.clear();

  endRemoveRows();

  return true;
}

/** Removes rows from a group
* @param position : The position of the first row that will be removed
* @param count : The number of rows to remove
* @param parent : The parent item
* @return : Boolean indicating whether or not rows were removed
*/
bool QDataProcessorTwoLevelTreeModel::removeRows(int position, int count,
                                                 int parent) {

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
  int absolutePosition = m_rowsOfGroup[parent][position];
  for (int pos = position; pos < position + count; pos++) {
    m_tWS->removeRow(absolutePosition);
  }

  m_rowsOfGroup[parent].erase(m_rowsOfGroup[parent].begin() + position,
                              m_rowsOfGroup[parent].begin() + position + count);

  // Update row indices in this group
  for (int row = position; row < rowCount(index(parent, 0)); row++) {
    m_rowsOfGroup[parent][row] = absolutePosition++;
  }

  // Update row indices in subsequent groups
  for (int group = parent + 1; group < rowCount(); group++)
    for (int row = 0; row < rowCount(index(group, 0)); row++)
      m_rowsOfGroup[group][row] = absolutePosition++;

  if (m_rowsOfGroup[parent].size() == 0) {
    removeGroups(parent, 1);
  }

  endRemoveRows();

  return true;
}

/** Returns the number of rows of a given parent
* @param parent : The parent item
* @return : The number of rows
*/
int QDataProcessorTwoLevelTreeModel::rowCount(const QModelIndex &parent) const {

  // We are counting the number of groups
  if (!parent.isValid())
    return static_cast<int>(m_rowsOfGroup.size());

  // This shouldn't happen
  if (parent.parent().isValid())
    return 0;

  // This group does not exist anymore
  if (parent.row() >= static_cast<int>(m_rowsOfGroup.size()))
    return 0;

  // Group exists, return number of children
  return static_cast<int>(m_rowsOfGroup[parent.row()].size());
}

/** Updates an index with given data
* @param index : the index
* @param value : the new value
* @param role : the role
*/
bool QDataProcessorTwoLevelTreeModel::setData(const QModelIndex &index,
                                              const QVariant &value, int role) {

  if (role != Qt::EditRole)
    return false;

  if (!parent(index).isValid()) {
    // Index corresponds to a group

    if (index.column() == 0) {

      const std::string newName = value.toString().toStdString();

      // Update the group name, which means updating:

      // 1. Axiliary member variables

      m_groupName[index.row()] = newName;

      // 2. Table workspace

      size_t nrows = m_rowsOfGroup[index.row()].size();
      for (size_t row = 0; row < nrows; row++) {
        m_tWS->String(m_rowsOfGroup[index.row()][row], 0) = newName;
      }

    } else {
      return false;
    }
  } else {
    // Index corresponds to a row

    // First we need to find the absolute position of this row in the table
    int absolutePosition = m_rowsOfGroup[parent(index).row()][index.row()];
    m_tWS->String(absolutePosition, index.column() + 1) =
        value.toString().toStdString();
  }

  emit dataChanged(index, index);

  return true;
}

/** Setup the data, initialize member variables using a table workspace and
* whitelist
* @param table : A table workspace containing the data
*/
void QDataProcessorTwoLevelTreeModel::setupModelData(
    ITableWorkspace_sptr table) {

  int nrows = static_cast<int>(table->rowCount());

  int lastIndex = 0;
  std::map<std::string, int> groupIndex;

  for (int r = 0; r < nrows; r++) {

    const std::string &groupName = m_tWS->String(r, 0);

    if (groupIndex.count(groupName) == 0) {
      groupIndex[groupName] = lastIndex++;

      m_groupName.push_back(groupName);
      m_rowsOfGroup.push_back(std::vector<int>());
    }

    m_rowsOfGroup[groupIndex[groupName]].push_back(r);
  }
}

/** Return the underlying data structure, i.e. the table workspace this model is
 * representing
 *
 * @return :: the underlying table workspace
 */
ITableWorkspace_sptr
QDataProcessorTwoLevelTreeModel::getTableWorkspace() const {
  return m_tWS;
}

} // namespace MantidWidgets
} // namespace Mantid
