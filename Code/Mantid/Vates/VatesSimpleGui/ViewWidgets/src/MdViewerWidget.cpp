#include "MantidVatesSimpleGuiViewWidgets/MdViewerWidget.h"

#include "MantidVatesSimpleGuiQtWidgets/ModeControlWidget.h"
#include "MantidVatesSimpleGuiViewWidgets/MultisliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/TimeControlWidget.h"

#include "MantidQtAPI/InterfaceManager.h"
#include "MantidKernel/DynamicFactory.h"

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
#include <QMainWindow>
#include <QModelIndex>

#include <iostream>

namespace Mantid
{
namespace Vates
{
namespace SimpleGui
{
REGISTER_VATESGUI(MdViewerWidget)

MdViewerWidget::MdViewerWidget(QWidget *parent) : QWidget(parent)
{
  this->ui.setupUi(this);
  this->ui.splitter_2->setStretchFactor(1, 1);

  // Unset the connections since the views aren't up yet.
  this->removeProxyTabWidgetConnections();

  // FIXME: This doesn't allow a clean split of the classes. I will need
  //        to investigate creating the individual behaviors to see if that
  //        eliminates the dependence on the QMainWindow.
  if (parent->inherits("QMainWindow"))
  {
    QMainWindow *mw = qobject_cast<QMainWindow *>(parent);
    new pqParaViewBehaviors(mw, mw);
  }

  QObject::connect(this->ui.modeControlWidget,
                   SIGNAL(executeSwitchViews(ModeControlWidget::Views)),
                   this, SLOT(switchViews(ModeControlWidget::Views)));

  // Commented this out to only use Mantid supplied readers
  // Initialize all readers available to ParaView. Now our application can load
  // all types of datasets supported by ParaView.
  //vtkSMProxyManager::GetProxyManager()->GetReaderFactory()->RegisterPrototypes("sources");

  // Set the standard view as the default
  this->currentView = this->setMainViewWidget(this->ui.viewWidget,
                                              ModeControlWidget::STANDARD);

  // Create a layout to manage the view properly
  this->viewLayout = new QHBoxLayout(this->ui.viewWidget);
  this->viewLayout->setMargin(0);
  this->viewLayout->setStretch(0, 1);
  this->viewLayout->addWidget(this->currentView);

  this->setParaViewComponentsForView();
}

MdViewerWidget::~MdViewerWidget()
{
}

void MdViewerWidget::connectLoadDataReaction(QAction *action)
{
  // We want the actionLoad to result in the showing up the ParaView's OpenData
  // dialog letting the user pick from one of the supported file formats.
  this->dataLoader = new pqLoadDataReaction(action);
  QObject::connect(this->dataLoader, SIGNAL(loadedData(pqPipelineSource*)),
                   this, SLOT(onDataLoaded(pqPipelineSource*)));
}

void MdViewerWidget::removeProxyTabWidgetConnections()
{
  QObject::disconnect(&pqActiveObjects::instance(), 0,
                      this->ui.proxyTabWidget, 0);
}

ViewBase* MdViewerWidget::setMainViewWidget(QWidget *container,
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

void MdViewerWidget::setParaViewComponentsForView()
{
  // Extra setup stuff to hook up view to other items
  this->ui.proxyTabWidget->setupDefaultConnections();
  this->ui.proxyTabWidget->setView(this->currentView->getView());
  this->ui.proxyTabWidget->setShowOnAccept(true);
  this->ui.pipelineBrowser->setActiveView(this->currentView->getView());
  if (this->currentView->inherits("MultiSliceView"))
  {
    QObject::connect(this->ui.pipelineBrowser,
                     SIGNAL(clicked(const QModelIndex &)),
                     static_cast<MultiSliceView *>(this->currentView),
                     SLOT(selectIndicator()));
    QObject::connect(this->ui.proxyTabWidget->getObjectInspector(),
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
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorMapChanged(const pqColorMapModel *)),
                   this->currentView,
                   SLOT(onColorMapChange(const pqColorMapModel *)));
  QObject::connect(this->ui.colorSelectionWidget,
                   SIGNAL(colorScaleChanged(double, double)),
                   this->currentView,
                   SLOT(onColorScaleChange(double, double)));
  QObject::connect(this->currentView, SIGNAL(dataRange(double, double)),
                   this->ui.colorSelectionWidget,
                   SLOT(setColorScaleRange(double, double)));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(autoScale()),
                   this->currentView, SLOT(onAutoScale()));
  QObject::connect(this->ui.colorSelectionWidget, SIGNAL(logScale(int)),
                   this->currentView, SLOT(onLogScale(int)));
}

void MdViewerWidget::onDataLoaded(pqPipelineSource* source)
{
  UNUSED_ARG(source);
  if (this->currentView->origSource)
  {
    pqApplicationCore::instance()->getObjectBuilder()->destroy(this->currentView->origSource);
  }
  //this->originSource = source;

  this->currentView->render();
  this->ui.proxyTabWidget->getObjectInspector()->accept();

  const unsigned int val = vtkSMPropertyHelper(\
        this->currentView->origSource->getProxy(),
        "InputGeometryXML", true).GetNumberOfElements();
  if (val > 0)
  {
    emit this->enableMultiSliceViewButton();
  }
  emit this->enableThreeSliceViewButton();
}

void MdViewerWidget::switchViews(ModeControlWidget::Views v)
{
  this->removeProxyTabWidgetConnections();
  this->hiddenView = this->setMainViewWidget(this->ui.viewWidget, v);
  this->hiddenView->hide();
  this->viewLayout->removeWidget(this->currentView);
  this->swapViews();
  this->viewLayout->addWidget(this->currentView);
  this->currentView->show();
  this->hiddenView->hide();
  this->setParaViewComponentsForView();
  this->hiddenView->close();
  this->hiddenView->destroyView();
  delete this->hiddenView;
  this->currentView->render();
  if (this->currentView->inherits("ThreeSliceView") ||
      this->currentView->inherits("StandardView"))
  {
    this->ui.proxyTabWidget->getObjectInspector()->accept();
  }
  if (this->currentView->inherits("ThreeSliceView"))
  {
    static_cast<ThreeSliceView *>(this->currentView)->correctVisibility(this->ui.pipelineBrowser);
  }
}

void MdViewerWidget::swapViews()
{
  ViewBase *temp;
  temp = this->currentView;
  this->currentView = this->hiddenView;
  this->hiddenView = temp;
}

}
}
}
