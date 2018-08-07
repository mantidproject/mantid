#include "MantidVatesSimpleGuiViewWidgets/StandardView.h"
#include "MantidVatesSimpleGuiViewWidgets/RebinnedSourcesManager.h"
// Have to deal with ParaView warnings and Intel compiler the hard way.
#if defined(__INTEL_COMPILER)
#pragma warning disable 1170
#endif

#include <pqActiveObjects.h>
#include <pqApplicationCore.h>
#include <pqDataRepresentation.h>
#include <pqObjectBuilder.h>
#include <pqPipelineFilter.h>
#include <pqPipelineRepresentation.h>
#include <pqPipelineSource.h>
#include <pqRenderView.h>
#include <pqServer.h>
#include <pqServerManagerModel.h>
#include <vtkDataObject.h>
#include <vtkSMPVRepresentationProxy.h>
#include <vtkSMPropertyHelper.h>
#include <vtkSMProxy.h>

#if defined(__INTEL_COMPILER)
#pragma warning enable 1170
#endif

#include <QAction>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QMenu>
#include <QMessageBox>
#include <QString>
#include <QToolTip>

namespace Mantid {
namespace Vates {
namespace SimpleGui {

/**
 * Simple class for a QMenu where the actions do show their tool tip
 * strings (this does not happen by default with standard QMenu).
 */
class QMenuWithToolTip : public QMenu {
public:
  explicit QMenuWithToolTip(QWidget *parent) : QMenu(parent) {}

  bool event(QEvent *e) override {
    if (QEvent::ToolTip == e->type()) {
      // grab the action specific tooltip
      QHelpEvent *he = dynamic_cast<QHelpEvent *>(e);
      if (!he)
        return false;
      QAction *a = actionAt(he->pos());
      if (a && a->toolTip() != a->text()) {
        QToolTip::showText(he->globalPos(), a->toolTip(), this);
        return true;
      }
    }
    return QMenu::event(e);
  }
};

QString StandardView::g_binMDName = "BinMD";
QString StandardView::g_sliceMDName = "SliceMD";
QString StandardView::g_cutMDName = "CutMD";
// important: these label strings must use the name of the corresponding
// Mantid algorithm as first token (before first space), as it will
// be used as a parameter when emitting the signal rebin
QString StandardView::g_binMDLbl = "Fast (" + g_binMDName + ")";
QString StandardView::g_sliceMDLbl = "Complete (" + g_sliceMDName + ")";
QString StandardView::g_cutMDLbl = "Horace style (" + g_cutMDName + ")";

const QString tipBefore = "Run the ";
const QString tipAfter =
    " Mantid algorithm (the algorithm dialog will show up)";
QString StandardView::g_binMDToolTipTxt = tipBefore + g_binMDName + tipAfter;
QString StandardView::g_sliceMDToolTipTxt =
    tipBefore + g_sliceMDName + tipAfter;
QString StandardView::g_cutMDToolTipTxt = tipBefore + g_cutMDName + tipAfter;
const std::string StandardView::SurfaceRepresentation = "Surface";
const std::string StandardView::WireFrameRepresentation = "Wireframe";

// To map action labels to algorithm names
QMap<QString, QString> StandardView::g_actionToAlgName;

/**
 * This function sets up the UI components, adds connections for the view's
 * buttons and creates the rendering view.
 * @param parent the parent widget for the standard view
 * @param rebinnedSourcesManager Pointer to a RebinnedSourcesManager
 * @param createRenderProxy :: whether to create a render proxy for the view
 */
StandardView::StandardView(QWidget *parent,
                           RebinnedSourcesManager *rebinnedSourcesManager,
                           bool createRenderProxy)
    : ViewBase(parent, rebinnedSourcesManager), m_binMDAction(nullptr),
      m_sliceMDAction(nullptr), m_cutMDAction(nullptr), m_unbinAction(nullptr) {
  this->m_ui.setupUi(this);
  this->m_cameraReset = false;

  // before setting the button-actions, register their algorithms
  if (0 == g_actionToAlgName.size()) {
    g_actionToAlgName.insert(g_binMDLbl, g_binMDName);
    g_actionToAlgName.insert(g_sliceMDLbl, g_sliceMDName);
    g_actionToAlgName.insert(g_cutMDLbl, g_cutMDName);
  }

  // Set up the buttons
  setupViewButtons();

  // Set the cut button to create a slice on the data
  QObject::connect(this->m_ui.cutButton, SIGNAL(clicked()), this,
                   SLOT(onCutButtonClicked()));

  // Set the cut button to create a slice on the data
  QObject::connect(this->m_ui.thresholdButton, SIGNAL(clicked()), this,
                   SLOT(onThresholdButtonClicked()));

  // Listen to a change in the active source, to adapt our rebin buttons
  QObject::connect(&pqActiveObjects::instance(),
                   SIGNAL(sourceChanged(pqPipelineSource *)), this,
                   SLOT(activeSourceChangeListener(pqPipelineSource *)));

  // Set the scale button to create the ScaleWorkspace operator
  QObject::connect(this->m_ui.scaleButton, SIGNAL(clicked()), this,
                   SLOT(onScaleButtonClicked()));

  if (createRenderProxy) {
    this->m_view = this->createRenderView(this->m_ui.renderFrame);

    QObject::connect(this->m_view.data(), SIGNAL(endRender()), this,
                     SLOT(onRenderDone()));
  }
}

StandardView::~StandardView() {}

void StandardView::setupViewButtons() {

  // Populate the rebin button
  QMenuWithToolTip *rebinMenu =
      new QMenuWithToolTip(this->m_ui.rebinToolButton);

  m_binMDAction = new QAction(g_binMDLbl, rebinMenu);
  m_binMDAction->setToolTip(g_binMDToolTipTxt);
  m_binMDAction->setIconVisibleInMenu(false);

  m_sliceMDAction = new QAction(g_sliceMDLbl, rebinMenu);
  m_sliceMDAction->setToolTip(g_sliceMDToolTipTxt);
  m_sliceMDAction->setIconVisibleInMenu(false);

  m_cutMDAction = new QAction(g_cutMDLbl, rebinMenu);
  m_cutMDAction->setToolTip(g_cutMDToolTipTxt);
  m_cutMDAction->setIconVisibleInMenu(false);

  m_unbinAction = new QAction("Remove Rebinning", rebinMenu);
  m_unbinAction->setIconVisibleInMenu(false);

  rebinMenu->addAction(m_binMDAction);
  rebinMenu->addAction(m_sliceMDAction);
  rebinMenu->addAction(m_cutMDAction);
  rebinMenu->addAction(m_unbinAction);

  this->m_ui.rebinToolButton->setPopupMode(QToolButton::InstantPopup);
  this->m_ui.rebinToolButton->setMenu(rebinMenu);

  QObject::connect(m_binMDAction, SIGNAL(triggered()), this, SLOT(onRebin()),
                   Qt::QueuedConnection);
  QObject::connect(m_sliceMDAction, SIGNAL(triggered()), this, SLOT(onRebin()),
                   Qt::QueuedConnection);
  QObject::connect(m_cutMDAction, SIGNAL(triggered()), this, SLOT(onRebin()),
                   Qt::QueuedConnection);
  // Set the unbinbutton to remove the rebinning on a workspace
  // which was binned in the VSI
  QObject::connect(m_unbinAction, SIGNAL(triggered()), this, SIGNAL(unbin()),
                   Qt::QueuedConnection);
}

void StandardView::destroyView() {
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->destroyFilter(QString("Slice"));
  this->destroyFilter(QString("Threshold"));
  builder->destroy(this->m_view);
}

pqRenderView *StandardView::getView() { return this->m_view.data(); }

void StandardView::render() {
  this->origSrc = pqActiveObjects::instance().activeSource();
  if (!this->origSrc) {
    return;
  }
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  activeSourceChangeListener(this->origSrc);

  if (this->isPeaksWorkspace(this->origSrc)) {
    this->m_ui.cutButton->setEnabled(false);
  }

  // Show the data
  pqDataRepresentation *drep = builder->createDataRepresentation(
      this->origSrc->getOutputPort(0), this->m_view);
  std::string reptype = StandardView::SurfaceRepresentation;
  if (this->isPeaksWorkspace(this->origSrc)) {
    reptype = StandardView::WireFrameRepresentation;
  }
  vtkSMPropertyHelper(drep->getProxy(), "Representation").Set(reptype.c_str());
  drep->getProxy()->UpdateVTKObjects();
  this->origRep = qobject_cast<pqPipelineRepresentation *>(drep);
  if (!this->isPeaksWorkspace(this->origSrc)) {
    vtkSMPVRepresentationProxy::SetScalarColoring(
        drep->getProxy(), "signal", vtkDataObject::FIELD_ASSOCIATION_CELLS);
    drep->getProxy()->UpdateVTKObjects();
  }

  emit this->triggerAccept();
  this->resetDisplay();
}

void StandardView::onCutButtonClicked() {
  // check that has active source
  if (!hasActiveSource()) {
    return;
  }

  // Apply cut to currently viewed data
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Cut", this->getPvActiveSrc());

  // We need to attach the visibility listener to the newly
  // created filter, this is required for automatic updating the color scale
  setVisibilityListener();
}

void StandardView::onThresholdButtonClicked() {
  // check that has active source
  if (!hasActiveSource()) {
    return;
  }

  // Apply cut to currently viewed data
  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  builder->createFilter("filters", "Threshold", this->getPvActiveSrc());

  // We need to attach the visibility listener to the newly
  // created filter, this is required for automatic updating the color scale
  setVisibilityListener();
}

void StandardView::onScaleButtonClicked() {
  // check that has active source
  if (!hasActiveSource()) {
    return;
  }

  pqObjectBuilder *builder = pqApplicationCore::instance()->getObjectBuilder();
  this->m_scaler = builder->createFilter(
      "filters", "MantidParaViewScaleWorkspace", this->getPvActiveSrc());

  /*
   Paraview will try to set the respresentation to Outline. This is not good.
   Instead we listen for the
   represnetation added as a result of the filter completion, and change the
   representation to be
   Surface instead.
   */
  QObject::connect(this->m_scaler,
                   SIGNAL(representationAdded(pqPipelineSource *,
                                              pqDataRepresentation *, int)),
                   this,
                   SLOT(onScaleRepresentationAdded(
                       pqPipelineSource *, pqDataRepresentation *, int)));

  // We need to attach the visibility listener to the newly
  // created filter, this is required for automatic updating the color scale
  setVisibilityListener();
}

/**
 * This function is responsible for calling resetCamera if the internal
 * variable cameraReset has been set to true.
 */
void StandardView::onRenderDone() {
  if (this->m_cameraReset) {
    this->resetCamera();
    this->m_cameraReset = false;
  }
}

void StandardView::renderAll() { this->m_view->render(); }

void StandardView::resetDisplay() {
  this->m_view->resetDisplay();
  this->m_view->forceRender();
}

void StandardView::resetCamera() {
  this->m_view->resetCamera();
  this->m_view->forceRender();
}

/**
 * This function enables the cut button for the standard view.
 */
void StandardView::updateUI() { this->m_ui.cutButton->setEnabled(true); }

void StandardView::updateView() { this->m_cameraReset = true; }

void StandardView::closeSubWindows() {}

void StandardView::setView(pqRenderView *view) {
  clearRenderLayout(this->m_ui.renderFrame);

  this->m_view = view;

  QHBoxLayout *hbox = new QHBoxLayout(this->m_ui.renderFrame);
  hbox->setMargin(0);
  hbox->addWidget(m_view->widget());

  QObject::connect(this->m_view.data(), SIGNAL(endRender()), this,
                   SLOT(onRenderDone()));
}

ModeControlWidget::Views StandardView::getViewType() {
  return ModeControlWidget::Views::STANDARD;
}

/**
 * Check if the rebin and unbin buttons should be visible
 * Note that for a rebin button to be visible there may be no
 * MDHisto workspaces present, yet  MDHisto workspaces which result from
 * rebinning within the VSI are allowed.
 */
void StandardView::setRebinAndUnbinButtons() {
  unsigned int numberOfInternallyRebinnedWorkspaces = 0;
  unsigned int numberOfTrueMDHistoWorkspaces = 0;
  unsigned int numberOfPeakWorkspaces = 0;

  pqServer *server = pqActiveObjects::instance().activeServer();
  pqServerManagerModel *smModel =
      pqApplicationCore::instance()->getServerManagerModel();
  const QList<pqPipelineSource *> sources =
      smModel->findItems<pqPipelineSource *>(server);

  foreach (pqPipelineSource *source, sources) {
    if (isInternallyRebinnedWorkspace(source)) {
      ++numberOfInternallyRebinnedWorkspaces;
    } else if (isMDHistoWorkspace(source)) {
      ++numberOfTrueMDHistoWorkspaces;
    } else if (isPeaksWorkspace(source)) {
      ++numberOfPeakWorkspaces;
    }
  }

  // If there are any true MDHisto workspaces then the rebin button should be
  // disabled
  bool allowRebinning =
      numberOfTrueMDHistoWorkspaces > 0 || numberOfPeakWorkspaces > 0;
  this->allowRebinningOptions(allowRebinning);

  // If there are no internally rebinned workspaces the button should be
  // disabled.
  allowUnbinOption(numberOfInternallyRebinnedWorkspaces > 0);
}

/**
 * Reacts to the user selecting the BinMD algorithm
 */
void StandardView::onRebin() {
  if (QAction *action = dynamic_cast<QAction *>(sender())) {
    // split always returns a list of at least one element
    QString algName = getAlgNameFromMenuLabel(action->text());
    emit rebin(algName.toStdString());
  }
}

/**
 * react to the addition of the representation and change it's type to be
 * Surface
 * @param representation : representation to modify
 */
void StandardView::onScaleRepresentationAdded(
    pqPipelineSource *, pqDataRepresentation *representation, int) {
  vtkSMPropertyHelper(representation->getProxy(), "Representation")
      .Set(StandardView::SurfaceRepresentation.c_str());
}

/**
Disable rebinning options
*/
void StandardView::allowRebinningOptions(bool allow) {
  this->m_binMDAction->setEnabled(allow);
  this->m_sliceMDAction->setEnabled(allow);
  this->m_cutMDAction->setEnabled(allow);
}

/**
Enable unbin option
*/
void StandardView::allowUnbinOption(bool allow) {
  this->m_unbinAction->setEnabled(allow);
}

/**
 * Listen for a change of the active source in order to check if the
 * active source is an MDEventSource for which we allow rebinning.
 */
void StandardView::activeSourceChangeListener(pqPipelineSource *source) {
  // If there is no active source, then we do not allow rebinning
  if (!source) {
    this->allowRebinningOptions(false);
    this->m_unbinAction->setEnabled(false);
    return;
  }

  // If it is a filter work your way down
  pqPipelineSource *localSource = source;
  pqPipelineFilter *filter = qobject_cast<pqPipelineFilter *>(localSource);

  while (filter) {
    localSource = filter->getInput(0);
    filter = qobject_cast<pqPipelineFilter *>(localSource);
  }

  // Important to first check for an internally rebinned source, then for
  // MDEvent source,
  // since the internally rebinned source may be an MDEventSource.
  std::string workspaceType(localSource->getProxy()->GetXMLName());

  // Check if the source is associated with a workspace which was internally
  // rebinned by the VSI.
  // In this case the user can further rebin or unbin the source.
  if (isInternallyRebinnedWorkspace(localSource)) {
    this->allowRebinningOptions(true);
    this->allowUnbinOption(true);
  }
  // Check if we are dealing with a MDEvent workspace. In this case we allow
  // rebinning, but
  // unbinning will not make a lot of sense.
  else if (workspaceType.find("MDEW Source") != std::string::npos) {
    this->allowRebinningOptions(true);
    this->allowUnbinOption(false);
  }
  // Otherwise we must be dealing with either a MDHIsto or PeaksWorkspace
  // which cannot be neither rebinned nor unbinned.
  else {
    this->allowRebinningOptions(false);
    this->allowUnbinOption(false);
  }
}

/**
 * Helper for the rebinning menu. For example it will give you the
 * name of the algorithm CutMD ("CutMD" == g_cutMDName) if you pass
 * its label in the rebinning menu (g_cutMDToolTipTxt).
 *
 * @param menuLbl label (what is shown in the action)
 * @return Name of the corresponding Mantid algorithm
 */
QString StandardView::getAlgNameFromMenuLabel(const QString &menuLbl) {
  QString res;
  if (g_actionToAlgName.contains(menuLbl)) {
    res = g_actionToAlgName.value(menuLbl);
  } else {
    // ideally an informative error would be given here but there doesn't seem
    // to be
    // a convnient way to do that in these view classes
  }
  return res;
}

} // namespace SimpleGui
} // namespace Vates
} // namespace Mantid
