#include "InstrumentTreeModel.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidAPI/Instrument.h"
#include "MantidObject.h"

/**
 * Constructor for tree model to display instrument tree
 */
InstrumentTreeModel::InstrumentTreeModel(const  boost::shared_ptr<Mantid::API::IInstrument>& data, QObject *parent) : QAbstractItemModel(parent)
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
 * Column count for the instrument tree. count is 1 for the Component Assembly and 0 for the ObjComponent.
 */
int InstrumentTreeModel::columnCount(const QModelIndex &parent) const
{
	try
	{
		if (parent.isValid())
		{
			Mantid::Geometry::IComponent* component=static_cast<Mantid::Geometry::IComponent*> (parent.internalPointer());
			Mantid::Geometry::IObjComponent* objcomp=dynamic_cast<Mantid::Geometry::IObjComponent*>(component);
			if(objcomp)
				return 0;
			return 1;
		}
		else
			return 1;
	}
	catch(...)
	{
		std::cout<<"Exception :: columnCount"<<std::endl;
	}
}

/**
 * Returns the string corresponding to the component name. the root of tree string will be instrument name
 */
QVariant InstrumentTreeModel::data(const QModelIndex &index, int role) const
{
//	std::cout<<"Data +++++++++ row"<<index.row()<<" column "<<index.column()<<" is valid "<<index.isValid()<<std::endl;
	try
	{
		if(role != Qt::DisplayRole)
			return QVariant();

		if(!index.isValid()) // not valid has to return the root node
			return QString(mInstrument->getName().c_str());
		Mantid::Geometry::IComponent* component=static_cast<Mantid::Geometry::IComponent*> (index.internalPointer());
		Mantid::Geometry::ICompAssembly* tmpAssem=dynamic_cast<Mantid::Geometry::ICompAssembly*>(component);
		//Check whether its a component assembly
		if(tmpAssem)
			return QString((tmpAssem)->getName().c_str());
		Mantid::Geometry::IObjComponent* tmpObj=dynamic_cast<Mantid::Geometry::IObjComponent*>(component);
		//Check whether its a object component
		if(tmpObj)
			return QString(component->getName().c_str());
		//If not ObjComponent or CompAssembly then it has to instrument 
		//Used this as some problem casting the Instrument to IComponent
		Mantid::API::Instrument* ins=static_cast<Mantid::API::Instrument*>(index.internalPointer());
		return QString(ins->getName().c_str());
	}
	catch(...)
	{
		std::cout<<" Exception: in data"<<std::endl;
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
		Mantid::Geometry::ICompAssembly *parentItem=NULL;
		if(!parent.isValid()) //invalid parent, has to be the root node i.e instrument
			return createIndex(row,column,mInstrument.get());

		Mantid::Geometry::IComponent *comp=static_cast<Mantid::Geometry::IComponent*>(parent.internalPointer());
		parentItem = dynamic_cast<Mantid::Geometry::ICompAssembly*>(comp);
		if(!parentItem)
		{
			//Used this as some problem casting the Instrument to IComponent
			Mantid::API::Instrument* ins=static_cast<Mantid::API::Instrument*>(parent.internalPointer());
			parentItem = dynamic_cast<Mantid::Geometry::ICompAssembly*>(ins);
			if(!parentItem) return QModelIndex();
			//Not an instrument so check for Component Assembly
			parentItem = boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(mInstrument).get();
		}
		//If component assembly pick the Component at the row index. if row index is higher than number
		// of components in assembly return empty model index
		if(parentItem->nelements()<row)
		{
			return QModelIndex();
		}
		else
		{
			return createIndex(row,column,(void*)((*parentItem)[row].get()));
		}
	}
	catch(...)
	{
		std::cout<<"Exception: in index"<<std::endl;
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

		Mantid::Geometry::IComponent *child= static_cast<Mantid::Geometry::IComponent*>(index.internalPointer());
		if(!dynamic_cast<Mantid::Geometry::ICompAssembly*>(child)&&!dynamic_cast<Mantid::Geometry::IObjComponent*>(child))
			return QModelIndex();
		if(child->getParent()==mInstrument.get())
			return createIndex(0,0,mInstrument.get());
		const Mantid::Geometry::ICompAssembly *greatParent=dynamic_cast<const Mantid::Geometry::ICompAssembly*>(child->getParent()->getParent());
		int iindex=0;
		for(int i=0;i<greatParent->nelements();i++)
			if((*greatParent)[i].get()==child->getParent())
				iindex=i;
		return createIndex(iindex, 0, (void*)child->getParent());
	}
	catch(...)
	{
		std::cout<<"Excpetion: in parent"<<std::endl;
	}
	return QModelIndex();
}

/** 
 * Return the row count. the number of elements in the component. for ObjComponent row count will be 0.
 */
int InstrumentTreeModel::rowCount(const QModelIndex &parent) const
{
//	std::cout<<"Data +++++++++ row"<<parent.row()<<" column "<<parent.column()<<" is valid "<<parent.isValid()<<std::endl;

	try
	{
		if(!parent.isValid()) // Root node row count is one.
		{
			return 1;//boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(mInstrument)->nelements();
		}
		else
		{
			Mantid::Geometry::IComponent* comp=static_cast<Mantid::Geometry::IComponent*>(parent.internalPointer());
			const Mantid::Geometry::ICompAssembly *assembly=dynamic_cast<const Mantid::Geometry::ICompAssembly*>(comp);
			if(assembly)
			{
				return assembly->nelements();
			}
			const Mantid::Geometry::IObjComponent *objcomp=dynamic_cast<const Mantid::Geometry::IObjComponent*>(comp);
			if(objcomp)
				return 0;
			return boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(mInstrument)->nelements();
		}
	}
	catch(...)
	{
		std::cout<<"Exception: in rowCount"<<std::endl;
	}
	return 0;
}