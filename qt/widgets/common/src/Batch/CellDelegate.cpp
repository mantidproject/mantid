#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include <QPainter>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

CellDelegate::CellDelegate(QObject *parent, QTreeView const &view,
                           FilteredTreeModel const &filteredModel,
                           QStandardItemModel const &mainModel)
    : QStyledItemDelegate(parent), m_view(view), m_filteredModel(filteredModel),
      m_mainModel(mainModel) {}

void CellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);
  painter->save();

  auto *item = modelItemFromIndex(
      m_mainModel,
      fromMainModel(m_filteredModel.mapToSource(index), m_mainModel));
  auto borderColor = getBorderColor(*item);
  auto borderThickness = getBorderThickness(*item);

  auto isCurrentCell = m_view.currentIndex() == index;
  auto pen = ([isCurrentCell, &borderColor, item]() -> QPen {
    if (isCurrentCell & item->isEditable())
      return QPen(Qt::black);
    else
      return QPen(borderColor);
  })();
  pen.setWidth(isCurrentCell ? 2 : borderThickness);

  painter->setPen(pen);
  painter->drawRect(option.rect.adjusted(1, 1, -1, -1));
  painter->restore();
}
}
}
}
