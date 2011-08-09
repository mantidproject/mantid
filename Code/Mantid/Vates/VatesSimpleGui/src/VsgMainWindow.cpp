#include "VsgMainWindow.h"

#include "ModeControlWidget.h"
#include "MultisliceView.h"
#include "StandardView.h"
#include "ThreesliceView.h"
#include "TimeControlWidget.h"

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqLoadDataReaction.h>
#include <pqObjectBuilder.h>
#include <pqObjectInspectorWidget.h>
#include <pqParaViewBehaviors.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxyManager.h>
#include <vtkSMReaderFactory.h>

#include <QHBoxLayout>
#include <QModelIndex>

#include <iostream>

VsgMainWindow::VsgMainWindow(QWidget *parent) : QMainWindow(parent)
{
  this->setupUi(this);
  this->splitter_2->setStretchFactor(1, 1);

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

  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the standard view as the default
  this->currentView = this->setMainViewWidget(this->viewWidget,
		  ModeControlWidget::STANDARD);

  // Create a layout to manage the view properly
  this->viewLayout = new QHBoxLayout(this->viewWidget);
  this->viewLayout->setMargin(0);
  this->viewLayout->setStretch(0, 1);
  this->viewLayout->addWidget(this->currentView);

  this->setMainWindowComponentsForView();
}

VsgMainWindow::~VsgMainWindow()
{

}

void VsgMainWindow::removeProxyTabWidgetConnections()
{
	QObject::disconnect(&pqActiveObjects::instance(), 0,
			this->proxyTabWidget, 0);
}

ViewBase* VsgMainWindow::setMainViewWidget(QWidget *container,
		ModeControlWidget::Views v)
{
  ViewBase *view;
	switch(v)
	{
	case ModeControlWidget::STANDARD:
	{
		view = new StandardView(container);
	}
	break;
	case ModeControlWidget::THREESLICE:
	{
		view = new ThreeSliceView(container);
	}
	break;
	case ModeControlWidget::MULTISLICE:
	{
		view = new MultiSliceView(container);
	}
	break;
	default:
		view = NULL;
		break;
	}
	return view;
}

void VsgMainWindow::setMainWindowComponentsForView()
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
  if (this->currentView->inherits("StandardView"))
  {
    QObject::connect(static_cast<StandardView *>(this->currentView),
    SIGNAL(enableMultiSliceViewButton()),
    this, SIGNAL(enableMultiSliceViewButton()));
  }
  QObject::connect(this->colorSelectionWidget, SIGNAL(colorMapChanged(const pqColorMapModel *)),
    this->currentView, SLOT(onColorMapChange(const pqColorMapModel *)));
  QObject::connect(this->colorSelectionWidget, SIGNAL(colorScaleChanged(double, double)),
    this->currentView, SLOT(onColorScaleChange(double, double)));
  QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
    this->colorSelectionWidget, SLOT(setColorScaleRange(double, double)));
  QObject::connect(this->colorSelectionWidget, SIGNAL(autoScale()),
    this->currentView, SLOT(onAutoScale()));
  QObject::connect(this->colorSelectionWidget, SIGNAL(logScale(int)),
                   this->currentView, SLOT(onLogScale(int)));
}

void VsgMainWindow::onDataLoaded(pqPipelineSource* source)
{
  if (this->originSource)
  {
	  pqApplicationCore::instance()->getObjectBuilder()->destroy(this->originSource);
  }
  this->originSource = source;

  this->currentView->render();
  this->proxyTabWidget->getObjectInspector()->accept();

  const unsigned int val = vtkSMPropertyHelper(this->originSource->getProxy(),
      "InputGeometryXML", true).GetNumberOfElements();
  if (val > 0)
  {
    emit this->enableMultiSliceViewButton();
  }
  emit this->enableThreeSliceViewButton();
}

void VsgMainWindow::switchViews(ModeControlWidget::Views v)
{
	this->removeProxyTabWidgetConnections();
	this->hiddenView = this->setMainViewWidget(this->viewWidget, v);
	this->hiddenView->hide();
  this->viewLayout->removeWidget(this->currentView);
	this->swapViews();
  this->viewLayout->addWidget(this->currentView);
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
  if (this->currentView->inherits("ThreeSliceView"))
  {
    static_cast<ThreeSliceView *>(this->currentView)->correctVisibility(this->pipelineBrowser);
  }
}

void VsgMainWindow::swapViews()
{
	ViewBase *temp;
	temp = this->currentView;
	this->currentView = this->hiddenView;
	this->hiddenView = temp;
}
