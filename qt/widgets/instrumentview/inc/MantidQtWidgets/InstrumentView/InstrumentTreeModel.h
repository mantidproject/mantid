#ifndef INSTRUMENTTREEMODEL_H
#define INSTRUMENTTREEMODEL_H

#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <QAbstractItemModel>

namespace MantidQt {
namespace MantidWidgets {

/**
 * The InstrumentTreeModel is a class used by a QTreeView
 * in order to display the components of an instrument as a
 * hierarchical tree view.
 *
 * It fills out the nodes in the tree as requested.
 *
 * Author: ???
 *
 */
class InstrumentTreeModel : public QAbstractItemModel {
  Q_OBJECT
public:
  InstrumentTreeModel(const InstrumentWidget *instrWidget, QObject *parent);
  ~InstrumentTreeModel() override;

  QVariant data(const QModelIndex &index, int role) const override;
  Qt::ItemFlags flags(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column,
                    const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  int rowCount(const QModelIndex &paren = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  static size_t extractIndex(const QModelIndex &index);

private:
  /// instrument widget to which the model corresponds
  const InstrumentWidget *m_instrWidget;
  mutable std::vector<size_t> m_componentIndices;
};
} // namespace MantidWidgets
} // namespace MantidQt

#endif // INSTRUMENTTREEMODEL_H
