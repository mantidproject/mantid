#ifndef MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#define MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#include <QStyledItemDelegate>
#include <QTreeView>

namespace MantidQt {
namespace MantidWidgets {
namespace Batch {
class CellDelegate : public QStyledItemDelegate {
public:
  explicit CellDelegate(QObject *parent, QTreeView const &view);
  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override;

private:
  QTreeView const &m_view;
};
}
}
}
#endif // MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
