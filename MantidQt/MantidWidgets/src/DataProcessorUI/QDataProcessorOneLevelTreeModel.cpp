#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorOneLevelTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"

namespace MantidQt {
namespace MantidWidgets {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
@param tableWorkspace : The table workspace to wrap
@param whitelist : A DataProcessorWhiteList containing the columns
*/
QDataProcessorOneLevelTreeModel::QDataProcessorOneLevelTreeModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist) {

  if (tableWorkspace->columnCount() != m_whitelist.size())
    throw std::invalid_argument(
        "Invalid table workspace. Table workspace must "
        "have the same number of columns as the white list");
}

QDataProcessorOneLevelTreeModel::~QDataProcessorOneLevelTreeModel() {}

/** Returns the number of columns, i.e. elements in the whitelist
* @return : The number of columns
*/
int QDataProcessorOneLevelTreeModel::columnCount(
    const QModelIndex & /* parent */) const {
  return static_cast<int>(m_whitelist.size());
}

/** Returns data for specified index
* @param index : The index
* @param role : The role
* @return : The data associated with the given index
*/
QVariant QDataProcessorOneLevelTreeModel::data(const QModelIndex &index,
                                               int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  if (parent(index).isValid())
    return QVariant();

  return QString::fromStdString(m_tWS->String(index.row(), index.column()));
}

Qt::ItemFlags QDataProcessorOneLevelTreeModel::flags(const QModelIndex &index) const {
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
QVariant QDataProcessorOneLevelTreeModel::headerData(int section,
                                             Qt::Orientation orientation,
                                             int role) const {

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return QString::fromStdString(m_whitelist.colNameFromColIndex(section));

  return QVariant();
}

/** Returns the index of an element specified by its row, column and parent
* @param row : The row
* @param column : The column
* @param parent : The parent element
* @return : The index of the element
*/
QModelIndex
QDataProcessorOneLevelTreeModel::index(int row, int column,
                                       const QModelIndex &parent) const {

  UNUSED_ARG(parent);
  return createIndex(row, column);
}

/** Returns the parent of a given index
* @param index : The index
* @return : Its parent
*/
QModelIndex
QDataProcessorOneLevelTreeModel::parent(const QModelIndex &index) const {

  UNUSED_ARG(index);
  return QModelIndex();
}

/** Adds elements to the tree
* @param position : The position where to insert the new elements
* @param count : The number of elements to insert
* @param parent : The parent of the set of elements
* @return : Boolean indicating whether the insertion was successful or not
*/
bool QDataProcessorOneLevelTreeModel::insertRows(int position, int count,
                                                 const QModelIndex &parent) {
  if (parent.isValid())
    return false;

  // Incorrect position
  if (position < 0 || position > rowCount())
	  return false;

  // Incorrect number of rows
  if (count < 1)
	  return false;

  beginInsertRows(QModelIndex(), position, position + count - 1);

  // Update the table workspace
  for (int pos = position; pos < position + count; pos++) {
	  m_tWS->insertRow(position);
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
bool QDataProcessorOneLevelTreeModel::removeRows(int position, int count,
                                         const QModelIndex &parent) {

  if (parent.isValid())
	  return false;

  // Incorrect position
  if (position < 0 || position >= rowCount())
	  return false;

  // Incorrect number of rows
  if (count < 1 || position + count > rowCount())
	  return false;

  beginRemoveRows(QModelIndex(), position, position + count - 1);

  // Update the table workspace
  for (int pos = position; pos < position + count; pos++) {
	  m_tWS->removeRow(position);
  }

  endRemoveRows();

  return true;
}

/** Returns the number of rows of a given parent
* @param parent : The parent item
* @return : The number of rows
*/
int QDataProcessorOneLevelTreeModel::rowCount(const QModelIndex &parent) const {

  if (parent.isValid())
    return 0;

  return static_cast<int>(m_tWS->rowCount());
}

/** Updates an index with given data
* @param index : the index
* @param value : the new value
* @param role : the role
*/
bool QDataProcessorOneLevelTreeModel::setData(const QModelIndex &index,
                                              const QVariant &value, int role) {

  if (role != Qt::EditRole)
    return false;

  if (!index.isValid())
    return false;

  const std::string valueStr = value.toString().toStdString();
  m_tWS->String(index.row(), index.column()) = valueStr;

  emit dataChanged(index, index);

  return true;
}

} // namespace MantidWidgets
} // namespace Mantid
