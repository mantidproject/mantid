#ifndef INSTRUMENTTREEMODEL_H
#define INSTRUMENTTREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>

#include <boost/shared_ptr.hpp>

namespace MantidQt {
namespace MantidWidgets {
class InstrumentActor;

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
  InstrumentTreeModel(const InstrumentActor *, QObject *parent);
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

private:
  const InstrumentActor *
      m_instrumentActor; ///< actor of instrument to which the model corresponds
};
} // MantidWidgets
} // MantidQt

#endif // INSTRUMENTTREEMODEL_H
