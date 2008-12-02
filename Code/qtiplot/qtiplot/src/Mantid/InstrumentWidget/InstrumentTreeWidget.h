#ifndef INSTRUMENTTREEWIDGET_H
#define INSTRUMENTTREEWIDGET_H

#include <QTreeWidget>
#include "MantidAPI/Instrument.h"

class InstrumentTreeWidget:public QTreeWidget
{
    Q_OBJECT
public:
    InstrumentTreeWidget(QWidget *w):QTreeWidget(w){};
	void setInstrument(Mantid::API::Instrument*);
private:
	void ParseInstrumentGeometry();
	Mantid::API::Instrument* mInstrument;
};

#endif
