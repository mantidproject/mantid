#include "InstrumentTreeWidget.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidGeometry/ICompAssembly.h"
#include "MantidObject.h"
#include "GLActor.h"
#include <queue>
#include <QMessageBox>
#include <QString>
#include <cfloat>

void InstrumentTreeWidget::setInstrument(boost::shared_ptr<Mantid::API::IInstrument> ins)
{
	mInstrument=ins;
	ParseInstrumentGeometry();
}

void InstrumentTreeWidget::ParseInstrumentGeometry()
{
	clear();
    std::queue<boost::shared_ptr<Mantid::Geometry::IComponent> > CompList;
	std::queue<QTreeWidgetItem *> itemQueue;
	CompList.push(mInstrument);
	//Add top level instrument
	QTreeWidgetItem *topItem = new QTreeWidgetItem(QStringList(mInstrument->getName().c_str()));
	itemQueue.push(topItem);              
    addTopLevelItem(topItem);

	while(!CompList.empty())
	{
        boost::shared_ptr<Mantid::Geometry::IComponent> tmp = CompList.front();
		QTreeWidgetItem* item=itemQueue.front();
		item->setFlags(Qt::ItemIsEnabled |  Qt::ItemIsSelectable);
		CompList.pop();
		itemQueue.pop();
		//std::cout<<" Component: "<<tmp->getName()<<std::endl;
		//std::cout<<" Component Type:"<<tmp->type()<<std::endl;
        if(boost::dynamic_pointer_cast<Mantid::Geometry::IObjComponent>(tmp)){
			//boost::shared_ptr<MantidObject> obj(new MantidObject(dynamic_cast<Mantid::Geometry::ObjComponent*>(tmp)));
		/*} else if(tmp->type()=="Instrument"){
			Mantid::API::Instrument *tmpIns=dynamic_cast<Mantid::API::Instrument*>(tmp);
			for(int idx=0;idx<tmpIns->nelements();idx++)
			{
				CompList.push((*tmpIns)[idx]);
				QTreeWidgetItem* newItem=new QTreeWidgetItem(QStringList((*tmpIns)[idx]->getName().c_str()));
				item->addChild(newItem);
				itemQueue.push(newItem);
			}*/
		} else if(boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(tmp)){
            boost::shared_ptr<Mantid::Geometry::ICompAssembly> tmpAssem = boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(tmp);
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

void InstrumentTreeWidget::getSelectedBoundingBox(double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin)
{
	QList<QTreeWidgetItem*> selectedList=this->selectedItems();
    boost::shared_ptr<Mantid::Geometry::IComponent> selectedComponent = mInstrument;
	if(selectedList.size()==1)
	{
		QTreeWidgetItem* selectedItem=selectedList.first();
		//prepare a list of the hierarchy indexes
		if(selectedItem->parent()!=0){
			QList<int> indexes;
			do
			{
				QTreeWidgetItem* parentItem=selectedItem->parent();
				int selectedIndex=parentItem->indexOfChild(selectedItem);
				indexes.push_front(selectedIndex);
				selectedItem=parentItem;
			}while(selectedItem->parent()!=0);

			//parse through the Instrument hierarchy to get the component selected
			selectedComponent=mInstrument;
			/*while(indexes.size()!=0)
			{
				int index=indexes.takeFirst();
				if(selectedComponent->type()=="Instrument"){
					Mantid::API::Instrument *tmpIns=dynamic_cast<Mantid::API::Instrument*>(selectedComponent);
					selectedComponent=(*tmpIns)[index];
				} else if(selectedComponent->type()=="CompAssembly"){
					Mantid::Geometry::CompAssembly *tmpAssem=dynamic_cast<Mantid::Geometry::CompAssembly*>(selectedComponent);
					selectedComponent=(*tmpAssem)[index];
				} 
			}*/
			while(indexes.size()!=0)
			{
				int index=indexes.takeFirst();
                boost::shared_ptr<Mantid::Geometry::ICompAssembly> tmpAssem = boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(selectedComponent);
                if(tmpAssem){
					selectedComponent=(*tmpAssem)[index];
				} 
			}
		}
		//QString info;
		//info= selectedComponent->getName().c_str();
		//QMessageBox::information(this,tr("Detector Name Information"), info, QMessageBox::Ok|QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);

		//get the bounding box for the component
		xmax=ymax=zmax=-DBL_MAX;
		xmin=ymin=zmin=DBL_MAX;
        std::queue<boost::shared_ptr<Mantid::Geometry::IComponent> > CompList;
		CompList.push(selectedComponent);

		while(!CompList.empty())
		{
            boost::shared_ptr<Mantid::Geometry::IComponent> tmp = CompList.front();
			CompList.pop();
            boost::shared_ptr<Mantid::Geometry::IObjComponent> tmpObj = boost::dynamic_pointer_cast<Mantid::Geometry::IObjComponent>(tmp);
			if(tmpObj){
				double txmax,tymax,tzmax,txmin,tymin,tzmin;
				txmax=tymax=tzmax=-10000;
				txmin=tymin=tzmin=10000;
				tmpObj->getBoundingBox(txmax,tymax,tzmax,txmin,tymin,tzmin);
				if(txmax>xmax)xmax=txmax; if(tymax>ymax)ymax=tymax;if(tzmax>zmax)zmax=tzmax;
				if(txmin<xmin)xmin=txmin; if(tymin<ymin)ymin=tymin;if(tzmin<zmin)zmin=tzmin;
			/*} else if(tmp->type()=="Instrument"){
				Mantid::API::Instrument *tmpIns=dynamic_cast<Mantid::API::Instrument*>(tmp);
				for(int idx=0;idx<tmpIns->nelements();idx++)
				{
					CompList.push((*tmpIns)[idx]);
				}*/
            } else if(boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(tmp)){
                boost::shared_ptr<Mantid::Geometry::ICompAssembly> tmpAssem = boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(tmp);
				for(int idx=0;idx<tmpAssem->nelements();idx++)
				{
					CompList.push((*tmpAssem)[idx]);
				}
			} 
		}
	}
}

Mantid::Geometry::V3D InstrumentTreeWidget::getSamplePos()const
{
    boost::shared_ptr<Mantid::Geometry::IComponent> sample = mInstrument->getSample();
		if(sample!=NULL)
			return sample->getPos();
		else
			return Mantid::Geometry::V3D(0.0,0.0,0.0);
}
