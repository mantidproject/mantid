#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#include "MantidQtWidgets/InstrumentView/CollapsiblePanel.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/MiniPlot.h"
#include "MantidQtWidgets/InstrumentView/MiniPlotController.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"
#include "MantidQtWidgets/Common/TSVSerialiser.h"

#include "MantidKernel/Unit.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Crystal/OrientedLattice.h"
#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/V3D.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QTextEdit>
#include <QMenu>
#include <QAction>
#include <QActionGroup>
#include <QLabel>
#include <QMessageBox>
#include <QGridLayout>
#include <QSignalMapper>
#include <QPixmap>
#include <QSettings>
#include <QApplication>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <vector>

#include <boost/math/constants/constants.hpp>

namespace MantidQt {
namespace MantidWidgets {

using namespace boost::math;

/**
* Constructor.
* @param instrWidget :: Parent InstrumentWidget.
*/
InstrumentWidgetPickTab::InstrumentWidgetPickTab(InstrumentWidget *instrWidget)
    : InstrumentWidgetTab(instrWidget), m_freezePlot(false) {
  QVBoxLayout *layout = new QVBoxLayout(this);

  // connect to InstrumentWindow signals
  connect(m_instrWidget, SIGNAL(integrationRangeChanged(double, double)), this,
          SLOT(changedIntegrationRange(double, double)));

  // set up the selection display
  m_selectionInfoDisplay = new QTextEdit(this);

  // set up the plot widget
  m_miniplot = new MiniPlot(this);
  m_miniplotController = new MiniPlotController(m_instrWidget, m_miniplot);

  // Instrument display context menu actions
  m_storeCurve = new QAction("Store curve", this);
  connect(m_storeCurve, SIGNAL(triggered()), m_miniplot, SLOT(storeCurve()));
  m_savePlotToWorkspace = new QAction("Save plot to workspace", this);
  connect(m_savePlotToWorkspace, SIGNAL(triggered()), this,
          SLOT(savePlotToWorkspace()));

  CollapsibleStack *panelStack = new CollapsibleStack(this);
  m_infoPanel = panelStack->addPanel("Selection", m_selectionInfoDisplay);
  m_plotPanel = panelStack->addPanel("Name", m_miniplot);
  connect(m_miniplotController, SIGNAL(plotTypeChanged(QString)), m_plotPanel,
          SLOT(setCaption(const QString &)));

  m_selectionType = Single;

  m_infoController = nullptr;

  m_activeTool = new QLabel(this);
  // set up the tool bar
  m_zoom = new QPushButton();
  m_zoom->setCheckable(true);
  m_zoom->setAutoExclusive(true);
  m_zoom->setIcon(QIcon(":/PickTools/zoom.png"));
  m_zoom->setToolTip("Zoom in and out");

  m_one = new QPushButton();
  m_one->setCheckable(true);
  m_one->setAutoExclusive(true);
  m_one->setChecked(true);
  m_one->setToolTip("Select single pixel");
  m_one->setIcon(QIcon(":/PickTools/selection-pointer.png"));

  m_tube = new QPushButton();
  m_tube->setCheckable(true);
  m_tube->setAutoExclusive(true);
  m_tube->setIcon(QIcon(":/PickTools/selection-tube.png"));
  m_tube->setToolTip("Select whole tube");

  m_rectangle = new QPushButton();
  m_rectangle->setCheckable(true);
  m_rectangle->setAutoExclusive(true);
  m_rectangle->setIcon(QIcon(":/PickTools/selection-box.png"));
  m_rectangle->setToolTip("Draw a rectangle");

  m_ellipse = new QPushButton();
  m_ellipse->setCheckable(true);
  m_ellipse->setAutoExclusive(true);
  m_ellipse->setIcon(QIcon(":/PickTools/selection-circle.png"));
  m_ellipse->setToolTip("Draw a ellipse");

  m_ring_ellipse = new QPushButton();
  m_ring_ellipse->setCheckable(true);
  m_ring_ellipse->setAutoExclusive(true);
  m_ring_ellipse->setIcon(QIcon(":/PickTools/selection-circle-ring.png"));
  m_ring_ellipse->setToolTip("Draw an elliptical ring");

  m_ring_rectangle = new QPushButton();
  m_ring_rectangle->setCheckable(true);
  m_ring_rectangle->setAutoExclusive(true);
  m_ring_rectangle->setIcon(QIcon(":/PickTools/selection-box-ring.png"));
  m_ring_rectangle->setToolTip("Draw a rectangular ring");

  m_free_draw = new QPushButton();
  m_free_draw->setCheckable(true);
  m_free_draw->setAutoExclusive(true);
  m_free_draw->setIcon(QIcon(":/PickTools/brush.png"));
  m_free_draw->setToolTip("Draw an arbitrary shape");

  m_edit = new QPushButton();
  m_edit->setCheckable(true);
  m_edit->setAutoExclusive(true);
  m_edit->setIcon(QIcon(":/PickTools/selection-edit.png"));
  m_edit->setToolTip("Edit a shape");

  m_peak = new QPushButton();
  m_peak->setCheckable(true);
  m_peak->setAutoExclusive(true);
  m_peak->setIcon(QIcon(":/PickTools/selection-peak.png"));
  m_peak->setToolTip("Add single crystal peak");

  m_peakSelect = new QPushButton();
  m_peakSelect->setCheckable(true);
  m_peakSelect->setAutoExclusive(true);
  m_peakSelect->setIcon(QIcon(":/PickTools/eraser.png"));
  m_peakSelect->setToolTip("Erase single crystal peak(s)");

  m_peakCompare = new QPushButton();
  m_peakCompare->setCheckable(true);
  m_peakCompare->setAutoExclusive(true);
  m_peakCompare->setIcon(QIcon(":/PickTools/selection-peak-compare.png"));
  m_peakCompare->setToolTip("Compare single crystal peak(s)");

  m_peakAlign = new QPushButton();
  m_peakAlign->setCheckable(true);
  m_peakAlign->setAutoExclusive(true);
  m_peakAlign->setIcon(QIcon(":/PickTools/selection-peak-plane.png"));
  m_peakAlign->setToolTip("Crystal peak alignment tool");

  QGridLayout *toolBox = new QGridLayout();
  toolBox->addWidget(m_zoom, 0, 0);
  toolBox->addWidget(m_edit, 0, 1);
  toolBox->addWidget(m_ellipse, 0, 2);
  toolBox->addWidget(m_rectangle, 0, 3);
  toolBox->addWidget(m_ring_ellipse, 0, 4);
  toolBox->addWidget(m_ring_rectangle, 0, 5);
  toolBox->addWidget(m_free_draw, 0, 6);
  toolBox->addWidget(m_one, 1, 0);
  toolBox->addWidget(m_tube, 1, 1);
  toolBox->addWidget(m_peak, 1, 2);
  toolBox->addWidget(m_peakSelect, 1, 3);
  toolBox->addWidget(m_peakCompare, 1, 4);
  toolBox->addWidget(m_peakAlign, 1, 5);
  toolBox->setColumnStretch(6, 1);
  toolBox->setSpacing(2);
  connect(m_zoom, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_one, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_tube, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_peak, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_peakSelect, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_peakCompare, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_peakAlign, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_rectangle, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_ellipse, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_ring_ellipse, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_ring_rectangle, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_free_draw, SIGNAL(clicked()), this, SLOT(setSelectionType()));
  connect(m_edit, SIGNAL(clicked()), this, SLOT(setSelectionType()));

  // lay out the widgets
  layout->addWidget(m_activeTool);
  layout->addLayout(toolBox);
  layout->addWidget(panelStack);
}

/**
* Returns true if the plot can be updated when the mouse moves over detectors
*/
bool InstrumentWidgetPickTab::canUpdateTouchedDetector() const {
  return !m_peak->isChecked();
}

/**
* Update the plot caption. The captions shows the selection type.
*/
void InstrumentWidgetPickTab::setPlotCaption() {
  m_plotPanel->setCaption(m_miniplotController->getPlotCaption());
}

/**
* Set the selection type according to which tool button is checked.
*/
void InstrumentWidgetPickTab::setSelectionType() {
  ProjectionSurface::InteractionMode surfaceMode =
      ProjectionSurface::PickSingleMode;
  auto plotType = m_miniplotController->getPlotType();
  if (m_zoom->isChecked()) {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Navigation");
    surfaceMode = ProjectionSurface::MoveMode;
  } else if (m_one->isChecked()) {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Pixel selection");
    surfaceMode = ProjectionSurface::PickSingleMode;
    plotType = MiniPlotController::PlotType::Single;
  } else if (m_tube->isChecked()) {
    m_selectionType = Tube;
    m_activeTool->setText("Tool: Tube/bank selection");
    surfaceMode = ProjectionSurface::PickTubeMode;
    if (plotType < MiniPlotController::PlotType::TubeSum) {
      plotType = MiniPlotController::PlotType::TubeSum;
    }
  } else if (m_peak->isChecked()) {
    m_selectionType = AddPeak;
    m_activeTool->setText("Tool: Add a single crystal peak");
    surfaceMode = ProjectionSurface::AddPeakMode;
    plotType = MiniPlotController::PlotType::Single;
  } else if (m_peakSelect->isChecked()) {
    m_selectionType = ErasePeak;
    m_activeTool->setText("Tool: Erase crystal peak(s)");
    surfaceMode = ProjectionSurface::ErasePeakMode;
  } else if (m_peakCompare->isChecked()) {
    m_selectionType = ComparePeak;
    m_activeTool->setText("Tool: Compare crystal peak(s)");
    surfaceMode = ProjectionSurface::ComparePeakMode;
  } else if (m_peakAlign->isChecked()) {
    m_selectionType = AlignPeak;
    m_activeTool->setText("Tool: Align crystal peak(s)");
    surfaceMode = ProjectionSurface::AlignPeakMode;
  } else if (m_rectangle->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangle");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = MiniPlotController::PlotType::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "rectangle", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ellipse->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Ellipse");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = MiniPlotController::PlotType::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ellipse", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ring_ellipse->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Elliptical ring");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = MiniPlotController::PlotType::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ring ellipse", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ring_rectangle->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangular ring");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = MiniPlotController::PlotType::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ring rectangle", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_free_draw->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Arbitrary shape");
    surfaceMode = ProjectionSurface::DrawFreeMode;
    plotType = MiniPlotController::PlotType::Single;
    m_instrWidget->getSurface()->startCreatingFreeShape(
        Qt::green, QColor(255, 255, 255, 80));
  } else if (m_edit->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Shape editing");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = MiniPlotController::PlotType::Single;
  }
  m_miniplotController->setPlotType(plotType);
  auto surface = m_instrWidget->getSurface();
  if (surface) {
    surface->setInteractionMode(surfaceMode);
    auto interactionMode = surface->getInteractionMode();
    if (interactionMode == ProjectionSurface::DrawRegularMode ||
        interactionMode == ProjectionSurface::MoveMode) {
      updatePlotMultipleDetectors();
    } else {
      m_miniplot->removeActiveCurve();
    }
    setPlotCaption();
  }
  m_instrWidget->updateInfoText();
}

/**
* Respond to the show event.
*/
void InstrumentWidgetPickTab::showEvent(QShowEvent *) {
  // Make the state of the display view consistent with the current selection
  // type
  setSelectionType();
  // make sure picking updated
  m_instrWidget->updateInstrumentView(true);
  m_instrWidget->getSurface()->changeBorderColor(getShapeBorderColor());
}

/**
* Get the color of the overlay shapes in this tab.
* @return
*/
QColor InstrumentWidgetPickTab::getShapeBorderColor() const {
  return QColor(Qt::green);
}

/**
* Do something when the time bin integraion range has changed.
*/
void InstrumentWidgetPickTab::changedIntegrationRange(double, double) {
  m_miniplotController->updatePlot();
  auto surface = m_instrWidget->getSurface();
  if (surface) {
    auto interactionMode = surface->getInteractionMode();
    if (interactionMode == ProjectionSurface::DrawRegularMode ||
        interactionMode == ProjectionSurface::MoveMode) {
      updatePlotMultipleDetectors();
    }
  }
}

void InstrumentWidgetPickTab::initSurface() {
  ProjectionSurface *surface = getSurface().get();
  connect(surface, SIGNAL(singleComponentTouched(size_t)), this,
          SLOT(singleComponentTouched(size_t)));
  connect(surface, SIGNAL(singleComponentPicked(size_t)), this,
          SLOT(singleComponentPicked(size_t)));
  connect(
      surface, SIGNAL(comparePeaks(
                   const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                                   std::vector<Mantid::Geometry::IPeak *>> &)),
      this, SLOT(comparePeaks(
                const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                                std::vector<Mantid::Geometry::IPeak *>> &)));
  connect(surface, SIGNAL(alignPeaks(const std::vector<Mantid::Kernel::V3D> &,
                                     const Mantid::Geometry::IPeak *)),
          this, SLOT(alignPeaks(const std::vector<Mantid::Kernel::V3D>,
                                const Mantid::Geometry::IPeak *)));
  connect(surface, SIGNAL(peaksWorkspaceAdded()), this,
          SLOT(updateSelectionInfoDisplay()));
  connect(surface, SIGNAL(peaksWorkspaceDeleted()), this,
          SLOT(updateSelectionInfoDisplay()));
  connect(surface, SIGNAL(shapeCreated()), this, SLOT(shapeCreated()));
  connect(surface, SIGNAL(shapeChangeFinished()), this,
          SLOT(updatePlotMultipleDetectors()));
  connect(surface, SIGNAL(shapesCleared()), this,
          SLOT(updatePlotMultipleDetectors()));
  connect(surface, SIGNAL(shapesRemoved()), this,
          SLOT(updatePlotMultipleDetectors()));
  Projection3D *p3d = dynamic_cast<Projection3D *>(surface);
  if (p3d) {
    connect(p3d, SIGNAL(finishedMove()), this,
            SLOT(updatePlotMultipleDetectors()));
  }
  m_infoController =
      new ComponentInfoController(this, m_instrWidget, m_selectionInfoDisplay);
}

/**
* Return current ProjectionSurface.
*/
boost::shared_ptr<ProjectionSurface>
InstrumentWidgetPickTab::getSurface() const {
  return m_instrWidget->getSurface();
}

const InstrumentWidget *InstrumentWidgetPickTab::getInstrumentWidget() const {
  return m_instrWidget;
}

/**
* Save tab's persistent settings to the provided QSettings instance
*/
void InstrumentWidgetPickTab::saveSettings(QSettings &settings) const {
  m_miniplotController->saveSettings(settings);
}

/**
* Restore (read and apply) tab's persistent settings from the provided QSettings
* instance
*/
void InstrumentWidgetPickTab::loadSettings(const QSettings &settings) {
  m_miniplotController->loadSettings(settings);
}

/**
* Fill in the context menu.
* @param context A menu to fill
* @return True if items were added, false otherwise
*/
bool InstrumentWidgetPickTab::addToDisplayContextMenu(QMenu &context) const {
  m_freezePlot = true;
  bool res = false;
  if (m_miniplot->hasActiveCurve()) {
    context.addAction(m_storeCurve);
    res = true;
  }
  if (m_miniplot->hasStoredCurves() || m_miniplot->hasActiveCurve()) {
    context.addAction(m_savePlotToWorkspace);
    res = true;
  }
  return res;
}

/**
* Select a tool on the tab
* @param tool One of the enumerated tool types, @see ToolType
*/
void InstrumentWidgetPickTab::selectTool(const ToolType tool) {
  switch (tool) {
  case Zoom:
    m_zoom->setChecked(true);
    break;
  case PixelSelect:
    m_one->setChecked(true);
    break;
  case TubeSelect:
    m_tube->setChecked(true);
    break;
  case PeakSelect:
    m_peak->setChecked(true);
    break;
  case PeakCompare:
    m_peakCompare->setChecked(true);
  case PeakErase:
    m_peakSelect->setChecked(true);
    break;
  case DrawRectangle:
    m_rectangle->setChecked(true);
    break;
  case DrawEllipse:
    m_ellipse->setChecked(true);
    break;
  case DrawFree:
    m_free_draw->setChecked(true);
    break;
  case EditShape:
    m_edit->setChecked(true);
    break;
  default:
    throw std::invalid_argument("Invalid tool type.");
  }
  setSelectionType();
}

void InstrumentWidgetPickTab::singleComponentTouched(size_t pickID) {
  if (canUpdateTouchedDetector()) {
    m_infoController->displayInfo(pickID);
    m_miniplotController->setPlotData(pickID);
    m_miniplotController->updatePlot();
  }
}

void InstrumentWidgetPickTab::singleComponentPicked(size_t pickID) {
  m_infoController->displayInfo(pickID);
  m_miniplotController->setPlotData(pickID);
  m_miniplotController->updatePlot();
}

void InstrumentWidgetPickTab::comparePeaks(
    const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                    std::vector<Mantid::Geometry::IPeak *>> &peaks) {
  m_infoController->displayComparePeaksInfo(peaks);
}

void InstrumentWidgetPickTab::alignPeaks(
    const std::vector<Mantid::Kernel::V3D> &planePeaks,
    const Mantid::Geometry::IPeak *peak) {
  m_infoController->displayAlignPeaksInfo(planePeaks, peak);
}

/**
* Update the selection display using currently selected detector.
* Updates non-detector information on it.
*/
void InstrumentWidgetPickTab::updateSelectionInfoDisplay() {
  // updateSelectionInfo(m_currentDetID);
}

/**
* Respond to the shapeCreated signal from the surface.
*/
void InstrumentWidgetPickTab::shapeCreated() {
  if (!isVisible())
    return;
  if (!m_free_draw->isChecked()) {
    selectTool(EditShape);
  }
}

/**
* Update the mini-plot with information from multiple detector
* selected with drawn shapes.
*/
void InstrumentWidgetPickTab::updatePlotMultipleDetectors() {
  if (!isVisible())
    return;
  ProjectionSurface &surface = *getSurface();
  if (surface.hasMasks()) {
    std::vector<size_t> dets;
    surface.getMaskedDetectors(dets);
    m_miniplotController->setPlotData(dets);
  } else {
    m_miniplotController->clear();
  }
  m_miniplotController->updatePlot();
}

/**
* Save data plotted on the miniplot into a MatrixWorkspace.
*/
void InstrumentWidgetPickTab::savePlotToWorkspace() {
  m_miniplotController->savePlotToWorkspace();
}

/** Load pick tab state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void InstrumentWidgetPickTab::loadFromProject(const std::string &lines) {
  API::TSVSerialiser tsv(lines);

  if (!tsv.selectSection("picktab"))
    return;

  std::string tabLines;
  tsv >> tabLines;
  API::TSVSerialiser tab(tabLines);

  // load active push button
  std::vector<QPushButton *> buttons{
      m_zoom, m_edit, m_ellipse, m_rectangle, m_ring_ellipse, m_ring_rectangle,
      m_free_draw, m_one, m_tube, m_peak, m_peakSelect};

  tab.selectLine("ActiveTools");
  for (auto button : buttons) {
    bool value;
    tab >> value;
    button->setChecked(value);
  }
}

/** Save the state of the pick tab to a Mantid project file
 * @return a string representing the state of the pick tab
 */
std::string InstrumentWidgetPickTab::saveToProject() const {
  API::TSVSerialiser tsv, tab;

  // save active push button
  std::vector<QPushButton *> buttons{
      m_zoom, m_edit, m_ellipse, m_rectangle, m_ring_ellipse, m_ring_rectangle,
      m_free_draw, m_one, m_tube, m_peak, m_peakSelect};

  tab.writeLine("ActiveTools");
  for (auto button : buttons) {
    tab << button->isChecked();
  }

  tsv.writeSection("picktab", tab.outputLines());
  return tsv.outputLines();
}

//=====================================================================================//

/**
* Create and setup iteself.
* @param tab :: QObject parent - the tab.
* @param instrWidget :: A pointer to an InstrumentWidget instance.
* @param infoDisplay :: Widget on which to display the information.
*/
ComponentInfoController::ComponentInfoController(
    InstrumentWidgetPickTab *tab, const InstrumentWidget *instrWidget,
    QTextEdit *infoDisplay)
    : QObject(tab), m_tab(tab), m_instrWidget(instrWidget),
      m_selectionInfoDisplay(infoDisplay), m_freezePlot(false),
      m_instrWidgetBlocked(false),
      m_currentPickID(std::numeric_limits<size_t>::max()) {}

/**
* Display info on a component refered to by a pick ID.
* @param pickID :: A pick ID of a component.
*/
void ComponentInfoController::displayInfo(size_t pickID) {
  if (m_freezePlot) { // freeze the plot for one update
    m_freezePlot = false;
    pickID = m_currentPickID;
  }

  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  QString text = "";
  if (componentInfo.isDetector(pickID)) {
    text += displayDetectorInfo(pickID);
  } else if (auto componentID = actor.getComponentID(pickID)) {
    text += displayNonDetectorInfo(componentID);
  } else {
    clear();
  }
  // display info about peak overlays
  text += getPeakOverlayInfo();

  if (!text.isEmpty()) {
    m_selectionInfoDisplay->setText(text);
  } else {
    clear();
  }
}

/**
* Return string with info on a detector.
* @param index :: A detector Index.
*/
QString ComponentInfoController::displayDetectorInfo(size_t index) {
  if (m_instrWidgetBlocked) {
    clear();
    return "";
  }

  QString text;

  // collect info about selected detector and add it to text
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  auto detid = actor.getDetID(index);

  text = "Selected detector: " +
         QString::fromStdString(componentInfo.name(index)) + "\n";
  text += "Detector ID: " + QString::number(detid) + '\n';
  QString wsIndex;
  auto ws = actor.getWorkspaceIndex(index);
  wsIndex = ws == InstrumentActor::INVALID_INDEX ? "None" : QString::number(ws);
  text += "Workspace index: " + wsIndex + '\n';
  Mantid::Kernel::V3D pos = componentInfo.position(index);
  text += "xyz: " + QString::number(pos.X()) + "," + QString::number(pos.Y()) +
          "," + QString::number(pos.Z()) + '\n';
  double r, t, p;
  pos.getSpherical(r, t, p);
  text += "rtp: " + QString::number(r) + "," + QString::number(t) + "," +
          QString::number(p) + '\n';
  if (componentInfo.hasParent(index)) {
    QString textpath;
    auto parent = index;
    while (componentInfo.hasParent(parent)) {
      parent = componentInfo.parent(parent);
      textpath =
          "/" + QString::fromStdString(componentInfo.name(parent)) + textpath;
    }
    text += "Component path:" + textpath + "/" +
            QString::fromStdString(componentInfo.name(index)) + '\n';

    const double integrated = actor.getIntegratedCounts(index);
    const QString counts =
        integrated == -1.0 ? "N/A" : QString::number(integrated);
    text += "Counts: " + counts + '\n';
    // display info about peak overlays
    text += actor.getParameterInfo(index);
  }
  return text;
}

/**
* string with information about a selected non-detector component (such as
* choppers, etc).
* @param compID :: A component ID for a component to display.
*/
QString ComponentInfoController::displayNonDetectorInfo(
    Mantid::Geometry::ComponentID compID) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  auto component = componentInfo.indexOf(compID);
  QString text = "Selected component: ";
  text += QString::fromStdString(componentInfo.name(component)) + '\n';
  Mantid::Kernel::V3D pos = componentInfo.position(component);
  text += "xyz: " + QString::number(pos.X()) + "," + QString::number(pos.Y()) +
          "," + QString::number(pos.Z()) + '\n';
  double r, t, p;
  pos.getSpherical(r, t, p);
  text += "rtp: " + QString::number(r) + "," + QString::number(t) + "," +
          QString::number(p) + '\n';
  text += actor.getParameterInfo(component);
  return text;
}

QString
ComponentInfoController::displayPeakInfo(Mantid::Geometry::IPeak *peak) {
  std::stringstream text;
  auto instrument = peak->getInstrument();
  auto sample = instrument->getSample()->getPos();
  auto source = instrument->getSource()->getPos();
  auto L1 = (sample - source);

  auto detector = peak->getDetector();
  auto twoTheta = detector->getTwoTheta(sample, L1) * double_constants::radian;
  auto d = peak->getDSpacing();

  text << "d: " << d << "\n";
  text << "2 Theta: " << twoTheta << "\n";
  text << "Theta: " << (twoTheta / 2.) << "\n";
  text << "\n";

  return QString::fromStdString(text.str());
}

QString ComponentInfoController::displayPeakAngles(const std::pair<
    Mantid::Geometry::IPeak *, Mantid::Geometry::IPeak *> &peaks) {
  std::stringstream text;
  auto peak1 = peaks.first;
  auto peak2 = peaks.second;

  auto pos1 = peak1->getQSampleFrame();
  auto pos2 = peak2->getQSampleFrame();
  auto angle = pos1.angle(pos2);
  angle *= double_constants::radian;

  text << "Angle: " << angle << "\n";

  return QString::fromStdString(text.str());
}

void ComponentInfoController::displayComparePeaksInfo(
    const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                    std::vector<Mantid::Geometry::IPeak *>> &peaks) {
  std::stringstream text;

  text << "Comparison Information\n";
  auto peaksFromDetectors =
      std::make_pair(peaks.first.front(), peaks.second.front());
  text << displayPeakAngles(peaksFromDetectors).toStdString();

  text << "-------------------------------\n";
  text << "First Detector Peaks \n";
  for (auto peak : peaks.first)
    text << displayPeakInfo(peak).toStdString();

  text << "-------------------------------\n";
  text << "Second Detector Peaks \n";
  for (auto peak : peaks.second)
    text << displayPeakInfo(peak).toStdString();

  text << "\n";

  m_selectionInfoDisplay->setText(QString::fromStdString(text.str()));
}

void ComponentInfoController::displayAlignPeaksInfo(
    const std::vector<Mantid::Kernel::V3D> &planePeaks,
    const Mantid::Geometry::IPeak *peak) {

  using Mantid::Kernel::V3D;

  if (planePeaks.size() < 2)
    return;

  const auto pos1 = planePeaks[0];
  const auto pos2 = planePeaks[1];

  // find projection of beam direction onto plane
  // this is so we always orientate to a common reference direction
  const auto instrument = peak->getInstrument();
  const auto samplePos = instrument->getSample()->getPos();
  const auto sourcePos = instrument->getSource()->getPos();

  // find vectors in plane & plane normal
  auto u = pos2 - pos1;
  auto v = samplePos - pos1;
  auto n = u.cross_prod(v);

  const auto beam = samplePos - sourcePos;

  // Check if the chosen vectors result in a vector that is parallel
  // to the beam axis. If not take the projection, else use the beam
  if (!beam.cross_prod(n).nullVector()) {
    u = beam - n * (beam.scalar_prod(n) / n.norm2());
  } else {
    u = beam;
  }

  // update in-plane vectors
  v = u.cross_prod(n);

  u.normalize();
  v.normalize();
  n.normalize();

  // now compute in plane & out of plane angles
  const auto pos3 = peak->getQSampleFrame();
  const auto x = pos3.scalar_prod(u);
  const auto y = pos3.scalar_prod(v);
  const auto z = pos3.scalar_prod(n);

  const V3D p(x, y, z);
  // compute the elevation angle from the plane
  const auto R = p.norm();
  auto theta = (R != 0) ? asin(z / R) : 0.0;
  // compute in-plane angle
  auto phi = atan2(y, x);

  // convert angles to degrees
  theta *= double_constants::radian;
  phi *= double_constants::radian;

  std::stringstream text;
  text << " theta: " << theta << " phi: " << phi << "\n";

  m_selectionInfoDisplay->setText(QString::fromStdString(text.str()));
}

/**
* Return non-detector info to be displayed in the selection info display.
*/
QString ComponentInfoController::getPeakOverlayInfo() {
  QString text;
  QStringList overlays = m_tab->getSurface()->getPeaksWorkspaceNames();
  if (!overlays.isEmpty()) {
    text += "Peaks:\n" + overlays.join("\n") + "\n";
  }
  return text;
}

/**
* Clear the information display.
*/
void ComponentInfoController::clear() { m_selectionInfoDisplay->clear(); }

} // MantidWidgets
} // MantidQt
