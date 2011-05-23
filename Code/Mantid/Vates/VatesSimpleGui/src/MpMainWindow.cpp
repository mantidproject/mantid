#include "mpmainwindow.h"

#include "modecontrolwidget.h"
#include "multisliceview.h"
#include "standardview.h"
#include "threesliceview.h"
#include "timecontrolwidget.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqLoadDataReaction.h"
#include "pqObjectBuilder.h"
#include "pqObjectInspectorWidget.h"
#include "pqParaViewBehaviors.h"
#include "pqPipelineSource.h"
#include "pqRenderView.h"
#include "vtkSMProxyManager.h"
#include "vtkSMReaderFactory.h"

#include <QModelIndex>

#include <iostream>

mpMainWindow::mpMainWindow(QWidget *parent) : QMainWindow(parent)
{
  this->setupUi(this);

  // Unset the connections since the views aren't up yet.
  this->removeProxyTabWidgetConnections();

  new pqParaViewBehaviors(this, this);

  // We want the actionLoad to result in the showing up the ParaView's OpenData
  // dialog letting the user pick from one of the supported file formats.
  pqLoadDataReaction* dataLoader = new pqLoadDataReaction(this->action_Open);
  QObject::connect(dataLoader, SIGNAL(loadedData(pqPipelineSource*)),
    this, SLOT(onDataLoaded(pqPipelineSource*)));

  QObject::connect(this->modeControlWidget,
		  SIGNAL(executeSwitchViews(ModeControlWidget::Views)),
		  this, SLOT(switchViews(ModeControlWidget::Views)));

  //pqUndoReaction
  //QObject::connect()

  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the standard view as the default
  this->currentView = this->setMainViewWidget(this->viewWidget,
		  ModeControlWidget::STANDARD);
  this->setMainWindowComponentsForView();
}

mpMainWindow::~mpMainWindow()
{

}

void mpMainWindow::removeProxyTabWidgetConnections()
{
	QObject::disconnect(&pqActiveObjects::instance(), 0,
			this->proxyTabWidget, 0);
}

IView* mpMainWindow::setMainViewWidget(QWidget *container,
		ModeControlWidget::Views v)
{
	switch(v)
	{
	case ModeControlWidget::STANDARD:
	{
		return new StandardView(container);
	}
	break;
	case ModeControlWidget::THREESLICE:
	{
		return new ThreeSliceView(container);
	}
	break;
	case ModeControlWidget::MULTISLICE:
	{
		return new MultiSliceView(container);
	}
	default:
		return NULL;
	}
}

void mpMainWindow::setMainWindowComponentsForView()
{
	// Extra setup stuff to hook up view to other items
	this->proxyTabWidget->setupDefaultConnections();
	this->proxyTabWidget->setView(this->currentView->getView());
	this->proxyTabWidget->setShowOnAccept(true);
	this->pipelineBrowser->setActiveView(this->currentView->getView());
	if (this->currentView->inherits("MultiSliceView"))
	{
		QObject::connect(this->pipelineBrowser,
				SIGNAL(clicked(const QModelIndex &)),
				static_cast<MultiSliceView *>(this->currentView),
				SLOT(selectIndicator()));
		QObject::connect(this->proxyTabWidget->getObjectInspector(),
				SIGNAL(accepted()),
				static_cast<MultiSliceView *>(this->currentView),
				SLOT(updateSelectedIndicator()));
	}
}

void mpMainWindow::onDataLoaded(pqPipelineSource* source)
{
  if (this->originSource)
  {
	  pqApplicationCore::instance()->getObjectBuilder()->destroy(this->originSource);
  }
  this->originSource = source;

  this->currentView->render();
  this->proxyTabWidget->getObjectInspector()->accept();
  emit enableModeButtons();
}

void mpMainWindow::switchViews(ModeControlWidget::Views v)
{
	this->removeProxyTabWidgetConnections();
	this->hiddenView = this->setMainViewWidget(this->viewWidget, v);
	this->hiddenView->hide();
	this->swapViews();
	this->currentView->show();
	this->hiddenView->hide();
	this->setMainWindowComponentsForView();
	this->hiddenView->close();
	delete this->hiddenView;
	this->currentView->render();
	if (this->currentView->inherits("ThreeSliceView") ||
			this->currentView->inherits("StandardView"))
	{
		this->proxyTabWidget->getObjectInspector()->accept();
	}
}

void mpMainWindow::swapViews()
{
	IView *temp;
	temp = this->currentView;
	this->currentView = this->hiddenView;
	this->hiddenView = temp;
}
