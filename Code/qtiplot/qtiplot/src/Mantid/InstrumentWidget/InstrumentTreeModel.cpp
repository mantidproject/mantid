#include "InstrumentTreeModel.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/Instrument/Instrument.h"
#include "MantidObject.h"

using Mantid::Geometry::IInstrument;
using Mantid::Geometry::IComponent;
using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IObjComponent;

/**
 * Constructor for tree model to display instrument tree
 */
InstrumentTreeModel::InstrumentTreeModel(const  boost::shared_ptr<IInstrument>& data, QObject *parent) : QAbstractItemModel(parent)
{
	mInstrument=data;
}

/**
 * Destructor for instrument display tree
 */
InstrumentTreeModel::~InstrumentTreeModel()
{
}

/**
 * Column count for the instrument tree.
 *  Returns a count of 1 for the Component Assembly = I think this means "I have child nodes"
 *  Returns 0 for the ObjComponent = I'm an end point.
 */
int InstrumentTreeModel::columnCount(const QModelIndex &parent) const
{
	try
	{
		if (parent.isValid())
		{
			boost::shared_ptr<IComponent> comp=mInstrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()));
			boost::shared_ptr<ICompAssembly> objcomp=boost::dynamic_pointer_cast<ICompAssembly>(comp);
			if(objcomp!=boost::shared_ptr<ICompAssembly>())
				return 1;
			return 0;
		}
		else
			return 1;
	}
	catch(...)
	{
	  //std::cout<<"Exception :: columnCount"<<std::endl;
	  return 0;
	}
}

/**
 * Returns the string corresponding to the component name. the root of tree string will be instrument name
 */
QVariant InstrumentTreeModel::data(const QModelIndex &index, int role) const
{
	try
	{
		if(role != Qt::DisplayRole)
			return QVariant();

		if(!index.isValid()) // not valid has to return the root node
			return QString(mInstrument->getName().c_str());

		boost::shared_ptr<IComponent> ins= mInstrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));
		if(ins!=boost::shared_ptr<IComponent>())
			return QString(ins->getName().c_str());
		return QString("Error");
	}
	catch(...)
	{
	  //std::cout<<" Exception: in data"<<std::endl;
	  return 0;
	}
}

/**
 * Flags whether node in the tree is selectable or not. In instrument tree all components are selectable.
 */
Qt::ItemFlags InstrumentTreeModel::flags(const QModelIndex &index) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

/**
 * Instrument header. returns nothing. no header for tree
 */
QVariant InstrumentTreeModel::headerData(int section, Qt::Orientation orientation,int role) const
{
	return QVariant();
}

/**
 * Returns the ModelIndex at a give row and column and the parent.
 */
QModelIndex InstrumentTreeModel::index(int row, int column, const QModelIndex &parent) const
{
//	std::cout<<"Index +++++++++ row"<<row<<" column "<<column<<" is valid "<<parent.isValid()<<std::endl;
	try
	{
		boost::shared_ptr<ICompAssembly> parentItem;
		if(!parent.isValid()) //invalid parent, has to be the root node i.e instrument
			return createIndex(row,column,mInstrument->getComponentID());

		boost::shared_ptr<IComponent> comp=mInstrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()));
		parentItem = boost::dynamic_pointer_cast<ICompAssembly>(comp);
		if(!parentItem)
		{
			boost::shared_ptr<IObjComponent> objcomp=boost::dynamic_pointer_cast<IObjComponent>(comp);
			if(objcomp!=boost::shared_ptr<IObjComponent>()) return QModelIndex();
			//Not an instrument so check for Component Assembly
			parentItem = boost::dynamic_pointer_cast<ICompAssembly>(mInstrument);
		}
		//If component assembly pick the Component at the row index. if row index is higher than number
		// of components in assembly return empty model index
		if(parentItem->nelements()<row)
		{
			return QModelIndex();
		}
		else
		{
			return createIndex(row,column,(void*)((*parentItem)[row]->getComponentID()));
		}
	}
	catch(...)
	{
	  std::cout << "InstrumentTreeModel::index(" << row << "," << column << ") threw an exception." << std::endl;
	}
	return QModelIndex();
}

/**
 * Returns the parent model index.
 */
QModelIndex InstrumentTreeModel::parent(const QModelIndex &index) const
{
//	std::cout<<"parent +++++++++ row"<<index.row()<<" column "<<index.column()<<" is valid "<<index.isValid()<<std::endl;
	try
	{
		if(!index.isValid()) // the index corresponds to root so there is no parent for root return empty.
			return QModelIndex();

		if(mInstrument->getComponentID()==static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()))
			return QModelIndex();

		boost::shared_ptr<IComponent> child= mInstrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));		
		if(child->getParent()->getComponentID()==mInstrument->getComponentID())
			return createIndex(0,0,mInstrument->getComponentID());
		boost::shared_ptr<IComponent> parent=mInstrument->getComponentByID(child->getParent()->getComponentID());
		boost::shared_ptr<IComponent> greatParent=mInstrument->getComponentByID(parent->getParent()->getComponentID());
		boost::shared_ptr<ICompAssembly> greatParentAssembly=boost::dynamic_pointer_cast<ICompAssembly>(greatParent);
		int iindex=0;
		for(int i=0;i<greatParentAssembly->nelements();i++)
			if((*greatParentAssembly)[i]->getComponentID()==parent->getComponentID())
				iindex=i;
		return createIndex(iindex, 0, (void*)parent->getComponentID());
	}
	catch(...)
	{
	  //		std::cout<<"Exception: in parent"<<std::endl;
	}
	return QModelIndex();
}

/** 
 * Return the row count. the number of elements in the component. for ObjComponent row count will be 0.
 */
int InstrumentTreeModel::rowCount(const QModelIndex &parent) const
{
//	std::cout<<"rowCount +++++++++ row"<<parent.row()<<" column "<<parent.column()<<" is valid "<<parent.isValid()<<std::endl;

	try
	{
		if(!parent.isValid()) // Root node row count is one.
		{
			return 1;//boost::dynamic_pointer_cast<ICompAssembly>(mInstrument)->nelements();
		}
		else
		{
			if(mInstrument->getComponentID()==static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()))
				return boost::dynamic_pointer_cast<ICompAssembly>(mInstrument)->nelements();
			boost::shared_ptr<IComponent> comp=mInstrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(parent.internalPointer()));//static_cast<IComponent*>(parent.internalPointer());
			boost::shared_ptr<ICompAssembly> assembly=boost::dynamic_pointer_cast<ICompAssembly>(comp);
			if(assembly!=boost::shared_ptr<ICompAssembly>())
			{
				return assembly->nelements();
			}
			boost::shared_ptr<IObjComponent> objcomp=boost::dynamic_pointer_cast<IObjComponent>(comp);
			if(objcomp!=boost::shared_ptr<IObjComponent>())
				return 0;
		}
	}
	catch(...)
	{
	  //std::cout<<"Exception: in rowCount"<<std::endl;
	}
	return 0;
}
