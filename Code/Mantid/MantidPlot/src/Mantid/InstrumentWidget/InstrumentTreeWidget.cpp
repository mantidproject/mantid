#include "InstrumentTreeWidget.h"
#include "InstrumentActor.h"
#ifdef WIN32
#include <windows.h>
#endif
#include "MantidKernel/Exception.h"
#include "MantidGeometry/ICompAssembly.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "GLActor.h"
#include <queue>
#include <QMessageBox>
#include <QString>
#include <cfloat>
#include <iostream>

InstrumentTreeWidget::InstrumentTreeWidget(QWidget *w):QTreeView(w), m_treeModel(0) 
{
  connect(this,SIGNAL(clicked(const QModelIndex)),this,SLOT(sendComponentSelectedSignal(const QModelIndex)));
};

void InstrumentTreeWidget::setInstrumentActor(InstrumentActor* instrActor)
{
  m_instrActor = instrActor;
  m_workspace = instrActor->getWorkspace();
  m_instrument = instrActor->getInstrument();
  m_treeModel=new InstrumentTreeModel(m_workspace);
  setModel(m_treeModel);
  setSelectionMode(SingleSelection);
  setSelectionBehavior(SelectRows);
}

void InstrumentTreeWidget::getSelectedBoundingBox(const QModelIndex& index,double &xmax, double &ymax, double &zmax, double &xmin, double &ymin, double &zmin)
{
  //Check whether its instrument
  boost::shared_ptr<const Mantid::Geometry::IComponent> selectedComponent;
  if(m_instrument->getComponentID()==static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()))
    selectedComponent=boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(m_instrument);
  else
    selectedComponent=m_instrument->getComponentByID(static_cast<Mantid::Geometry::ComponentID>(index.internalPointer()));

  //get the bounding box for the component
  xmax=ymax=zmax=-DBL_MAX;
  xmin=ymin=zmin=DBL_MAX;
  Mantid::Geometry::BoundingBox boundBox;
  std::queue<boost::shared_ptr<const Mantid::Geometry::IComponent> > CompList;
  CompList.push(selectedComponent);
  while(!CompList.empty())
  {
    boost::shared_ptr<const Mantid::Geometry::IComponent> tmp = CompList.front();
    CompList.pop();
    boost::shared_ptr<const Mantid::Geometry::IObjComponent> tmpObj = boost::dynamic_pointer_cast<const Mantid::Geometry::IObjComponent>(tmp);
    if(tmpObj){
      try{
        //std::cerr << int(tmpObj->getComponentID()) << ' ' << int(m_instrument->getSample()->getComponentID()) << std::endl;
        if (tmpObj->getComponentID() == m_instrument->getSample()->getComponentID())
        {
          boundBox = m_workspace->sample().getShape().getBoundingBox();
          boundBox.moveBy(tmpObj->getPos());
        }
        else
        {
          tmpObj->getBoundingBox(boundBox);
        }
        double txmax(boundBox.xMax()),tymax(boundBox.yMax()),tzmax(boundBox.zMax()),
          txmin(boundBox.xMin()),tymin(boundBox.yMin()),tzmin(boundBox.zMin());
        if(txmax>xmax)xmax=txmax; 
        if(tymax>ymax)ymax=tymax;
        if(tzmax>zmax)zmax=tzmax;
        if(txmin<xmin)xmin=txmin; 
        if(tymin<ymin)ymin=tymin;
        if(tzmin<zmin)zmin=tzmin;
      }
      catch(Mantid::Kernel::Exception::NullPointerException &)
      {
      }
    } else if(boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(tmp)){
      boost::shared_ptr<const Mantid::Geometry::ICompAssembly> tmpAssem = boost::dynamic_pointer_cast<const Mantid::Geometry::ICompAssembly>(tmp);
      for(int idx=0;idx<tmpAssem->nelements();idx++)
      {
        CompList.push((*tmpAssem)[idx]);
      }
    }
  }
}

//Mantid::Kernel::V3D InstrumentTreeWidget::getSamplePos()const
//{
//  boost::shared_ptr<const Mantid::Geometry::IComponent> sample = m_instrument->getSample();
//  if(sample!=NULL)
//    return sample->getPos();
//  else
//    return Mantid::Kernel::V3D(0.0,0.0,0.0);
//}

QModelIndex InstrumentTreeWidget::findComponentByName(const QString & name) const
{
  if( !m_treeModel ) return QModelIndex();
  //The data is in a tree model so recursively search until we find the string we want. Note the match is NOT
  //case sensitive
  QModelIndexList matches = m_treeModel->match(m_treeModel->index(0, 0, QModelIndex()), Qt::DisplayRole, name, 1, 
    Qt::MatchFixedString | Qt::MatchRecursive);
  if( matches.isEmpty() ) return QModelIndex();
  return matches.first();
}

void InstrumentTreeWidget::sendComponentSelectedSignal(const QModelIndex index)
{
  Mantid::Geometry::ComponentID id = static_cast<Mantid::Geometry::ComponentID>(index.internalPointer());
  m_instrActor->accept(SetVisibleComponentVisitor(id));
  emit componentSelected(id);
}
