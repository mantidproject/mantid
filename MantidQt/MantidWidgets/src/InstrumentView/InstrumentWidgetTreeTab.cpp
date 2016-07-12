#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentWidgetTreeTab.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentTreeWidget.h"
#include "MantidQtMantidWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtMantidWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtMantidWidgets/InstrumentView/GLActorVisitor.h"

#include <QVBoxLayout>
#include <QMessageBox>

namespace MantidQt {
namespace MantidWidgets {

InstrumentWidgetTreeTab::InstrumentWidgetTreeTab(InstrumentWidget *instrWidget)
    : InstrumentWidgetTab(instrWidget) {
  QVBoxLayout *layout = new QVBoxLayout(this);
  // Tree Controls
  m_instrumentTree = new InstrumentTreeWidget(0);
  layout->addWidget(m_instrumentTree);
  connect(m_instrumentTree,
          SIGNAL(componentSelected(Mantid::Geometry::ComponentID)),
          m_instrWidget,
          SLOT(componentSelected(Mantid::Geometry::ComponentID)));
  connect(m_instrWidget, SIGNAL(requestSelectComponent(QString)), this,
          SLOT(selectComponentByName(QString)));
}

void InstrumentWidgetTreeTab::initSurface() {
  m_instrumentTree->setInstrumentActor(m_instrWidget->getInstrumentActor());
}

/**
        * Find an instrument component by its name. This is used from the
        * scripting API and errors (component not found) are shown as a
        * message box in the GUI.
        *
        * @param name :: Name of an instrument component.
        */
void InstrumentWidgetTreeTab::selectComponentByName(const QString &name) {
  QModelIndex component = m_instrumentTree->findComponentByName(name);
  if (!component.isValid()) {
    QMessageBox::warning(this, "Instrument Window - Tree Tab - Error",
                         "No component named '" + name +
                             "' was found. "
                             "Please use a valid component name ");
    return;
  }

  m_instrumentTree->clearSelection();
  m_instrumentTree->scrollTo(component, QAbstractItemView::EnsureVisible);
  m_instrumentTree->selectionModel()->select(component,
                                             QItemSelectionModel::Select);
  m_instrumentTree->sendComponentSelectedSignal(component);
}

/**
        * Update surface when tab becomes visible.
        */
void InstrumentWidgetTreeTab::showEvent(QShowEvent *) {
  getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
}
} // MantidWidgets
} // MantidQt