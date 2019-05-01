// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/DataProcessorUI/QOneLevelTreeModel.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidQtWidgets/Common/DataProcessorUI/TreeData.h"

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {
using namespace Mantid::API;

//----------------------------------------------------------------------------------------------
/** Constructor
@param tableWorkspace : The table workspace to wrap
@param whitelist : A WhiteList containing the columns
*/
QOneLevelTreeModel::QOneLevelTreeModel(ITableWorkspace_sptr tableWorkspace,
                                       const WhiteList &whitelist)
    : AbstractTreeModel(tableWorkspace, whitelist) {

  if (tableWorkspace->columnCount() != m_whitelist.size())
    throw std::invalid_argument(
        "Invalid table workspace. Table workspace must "
        "have the same number of columns as the white list");

  // Create vector for caching row data
  for (size_t i = 0; i < tableWorkspace->rowCount(); ++i)
    m_rows.emplace_back(std::make_shared<RowData>(columnCount()));

  // Update cached row data from the table
  updateAllRowData();

  // This ensures the cached row data is updated when the table changes
  connect(this, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
          this,
          SLOT(tableDataUpdated(const QModelIndex &, const QModelIndex &)));
}

QOneLevelTreeModel::~QOneLevelTreeModel() {}

/** Returns data for specified index
 * @param index : The index
 * @param role : The role
 * @return : The data associated with the given index as a list of strings
 */
QVariant QOneLevelTreeModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (parent(index).isValid())
    return QVariant();

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    return QString::fromStdString(m_tWS->String(index.row(), index.column()));
  } else if (role == Qt::BackgroundRole) {
    // Highlight if the process status for this row is set (red if failed,
    // green if succeeded)
    const auto rowData = m_rows.at(index.row());
    if (rowData->isProcessed()) {
      if (rowData->reductionFailed())
        return QColor(Colour::FAILED);
      else
        return QColor(Colour::SUCCESS);
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
QVariant QOneLevelTreeModel::headerData(int section,
                                        Qt::Orientation orientation,
                                        int role) const {

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return m_whitelist.name(section);

  return QVariant();
}

/** Returns row data struct (which includes metadata about the row)
 * for specified index
 * @param index : The index
 * @return : The data associated with the given index as a RowData class
 */
RowData_sptr QOneLevelTreeModel::rowData(const QModelIndex &index) const {
  RowData_sptr result;

  // Return a null ptr if the index is invalid
  if (!index.isValid())
    return result;

  if (parent(index).isValid())
    return result;

  return m_rows.at(index.row());
}

/** Returns the index of an element specified by its row, column and parent
 * @param row : The row
 * @param column : The column
 * @param parent : The parent element
 * @return : The index of the element
 */
QModelIndex QOneLevelTreeModel::index(int row, int column,
                                      const QModelIndex &parent) const {

  UNUSED_ARG(parent);
  return createIndex(row, column);
}

/** Gets the 'processed' status of a row
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : The 'processed' status
 */
bool QOneLevelTreeModel::isProcessed(int position,
                                     const QModelIndex &parent) const {

  // No parent items exists, this should not be possible
  if (parent.isValid())
    throw std::invalid_argument(
        "Invalid parent index, there are no parent data items in this model.");

  // Incorrect position
  if (position < 0 || position >= rowCount())
    throw std::invalid_argument("Invalid position. Position index must be "
                                "within the range of the number of rows in "
                                "this model");

  return m_rows[position]->isProcessed();
}

/** Check whether reduction failed for a row
 * @param position : The position of the item
 * @param parent : The parent of this item
 * @return : true if there was an error
 */
bool QOneLevelTreeModel::reductionFailed(int position,
                                         const QModelIndex &parent) const {

  // No parent items exists, this should not be possible
  if (parent.isValid())
    throw std::invalid_argument(
        "Invalid parent index, there are no parent data items in this model.");

  // Incorrect position
  if (position < 0 || position >= rowCount())
    throw std::invalid_argument("Invalid position. Position index must be "
                                "within the range of the number of rows in "
                                "this model");

  return m_rows[position]->reductionFailed();
}

/** Returns the parent of a given index
 * @param index : The index
 * @return : Its parent
 */
QModelIndex QOneLevelTreeModel::parent(const QModelIndex &index) const {

  UNUSED_ARG(index);
  return QModelIndex();
}

/** Adds elements to the tree
 * @param position : The position where to insert the new elements
 * @param count : The number of elements to insert
 * @param parent : The parent of the set of elements
 * @return : Boolean indicating whether the insertion was successful or not
 */
bool QOneLevelTreeModel::insertRows(int position, int count,
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

  // Update the table workspace and row process status vector
  for (int pos = position; pos < position + count; pos++) {
    m_tWS->insertRow(position);
    m_rows.insert(m_rows.begin() + position,
                  std::make_shared<RowData>(columnCount()));
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
bool QOneLevelTreeModel::removeRows(int position, int count,
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

  // Update the table workspace and row process status vector
  for (int pos = position; pos < position + count; pos++) {
    m_tWS->removeRow(position);
    m_rows.erase(m_rows.begin() + position);
  }

  endRemoveRows();

  return true;
}

/** Remove all rows from the tree
 * @return : Boolean indicating whether or not rows were removed
 */
bool QOneLevelTreeModel::removeAll() {
  beginRemoveRows(QModelIndex(), 0, rowCount() - 1);

  for (int pos = 0; pos < rowCount(); ++pos) {
    m_tWS->removeRow(0);
    m_rows.erase(m_rows.begin());
  }

  endRemoveRows();

  return true;
}

/** Returns the number of rows of a given parent
 * @param parent : The parent item
 * @return : The number of rows
 */
int QOneLevelTreeModel::rowCount(const QModelIndex &parent) const {

  if (parent.isValid())
    return 0;

  return static_cast<int>(m_tWS->rowCount());
}

/** Updates an index with given data
 * @param index : the index
 * @param value : the new value
 * @param role : the role
 */
bool QOneLevelTreeModel::setData(const QModelIndex &index,
                                 const QVariant &value, int role) {

  if (role != Qt::EditRole)
    return false;

  if (!index.isValid())
    return false;

  const std::string valueStr = value.toString().toStdString();
  if (m_tWS->String(index.row(), index.column()) == valueStr)
    return false;

  m_tWS->String(index.row(), index.column()) = valueStr;

  emit dataChanged(index, index);

  return true;
}

/** Sets the 'processed' status of a row
 * @param processed : True to set processed, false to set unprocessed
 * @param position : The position of the row to be set
 * @param parent : The parent of this row
 * @return : Boolean indicating whether process status was set successfully
 */
bool QOneLevelTreeModel::setProcessed(bool processed, int position,
                                      const QModelIndex &parent) {

  // No parent items exists, this should not be possible
  if (parent.isValid())
    return false;

  // Incorrect position
  if (position < 0 || position >= rowCount())
    return false;

  m_rows[position]->setProcessed(processed);

  return true;
}

/** Set the error message for a row
 * @param error : the error message
 * @param position : The position of the row to be set
 * @param parent : The parent of this row
 * @return : Boolean indicating whether error was set successfully
 */
bool QOneLevelTreeModel::setError(const std::string &error, int position,
                                  const QModelIndex &parent) {

  // No parent items exists, this should not be possible
  if (parent.isValid())
    return false;

  // Incorrect position
  if (position < 0 || position >= rowCount())
    return false;

  m_rows[position]->setError(error);

  return true;
}

/** Return the underlying data structure, i.e. the table workspace this model is
 * representing
 *
 * @return :: the underlying table workspace
 */
ITableWorkspace_sptr QOneLevelTreeModel::getTableWorkspace() const {
  return m_tWS;
}

/** Update all cached row data from the table data
 */
void QOneLevelTreeModel::updateAllRowData() {
  // Loop through all rows
  for (int row = 0; row < rowCount(); ++row) {
    auto rowData = m_rows[row];
    // Loop through all columns and update the value in the row data
    for (int col = 0; col < columnCount(); ++col) {
      auto value = data(index(row, col)).toString();
      rowData->setValue(col, value);
    }
  }
}

/** Called when the data in the table has changed. Updates the
 * table values in the cached RowData
 */
void QOneLevelTreeModel::tableDataUpdated(const QModelIndex & /*unused*/,
                                          const QModelIndex & /*unused*/) {
  updateAllRowData();
}

/** Checks whether the existing row is empty
 * @return : true if all of the values in the row are empty
 */
bool QOneLevelTreeModel::rowIsEmpty(int row) const {
  // Loop through all columns and return false if any are not empty
  for (int columnIndex = 0; columnIndex < columnCount(); ++columnIndex) {
    auto value = data(index(row, columnIndex)).toString().toStdString();
    if (!value.empty())
      return false;
  }

  // All cells in the row were empty
  return true;
}

/**
Inserts a new row with given values to the specified group in the specified
location
@param rowIndex :: The index to insert the new row after
@param rowValues :: the values to set in the row cells
*/
void QOneLevelTreeModel::insertRowWithValues(
    int rowIndex, const std::map<QString, QString> &rowValues) {

  insertRow(rowIndex);

  // Loop through all columns and update the value in the row
  int colIndex = 0;
  for (auto const &columnName : m_whitelist.names()) {
    if (rowValues.count(columnName)) {
      const auto value = rowValues.at(columnName).toStdString();
      m_tWS->String(rowIndex, colIndex) = value;
    }
    ++colIndex;
  }

  updateAllRowData();
}

/** Transfer data to the table
 * @param runs :: [input] Data to transfer as a vector of maps
 */
void QOneLevelTreeModel::transfer(
    const std::vector<std::map<QString, QString>> &runs) {

  // If the table only has one row, check if it is empty and if so, remove it.
  // This is to make things nicer when transferring, as the default table has
  // one empty row
  if (rowCount() == 1 && rowIsEmpty(0))
    removeRows(0, 1);

  // Loop over the rows (vector elements)
  for (const auto &row : runs) {
    // Add a new row to the model at the end of the current list
    const int rowIndex = rowCount();
    insertRowWithValues(rowIndex, row);
  }
}
} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt
