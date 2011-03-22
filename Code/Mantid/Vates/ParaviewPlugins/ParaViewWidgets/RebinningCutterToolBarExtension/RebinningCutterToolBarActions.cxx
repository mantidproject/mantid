#include "RebinningCutterToolBarActions.h"
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <pqApplicationCore.h>
#include <pqOutputPort.h>
#include <pqServerManagerSelectionModel.h>
#include <pqServer.h>
#include <pqObjectBuilder.h>

RebinningCutterToolBarActions::RebinningCutterToolBarActions(QObject* p) :
  QActionGroup(p)
{

  QIcon icon(":/RebinningCutter.png");

  QAction* action = this->addAction(new QAction(icon, "Create Mantid Rebinning Cutter Filter", this));
  QObject::connect(action, SIGNAL(triggered(bool)), this, SLOT(createTargetFilter()));
}

RebinningCutterToolBarActions::~RebinningCutterToolBarActions()
{
}

pqPipelineSource* RebinningCutterToolBarActions::getActiveSource() const
{
  pqServerManagerModelItem *item = 0;
  pqServerManagerSelectionModel *selection = pqApplicationCore::instance()->getSelectionModel();
  const pqServerManagerSelection *selected = selection->selectedItems();
  if (selected->size() == 1)
  {
    item = selected->first();
  }
  else if (selected->size() > 1)
  {
    item = selection->currentItem();
    if (item && !selection->isSelected(item))
    {
      item = 0;
    }
  }

  if (item && qobject_cast<pqPipelineSource*> (item))
  {
    return static_cast<pqPipelineSource*> (item);
  }
  else if (item && qobject_cast<pqOutputPort*> (item))
  {
    pqOutputPort* port = static_cast<pqOutputPort*> (item);
    return port->getSource();
  }

  return 0;
}

void RebinningCutterToolBarActions::createTargetFilter()
{
  // Get the currently selected source
  pqPipelineSource *src = this->getActiveSource();
  if (!src)
  {
    QMessageBox::information(NULL, "Create Filter Warning",
        "Cannot create the target filter without an input source.");
    return;
  }

  // Handles case where user may have acciedently failed to update the pipeline prior to adding the new filter.
  src->updatePipeline();

  pqObjectBuilder* builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "RebinningCutter", src);
}

