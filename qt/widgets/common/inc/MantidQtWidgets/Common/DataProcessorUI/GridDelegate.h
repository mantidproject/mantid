// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include <QPainter>
#include <QStyledItemDelegate>

namespace MantidQt {
namespace MantidWidgets {
namespace DataProcessor {

class GridDelegate : public QStyledItemDelegate {
public:
  explicit GridDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent){};

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {

    QStyledItemDelegate::paint(painter, option, index);

    painter->save();
    painter->setPen(QColor(Qt::black));
    painter->drawRect(option.rect);
    painter->restore();
  }
};

} // namespace DataProcessor
} // namespace MantidWidgets
} // namespace MantidQt