#include "InstrumentTreeWidget.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidObject.h"
#include "GLActor.h"
#include <queue>
#include <QMessageBox>
#include <QString>
#include <cfloat>
#include <iostream>

void InstrumentTreeWidget::setInstrument(boost::shared_ptr<Mantid::API::IInstrument> ins)
{
	mInstrument=ins;
	mTreeModel=new InstrumentTreeModel(ins);
	setModel(mTreeModel);
	setSelectionMode(SingleSelection);
	setSelectionBehavior(SelectRows);
}

void InstrumentTreeWidget::getSelectedBoundingBox(const QModelIndex& index,double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin)
{
	Mantid::Geometry::IComponent* selectedComponent=static_cast<Mantid::Geometry::IComponent*>(index.internalPointer());
	if(!dynamic_cast<Mantid::Geometry::IObjComponent*>(selectedComponent)&&!dynamic_cast<Mantid::Geometry::ICompAssembly*>(selectedComponent))
		selectedComponent=(boost::dynamic_pointer_cast<Mantid::Geometry::ICompAssembly>(mInstrument)).get();
	//get the bounding box for the component
	xmax=ymax=zmax=-DBL_MAX;
	xmin=ymin=zmin=DBL_MAX;
	std::queue<Mantid::Geometry::IComponent*> CompList;
	CompList.push(selectedComponent);
	while(!CompList.empty())
	{
		Mantid::Geometry::IComponent* tmp = CompList.front();
		CompList.pop();
		Mantid::Geometry::IObjComponent* tmpObj = dynamic_cast<Mantid::Geometry::IObjComponent*>(tmp);
		if(tmpObj){
			try{
				double txmax,tymax,tzmax,txmin,tymin,tzmin;
				txmax=tymax=tzmax=-10000;
				txmin=tymin=tzmin=10000;
				tmpObj->getBoundingBox(txmax,tymax,tzmax,txmin,tymin,tzmin);
				if(txmax>xmax)xmax=txmax; if(tymax>ymax)ymax=tymax;if(tzmax>zmax)zmax=tzmax;
				if(txmin<xmin)xmin=txmin; if(tymin<ymin)ymin=tymin;if(tzmin<zmin)zmin=tzmin;
			}
			catch(Mantid::Kernel::Exception::NullPointerException Ex)
			{
			}
		} else if(dynamic_cast<Mantid::Geometry::ICompAssembly*>(tmp)){
			Mantid::Geometry::ICompAssembly *tmpAssem = dynamic_cast<Mantid::Geometry::ICompAssembly*>(tmp);
			for(int idx=0;idx<tmpAssem->nelements();idx++)
			{
				CompList.push((*tmpAssem)[idx].get());
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
