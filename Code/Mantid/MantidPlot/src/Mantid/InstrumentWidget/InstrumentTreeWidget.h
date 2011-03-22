#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

#include <QTreeView>
#include "MantidGeometry/IInstrument.h"
#include "InstrumentTreeModel.h"


/** The InstrumentTreeWidget is a tree view
 * of the components of an instrument.
 *
 * Author: ???
 */

class InstrumentTreeWidget:public QTreeView
{
  Q_OBJECT
public:
  InstrumentTreeWidget(QWidget *w):QTreeView(w), mTreeModel(0) {};
  void setInstrument(boost::shared_ptr<Mantid::Geometry::IInstrument>);
  void getSelectedBoundingBox(const QModelIndex& index,double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin);
  Mantid::Geometry::V3D getSamplePos()const;
  QModelIndex findComponentByName(const QString & name) const;
  void sendComponentSelectedSignal(QModelIndex index);
signals:
  void componentSelected(const Mantid::Geometry::ComponentID);
private:
  boost::shared_ptr<Mantid::Geometry::IInstrument> mInstrument;
  InstrumentTreeModel *mTreeModel;
};

#endif
