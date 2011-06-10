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
  InstrumentTreeWidget(QWidget *w);
  void setInstrument(boost::shared_ptr<const Mantid::Geometry::IInstrument>);
  void getSelectedBoundingBox(const QModelIndex& index,double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin);
  Mantid::Geometry::V3D getSamplePos()const;
  QModelIndex findComponentByName(const QString & name) const;
protected slots:
  void sendComponentSelectedSignal(const QModelIndex);
signals:
  void componentSelected(const Mantid::Geometry::ComponentID);
private:
  boost::shared_ptr<const Mantid::Geometry::IInstrument> mInstrument;
  InstrumentTreeModel *mTreeModel;
};

#endif
