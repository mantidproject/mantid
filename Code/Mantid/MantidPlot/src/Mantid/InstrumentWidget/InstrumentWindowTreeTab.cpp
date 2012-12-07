#include "InstrumentWindow.h"
#include "InstrumentWindowTreeTab.h"
#include "InstrumentTreeWidget.h"
#include "InstrumentActor.h"

InstrumentWindowTreeTab::InstrumentWindowTreeTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    //Tree Controls
    m_instrumentTree = new InstrumentTreeWidget(0);
    layout->addWidget(m_instrumentTree);
    connect(m_instrumentTree,SIGNAL(componentSelected(Mantid::Geometry::ComponentID)),
                m_instrWindow,SLOT(componentSelected(Mantid::Geometry::ComponentID)));
}

void InstrumentWindowTreeTab::initSurface()
{
    m_instrumentTree->setInstrumentActor(m_instrWindow->getInstrumentActor());
}

/**
  * Find an instrument component by its name.
  * @param name :: Name of an instrument component.
  */
QModelIndex InstrumentWindowTreeTab::findComponentByName(const QString &name)
{
    return QModelIndex();
}

/**
  * Clean up on becoming invisible.
  */
void InstrumentWindowTreeTab::hideEvent(QHideEvent *)
{
    m_instrWindow->getInstrumentActor()->accept(SetAllVisibleVisitor());
}

