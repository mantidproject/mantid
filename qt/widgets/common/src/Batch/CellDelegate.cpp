// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Common/Batch/CellDelegate.h"
#include "MantidQtWidgets/Common/Batch/CellStandardItem.h"
#include "MantidQtWidgets/Common/Batch/QtStandardItemTreeAdapter.h"
#include <QPainter>
#include <QKeyEvent>

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

bool CellDelegate::eventFilter(QObject *object, QEvent *event) {
  QWidget *editor = qobject_cast<QWidget*>(object);
  if (!editor)
    return false;
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent *keyPress = static_cast<QKeyEvent *>(event);
    if (keyPress->key() == Qt::Key_Return) {
      auto current_filtered_index = m_view.currentIndex();
      emit commitData(editor);
    }
  }
  return false;
}
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
