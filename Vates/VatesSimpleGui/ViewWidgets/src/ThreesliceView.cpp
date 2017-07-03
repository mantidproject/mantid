#include "MantidVatesSimpleGuiViewWidgets/ThreesliceView.h"
#include "MantidVatesSimpleGuiViewWidgets/LibHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include <Poco/Path.h>

#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqPluginManager.h>
#include <pqRenderView.h>
#include <pqScalarsToColors.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QMessageBox>

namespace Mantid {
namespace Vates {
namespace SimpleGui {
namespace {
/// Static logger
Kernel::Logger g_log("ThreeSliceView");
}

ThreeSliceView::ThreeSliceView(QWidget *parent,
                               RebinnedSourcesManager *rebinnedSourcesManager,
                               bool createRenderProxy)
    : ViewBase(parent, rebinnedSourcesManager), m_mainView(), m_ui() {
  this->m_ui.setupUi(this);

  if (createRenderProxy) {
    this->m_mainView = this->createRenderView(this->m_ui.mainRenderFrame,
                                              QString("OrthographicSliceView"));
    pqActiveObjects::instance().setActiveView(this->m_mainView);
  }
}

ThreeSliceView::~ThreeSliceView() {}

void ThreeSliceView::destroyView() {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  // Active source disappears in only this view, so set it from the
  // internal source before destroying view.
  pqActiveObjects::instance().setActiveSource(this->origSrc);
  builder->destroy(this->m_mainView);
}

pqRenderView *ThreeSliceView::getView() { return this->m_mainView.data(); }

void ThreeSliceView::render() {
  this->makeThreeSlice();
  this->resetDisplay();
  emit this->triggerAccept();
}

void ThreeSliceView::makeThreeSlice() {
  pqPipelineSource *src = pqActiveObjects::instance().activeSource();
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();

  // Do not allow overplotting PeaksWorkspaces
  if (this->isPeaksWorkspace(src)) {
    QMessageBox::warning(this, QApplication::tr("Overplotting Warning"),
                         QApplication::tr("Threeslice mode does not allow "
                                          "overlay of PeaksWorkspaces"));
    // Need to destroy source since we tried to load it and set the active
    // back to something. In this case we'll choose the original source
    builder->destroy(src);
    pqActiveObjects::instance().setActiveSource(this->origSrc);
    return;
  }

  this->origSrc = src;

  if (this->origSrc == nullptr) {
    return;
  }

  pqDataRepresentation *drep = builder->createDataRepresentation(
      this->origSrc->getOutputPort(0), this->m_mainView);
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set("Slices");
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation *>(drep);
}

void ThreeSliceView::renderAll() { this->m_mainView->render(); }

void ThreeSliceView::resetDisplay() { this->m_mainView->resetDisplay(); }

void ThreeSliceView::setView(pqRenderView *view) {
  clearRenderLayout(this->m_ui.mainRenderFrame);
  this->m_mainView = view;
  QHBoxLayout *hbox = new QHBoxLayout(this->m_ui.mainRenderFrame);
  hbox->setMargin(0);
  hbox->addWidget(m_mainView->widget());
}

ModeControlWidget::Views ThreeSliceView::getViewType() {
  return ModeControlWidget::Views::THREESLICE;
}

void ThreeSliceView::resetCamera() { this->m_mainView->resetCamera(); }
}
}
}
