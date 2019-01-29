// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
/**
See the developer documentation for Batch Widget at
developer.mantidproject.org/BatchWidget/index.html
*/
#ifndef MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#define MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
#include "MantidQtWidgets/Common/Batch/FilteredTreeModel.h"
#include <QStandardItemModel>
#include <QStyledItemDelegate>
#include <QTreeView>

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
} // namespace Batch
} // namespace MantidWidgets
} // namespace MantidQt
#endif // MANTIDQTMANTIDWIDGETS_CELLDELEGATE_H_
