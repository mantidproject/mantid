#include "InstrumentTreeWidget.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidObject.h"
#include "GLActor.h"
#include <queue>

void InstrumentTreeWidget::setInstrument(Mantid::API::Instrument* ins)
{
	mInstrument=ins;
	ParseInstrumentGeometry();
}

void InstrumentTreeWidget::ParseInstrumentGeometry()
{
	clear();
	std::queue<Mantid::Geometry::Component *> CompList;
	std::queue<QTreeWidgetItem *> itemQueue;
	CompList.push(mInstrument);
	//Add top level instrument
	QTreeWidgetItem *topItem = new QTreeWidgetItem(QStringList(mInstrument->getName().c_str()));
	itemQueue.push(topItem);              
    addTopLevelItem(topItem);

	while(!CompList.empty())
	{
		Mantid::Geometry::Component* tmp = CompList.front();
		QTreeWidgetItem* item=itemQueue.front();
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
		item->setCheckState(0, Qt::Unchecked);
		CompList.pop();
		itemQueue.pop();
		//std::cout<<" Component: "<<tmp->getName()<<std::endl;
		//std::cout<<" Component Type:"<<tmp->type()<<std::endl;
		if(tmp->type()=="PhysicalComponent" ||tmp->type()=="DetectorComponent"){
			//boost::shared_ptr<MantidObject> obj(new MantidObject(dynamic_cast<Mantid::Geometry::ObjComponent*>(tmp)));
		} else if(tmp->type()=="Instrument"){
			Mantid::API::Instrument *tmpIns=dynamic_cast<Mantid::API::Instrument*>(tmp);
			for(int idx=0;idx<tmpIns->nelements();idx++)
			{
				CompList.push((*tmpIns)[idx]);
				QTreeWidgetItem* newItem=new QTreeWidgetItem(QStringList((*tmpIns)[idx]->getName().c_str()));
				item->addChild(newItem);
				itemQueue.push(newItem);
			}
		} else if(tmp->type()=="CompAssembly"){
			Mantid::Geometry::CompAssembly *tmpAssem=dynamic_cast<Mantid::Geometry::CompAssembly*>(tmp);
			for(int idx=0;idx<tmpAssem->nelements();idx++)
			{
				CompList.push((*tmpAssem)[idx]);
				QTreeWidgetItem* newItem=new QTreeWidgetItem(QStringList((*tmpAssem)[idx]->getName().c_str()));
				item->addChild(newItem);
				itemQueue.push(newItem);
			}
		} 
	}	
}