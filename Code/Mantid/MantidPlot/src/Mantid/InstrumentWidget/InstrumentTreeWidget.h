#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

#include <QTreeView>
#include "InstrumentTreeModel.h"
#include "MantidGeometry/IComponent.h"

class InstrumentActor;

/** The InstrumentTreeWidget is a tree view
 * of the components of an instrument.
 *
 * Author: ???
 */

class InstrumentTreeWidget:public QTreeView
{
  Q_OBJECT
public:
  InstrumentTreeWidget(QWidget *w);
  void setInstrumentActor(InstrumentActor* instrActor);
  void getSelectedBoundingBox(const QModelIndex& index,double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin);
  //Mantid::Kernel::V3D getSamplePos()const;
  QModelIndex findComponentByName(const QString & name) const;
protected slots:
  void sendComponentSelectedSignal(const QModelIndex);
signals:
  void componentSelected(const Mantid::Geometry::ComponentID);
private:
  InstrumentActor* m_instrActor;
  boost::shared_ptr<const Mantid::API::MatrixWorkspace> m_workspace;
  boost::shared_ptr<const Mantid::Geometry::IInstrument> m_instrument;
  InstrumentTreeModel *m_treeModel;
};

#endif
