#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include <QPainter>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {

CellDelegate::CellDelegate(QObject *parent, QTreeView const &view)
    : QStyledItemDelegate(parent), m_view(view) {}

void CellDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const {
  QStyledItemDelegate::paint(painter, option, index);
  painter->save();
  auto pen =
      (m_view.currentIndex() == index) ? QPen(Qt::black) : QPen(Qt::darkGray);
  pen.setWidth((m_view.currentIndex() == index) ? 2 : 1);
  painter->setPen(pen);
  painter->drawRect(option.rect.adjusted(1, 1, -1, -1));
  painter->restore();
}

}
}
}
