#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTableModel.h"
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
QDataProcessorTableModel::QDataProcessorTableModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_dataCachePeakIndex(-1), m_tWS(tableWorkspace), m_whitelist(whitelist) {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QDataProcessorTableModel::~QDataProcessorTableModel() {}

/**
Invalidate the cache for a row
@param row : the row the cache needs to be invalidated for
*/
void QDataProcessorTableModel::invalidateDataCache(const int row) const {
  // If the row is in the cache, invalidate the cache.
  if (row == m_dataCachePeakIndex || row == -1)
    m_dataCachePeakIndex = -1;
}

/**
Load data into the cache if required
@param row : to check and load if required
*/
void QDataProcessorTableModel::updateDataCache(const int row) const {
  // if the index is what is already cached just return
  if (row == m_dataCachePeakIndex)
    return;

  TableRow tableRow = m_tWS->getRow(row);

  // generate the cache
  m_dataCache.clear();

  int ncols = static_cast<int>(m_whitelist.size());

  for (int col = 0; col < ncols - 2; col++) {

    m_dataCache.push_back(
        QString::fromStdString(tableRow.cell<std::string>(col)));
  }

  // Column 'Group'
  m_dataCache.push_back(QString::number(tableRow.cell<int>(ncols - 2)));
  // Column 'Options'
  m_dataCache.push_back(
      QString::fromStdString(tableRow.cell<std::string>(ncols - 1)));

  m_dataCachePeakIndex = row;
}

/**
Update the model.
*/
void QDataProcessorTableModel::update() {
  emit layoutChanged(); // This should tell the view that the data has changed.
}

/**
@return the row count.
*/
int QDataProcessorTableModel::rowCount(const QModelIndex &) const {
  return static_cast<int>(m_tWS->rowCount());
}

/**
@return the number of columns in the model.
*/
int QDataProcessorTableModel::columnCount(const QModelIndex &) const {
  return static_cast<int>(m_whitelist.size());
}

/**
Find the column name at a given column index.
@param colIndex : Index to find column name for.
*/
QString QDataProcessorTableModel::findColumnName(const int colIndex) const {

  std::string colName = m_whitelist.colNameFromColIndex(colIndex);
  return QString::fromStdString(colName);
}

/**
Overrident data method, allows consuming view to extract data for an index and
role.
@param index : For which to extract the data
@param role : Role mode
*/
QVariant QDataProcessorTableModel::data(const QModelIndex &index,
                                        int role) const {

  int ncols = static_cast<int>(m_whitelist.size());

  if (role == Qt::TextAlignmentRole) {
    if (index.column() == ncols - 1)
      return Qt::AlignLeft;
    else
      return Qt::AlignRight;
  } else if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return QVariant();
  }
  const int colNumber = index.column();
  const int rowNumber = index.row();

  this->updateDataCache(rowNumber);
  return m_dataCache[colNumber];
}

/**
Overrident setData method, allows view to set data for an index and role.
@param index : For which to extract the data
@param value : New value
@param role : Role mode
@returns booean true if sucessful, false if unsucessful.
*/
bool QDataProcessorTableModel::setData(const QModelIndex &index,
                                       const QVariant &value, int role) {
  // Users may mistakenly enter whitespace. Let's strip it for them.
  QString str = value.toString().trimmed();

  if (index.isValid() && role == Qt::EditRole) {
    const int colNumber = index.column();
    const int rowNumber = index.row();

    const int ncols = static_cast<int>(m_whitelist.size());

    if (colNumber == ncols - 2) {

      m_tWS->Int(rowNumber, colNumber) = str.toInt();
    } else {

      m_tWS->String(rowNumber, colNumber) = str.toStdString();
    }

    invalidateDataCache(rowNumber);
    emit dataChanged(index, index);

    return true;
  }
  return false;
}

/**
Get the heading for a given section, orientation and role.
@param section : Column index
@param orientation : Heading orientation
@param role : Role mode of table.
@return HeaderData.
*/
QVariant QDataProcessorTableModel::headerData(int section,
                                              Qt::Orientation orientation,
                                              int role) const {
  if (role == Qt::WhatsThisRole && orientation == Qt::Horizontal) {
    return QString::fromStdString(m_whitelist.description(section));

  } else if (role == Qt::DisplayRole) {
    if (orientation == Qt::Horizontal)
      return findColumnName(section);
    if (orientation == Qt::Vertical)
      return QString::number(section + 1);
  }

  return QVariant();
}

/**
Provide flags on an index by index basis
@param index: To generate a flag for.
*/
Qt::ItemFlags QDataProcessorTableModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

/**
Insert the given number of rows at the specified position
@param row : The row to insert before
@param count : The number of rows to insert
@param parent : The parent index
*/
bool QDataProcessorTableModel::insertRows(int row, int count,
                                          const QModelIndex &parent) {
  if (count < 1)
    return true;

  if (row < 0)
    return false;

  beginInsertRows(parent, row, row + count - 1);
  for (int i = 0; i < count; ++i)
    m_tWS->insertRow(row + i);
  endInsertRows();

  invalidateDataCache(-1);
  return true;
}

/**
Remove the given number of rows from the specified position
@param row : The row index to remove from
@param count : The number of rows to remove
@param parent : The parent index
*/
bool QDataProcessorTableModel::removeRows(int row, int count,
                                          const QModelIndex &parent) {
  if (count < 1)
    return true;

  if (row < 0)
    return false;

  beginRemoveRows(parent, row, row + count - 1);
  for (int i = 0; i < count; ++i)
    m_tWS->removeRow(row);
  endRemoveRows();

  invalidateDataCache(-1);
  return true;
}

} // namespace MantidWidgets
} // namespace Mantid
