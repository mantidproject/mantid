#include "MantidQtWidgets/Common/DataProcessorUI/TreeManager.h"
#include <QStyledItemDelegate>
#include <QPainter>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class GridDelegate : public QStyledItemDelegate {
public:
  explicit GridDelegate(QObject *parent = 0, TreeManager *model = 0)
      : QStyledItemDelegate(parent), m_model(model){};

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const {
    painter->save();
    painter->setPen(QColor(Qt::black));
    if (m_model->isProcessed(index.row(), index.parent().row())) {
      painter->fillRect(option.rect, Qt::green);
    }
    painter->drawRect(option.rect);
    painter->restore();

    QStyledItemDelegate::paint(painter, option, index);
  }

protected:
  TreeManager *m_model;
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace Mantid