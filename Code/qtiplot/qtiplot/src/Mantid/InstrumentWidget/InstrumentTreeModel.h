#ifndef INSTRUMENTTREEMODEL_H
#define INSTRUMENTTREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include "MantidAPI/IInstrument.h"

class InstrumentTreeModel:public QAbstractItemModel
{
    Q_OBJECT
public:
    InstrumentTreeModel(const  boost::shared_ptr<Mantid::API::IInstrument>&, QObject *parent=0);
	~InstrumentTreeModel();

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column,const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &paren = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

private:
    boost::shared_ptr<Mantid::API::IInstrument> mInstrument; ///< instrument to which the model corresponds to
};

#endif
