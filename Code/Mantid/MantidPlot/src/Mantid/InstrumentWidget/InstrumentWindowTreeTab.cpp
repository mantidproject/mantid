#include "InstrumentWindow.h"
#include "InstrumentWindowTreeTab.h"
#include "InstrumentTreeWidget.h"
#include "InstrumentActor.h"
#include "ProjectionSurface.h"
#include "GLActorVisitor.h"

InstrumentWindowTreeTab::InstrumentWindowTreeTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    //Tree Controls
    m_instrumentTree = new InstrumentTreeWidget(0);
    layout->addWidget(m_instrumentTree);
    connect(m_instrumentTree,SIGNAL(componentSelected(Mantid::Geometry::ComponentID)),
                m_instrWindow,SLOT(componentSelected(Mantid::Geometry::ComponentID)));
    connect(m_instrWindow,SIGNAL(requestSelectComponent(QString)),this,SLOT(selectComponentByName(QString)));
}

void InstrumentWindowTreeTab::initSurface()
{
    m_instrumentTree->setInstrumentActor(m_instrWindow->getInstrumentActor());
}

/**
  * Find an instrument component by its name.
  * @param name :: Name of an instrument component.
  */
void InstrumentWindowTreeTab::selectComponentByName(const QString &name)
{
      QModelIndex component = m_instrumentTree->findComponentByName(name);
      if( !component.isValid() )
      {
        throw std::invalid_argument("No component named \'"+name.toStdString()+"\' found");
      }

      m_instrumentTree->clearSelection();
      m_instrumentTree->scrollTo(component, QAbstractItemView::EnsureVisible );
      m_instrumentTree->selectionModel()->select(component, QItemSelectionModel::Select);
      m_instrumentTree->sendComponentSelectedSignal(component);
}

/**
  * Update surface when tab becomes visible.
  */
void InstrumentWindowTreeTab::showEvent(QShowEvent *)
{
    getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
}

