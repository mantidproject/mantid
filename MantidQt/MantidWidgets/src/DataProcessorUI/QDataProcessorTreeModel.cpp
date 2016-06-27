#include "MantidQtMantidWidgets/DataProcessorUI/QDataProcessorTreeModel.h"
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
QDataProcessorTreeModel::QDataProcessorTreeModel(
    ITableWorkspace_sptr tableWorkspace,
    const DataProcessorWhiteList &whitelist)
    : m_tWS(tableWorkspace), m_whitelist(whitelist) {

  setupModelData(tableWorkspace, whitelist);
}

QDataProcessorTreeModel::~QDataProcessorTreeModel() { delete m_rootItem; }

int QDataProcessorTreeModel::columnCount(
    const QModelIndex & /* parent */) const {
  return m_rootItem->columnCount();
}

QVariant QDataProcessorTreeModel::data(const QModelIndex &index,
                                       int role) const {
  if (!index.isValid())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  QDataProcessorTreeItem *item = getItem(index);

  return item->data(index.column());
}

Qt::ItemFlags QDataProcessorTreeModel::flags(const QModelIndex &index) const {
  if (!index.isValid())
    return 0;

  return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

QDataProcessorTreeItem *
QDataProcessorTreeModel::getItem(const QModelIndex &index) const {
  if (index.isValid()) {
    QDataProcessorTreeItem *item =
        static_cast<QDataProcessorTreeItem *>(index.internalPointer());
    if (item)
      return item;
  }
  return m_rootItem;
}

QVariant QDataProcessorTreeModel::headerData(int section,
                                             Qt::Orientation orientation,
                                             int role) const {
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    return m_rootItem->data(section);

  return QVariant();
}

QModelIndex QDataProcessorTreeModel::index(int row, int column,
                                           const QModelIndex &parent) const {

  if (parent.isValid() && parent.column() != 0)
    return QModelIndex();

  QDataProcessorTreeItem *parentItem = getItem(parent);

  QDataProcessorTreeItem *childItem = parentItem->child(row);
  if (childItem)
    return createIndex(row, column, childItem);
  else
    return QModelIndex();
}

bool QDataProcessorTreeModel::insertRows(int position, int rows,
                                         const QModelIndex &parent) {
  QDataProcessorTreeItem *parentItem = getItem(parent);
  bool success;

  beginInsertRows(parent, position, position + rows - 1);
  success =
      parentItem->insertChildren(position, rows, m_rootItem->columnCount());
  endInsertRows();

  return success;
}

QModelIndex QDataProcessorTreeModel::parent(const QModelIndex &index) const {
  if (!index.isValid())
    return QModelIndex();

  QDataProcessorTreeItem *childItem = getItem(index);
  QDataProcessorTreeItem *parentItem = childItem->parent();

  if (parentItem == m_rootItem)
    return QModelIndex();

  return createIndex(parentItem->childNumber(), 0, parentItem);
}

bool QDataProcessorTreeModel::removeRows(int position, int rows,
                                         const QModelIndex &parent) {
  QDataProcessorTreeItem *parentItem = getItem(parent);
  bool success = true;

  beginRemoveRows(parent, position, position + rows - 1);
  success = parentItem->removeChildren(position, rows);
  endRemoveRows();

  return success;
}

int QDataProcessorTreeModel::rowCount(const QModelIndex &parent) const {
  QDataProcessorTreeItem *parentItem = getItem(parent);

  return parentItem->childCount();
}

bool QDataProcessorTreeModel::setData(const QModelIndex &index,
                                      const QVariant &value, int role) {
  if (role != Qt::EditRole)
    return false;

  QDataProcessorTreeItem *item = getItem(index);
  bool result = item->setData(index.column(), value);

  if (result)
    emit dataChanged(index, index);

  return result;
}

void QDataProcessorTreeModel::setupModelData(
    ITableWorkspace_sptr table, const DataProcessorWhiteList &whitelist) {

  QVector<QVariant> header;
  for (int i = 0; i < m_whitelist.size(); i++)
    header << QString::fromStdString(m_whitelist.colNameFromColIndex(i));

  m_rootItem = new QDataProcessorTreeItem(header);

  int nrows = static_cast<int>(table->rowCount());
  int ncols = static_cast<int>(table->columnCount() - 1);

  // Insert the groups

  int lastID = 0;
  std::map<std::string, int> groupID;
  for (int r = 0; r < nrows; r++) {

    const std::string &groupName = m_tWS->String(r, 0);

    if (!groupID.count(groupName)) {

      groupID[groupName] = lastID++;

      m_rootItem->insertChildren(m_rootItem->childCount(), 1, 1);

      m_rootItem->child(m_rootItem->childCount() - 1)
          ->setData(0, QString::fromStdString(groupName));
    }
  }

  // Now insert the data

  for (int r = 0; r < nrows; r++) {

    const std::string groupName = m_tWS->String(r, 0);
    int id = groupID[groupName];

    auto groupItem = m_rootItem->child(id);

    id = groupItem->childCount();

    groupItem->insertChildren(id, 1, ncols);

    auto childItem = groupItem->child(id);

    for (int c = 0; c < ncols; c++) {
      const std::string value = m_tWS->String(r, c + 1);
      childItem->setData(c, QString::fromStdString(value));
    }
  }
}

} // namespace MantidWidgets
} // namespace Mantid
