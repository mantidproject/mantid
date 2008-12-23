#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

#include <QTreeWidget>
#include "MantidAPI/IInstrument.h"

class InstrumentTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    InstrumentTreeWidget(QWidget *w):QTreeWidget(w){};
	void setInstrument(boost::shared_ptr<Mantid::API::IInstrument>);
	void getSelectedBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin);
	Mantid::Geometry::V3D getSamplePos()const;
private:
	void ParseInstrumentGeometry();
    boost::shared_ptr<Mantid::API::IInstrument> mInstrument;
};

#endif
