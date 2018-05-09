#ifndef MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#define MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#include <QStyledItemDelegate>
#include <QTreeView>
#include <QStandardItemModel>
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class CellDelegate : public QStyledItemDelegate {
public:
  explicit CellDelegate(QObject *parent, QTreeView const &view,
                        FilteredTreeModel const &filterModel,
                        QStandardItemModel const &mainModel);
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private:
  QTreeView const &m_view;
  FilteredTreeModel const &m_filteredModel;
  QStandardItemModel const &m_mainModel;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
