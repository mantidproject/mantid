// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetPickTab.h"
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include "MantidQtWidgets/Common/TSVSerialiser.h"
#endif
#include "MantidQtWidgets/InstrumentView/CollapsiblePanel.h"
#include "MantidQtWidgets/InstrumentView/InstrumentActor.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"
#include "MantidQtWidgets/InstrumentView/PeakMarker2D.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

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
#include "MantidKernel/Unit.h"
#include "MantidKernel/V3D.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <numeric>
#include <vector>

#include <boost/math/constants/constants.hpp>

namespace MantidQt {
namespace MantidWidgets {

using namespace boost::math;

namespace {
// Get the phi angle between the detector with reference to the origin
// Makes assumptions about beam direction. Legacy code and not robust.
double getPhi(const Mantid::Kernel::V3D &pos) {
  return std::atan2(pos[1], pos[0]);
}

// Calculate the phi angle between detector and beam, and then offset.
// Makes assumptions about beam direction. Legacy code and not robust.
double getPhiOffset(const Mantid::Kernel::V3D &pos, const double offset) {
  double avgPos = getPhi(pos);
  return avgPos < 0 ? -(offset + avgPos) : offset - avgPos;
}
} // namespace

/**
 * Constructor.
 * @param instrWidget :: Parent InstrumentWidget.
 */
InstrumentWidgetPickTab::InstrumentWidgetPickTab(InstrumentWidget *instrWidget)
    : InstrumentWidgetTab(instrWidget), m_freezePlot(false),
      m_tubeXUnitsCache(0), m_plotTypeCache(0) {

  // connect to InstrumentWindow signals
  connect(m_instrWidget, SIGNAL(integrationRangeChanged(double, double)), this,
          SLOT(changedIntegrationRange(double, double)));

  m_plotSum = true;

  QVBoxLayout *layout = new QVBoxLayout(this);

  // set up the selection display
  m_selectionInfoDisplay = new QTextEdit(this);

  // set up the plot widget
  m_plot = new MiniPlot(this);
  connect(m_plot, SIGNAL(showContextMenu()), this, SLOT(plotContextMenu()));

  // Plot context menu actions
  m_sumDetectors = new QAction("Sum", this);
  m_sumDetectors->setCheckable(true);
  m_sumDetectors->setChecked(true);
  m_integrateTimeBins = new QAction("Integrate", this);
  m_integrateTimeBins->setCheckable(true);
  m_summationType = new QActionGroup(this);
  m_summationType->addAction(m_sumDetectors);
  m_summationType->addAction(m_integrateTimeBins);
  m_logY = new QAction("Y log scale", this);
  m_linearY = new QAction("Y linear scale", this);
  m_yScale = new QActionGroup(this);
  m_yScale->addAction(m_linearY);
  m_yScale->addAction(m_logY);
  m_logY->setCheckable(true);
  m_linearY->setCheckable(true);
  m_linearY->setChecked(true);
  connect(m_sumDetectors, SIGNAL(triggered()), this, SLOT(sumDetectors()));
  connect(m_integrateTimeBins, SIGNAL(triggered()), this,
          SLOT(integrateTimeBins()));
  connect(m_logY, SIGNAL(triggered()), m_plot, SLOT(setYLogScale()));
  connect(m_linearY, SIGNAL(triggered()), m_plot, SLOT(setYLinearScale()));

  m_unitsMapper = new QSignalMapper(this);

  m_detidUnits = new QAction("Detector ID", this);
  m_detidUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_detidUnits, DetectorPlotController::DETECTOR_ID);
  connect(m_detidUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_lengthUnits = new QAction("Tube length", this);
  m_lengthUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_lengthUnits, DetectorPlotController::LENGTH);
  connect(m_lengthUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_phiUnits = new QAction("Phi", this);
  m_phiUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_phiUnits, DetectorPlotController::PHI);
  connect(m_phiUnits, SIGNAL(triggered()), m_unitsMapper, SLOT(map()));

  m_outOfPlaneAngleUnits = new QAction("Out of plane angle", this);
  m_outOfPlaneAngleUnits->setCheckable(true);
  m_unitsMapper->setMapping(m_outOfPlaneAngleUnits,
                            DetectorPlotController::OUT_OF_PLANE_ANGLE);
  connect(m_outOfPlaneAngleUnits, SIGNAL(triggered()), m_unitsMapper,
          SLOT(map()));

  m_unitsGroup = new QActionGroup(this);
  m_unitsGroup->addAction(m_detidUnits);
  m_unitsGroup->addAction(m_lengthUnits);
  m_unitsGroup->addAction(
      m_phiUnits); // re #4169 disabled until fixed or removed
  m_unitsGroup->addAction(m_outOfPlaneAngleUnits);
  connect(m_unitsMapper, SIGNAL(mapped(int)), this, SLOT(setTubeXUnits(int)));

  // Instrument display context menu actions
  m_storeCurve = new QAction("Store curve", this);
  connect(m_storeCurve, SIGNAL(triggered()), this, SLOT(storeCurve()));
  m_savePlotToWorkspace = new QAction("Save plot to workspace", this);
  connect(m_savePlotToWorkspace, SIGNAL(triggered()), this,
          SLOT(savePlotToWorkspace()));

  CollapsibleStack *panelStack = new CollapsibleStack(this);
  m_infoPanel = panelStack->addPanel("Selection", m_selectionInfoDisplay);
  m_plotPanel = panelStack->addPanel("Name", m_plot);

  m_selectionType = Single;

  m_infoController = nullptr;
  m_plotController = nullptr;

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
 * Display the miniplot's context menu.
 */
void InstrumentWidgetPickTab::plotContextMenu() {
  QMenu context(m_plot);

  auto plotType = m_plotController->getPlotType();

  if (plotType == DetectorPlotController::TubeSum ||
      plotType == DetectorPlotController::TubeIntegral) {
    // only for multiple detector selectors
    context.addActions(m_summationType->actions());
    m_sumDetectors->setChecked(plotType == DetectorPlotController::TubeSum);
    m_integrateTimeBins->setChecked(plotType !=
                                    DetectorPlotController::TubeSum);
    m_integrateTimeBins->setEnabled(true);
    context.addSeparator();
  }

  if (m_plot->hasStored()) {
    // the remove menu
    QMenu *removeCurves = new QMenu("Remove", this);
    QSignalMapper *signalMapper = new QSignalMapper(this);
    QStringList labels = m_plot->getLabels();
    foreach (QString label, labels) {
      QColor c = m_plot->getCurveColor(label);
      QPixmap pixmap(16, 2);
      pixmap.fill(c);
      QAction *remove = new QAction(QIcon(pixmap), label, removeCurves);
      removeCurves->addAction(remove);
      connect(remove, SIGNAL(triggered()), signalMapper, SLOT(map()));
      signalMapper->setMapping(remove, label);
    }
    connect(signalMapper, SIGNAL(mapped(const QString &)), this,
            SLOT(removeCurve(const QString &)));
    context.addMenu(removeCurves);
  }

  // the axes menu
  QMenu *axes = new QMenu("Axes", this);
  axes->addActions(m_yScale->actions());
  if (m_plot->isYLogScale()) {
    m_logY->setChecked(true);
  } else {
    m_linearY->setChecked(true);
  }

  // Tube x units menu options
  if (plotType == DetectorPlotController::TubeIntegral) {
    axes->addSeparator();
    axes->addActions(m_unitsGroup->actions());
    auto tubeXUnits = m_plotController->getTubeXUnits();
    switch (tubeXUnits) {
    case DetectorPlotController::DETECTOR_ID:
      m_detidUnits->setChecked(true);
      break;
    case DetectorPlotController::LENGTH:
      m_lengthUnits->setChecked(true);
      break;
    case DetectorPlotController::PHI:
      m_phiUnits->setChecked(true);
      break;
    case DetectorPlotController::OUT_OF_PLANE_ANGLE:
      m_outOfPlaneAngleUnits->setChecked(true);
      break;
    default:
      m_detidUnits->setChecked(true);
    }
  }
  context.addMenu(axes);

  // save plot to workspace
  if (m_plot->hasStored() || m_plot->hasCurve()) {
    context.addAction(m_savePlotToWorkspace);
  }

  // show menu
  context.exec(QCursor::pos());
}

/**
 * Update the plot caption. The captions shows the selection type.
 */
void InstrumentWidgetPickTab::setPlotCaption() {
  m_plotPanel->setCaption(m_plotController->getPlotCaption());
}

/**
 * Switch to the detectors summing regime.
 */
void InstrumentWidgetPickTab::sumDetectors() {
  m_plotController->setPlotType(DetectorPlotController::TubeSum);
  m_plotController->updatePlot();
  setPlotCaption();
}

/**
 * Switch to the time bin integration regime.
 */
void InstrumentWidgetPickTab::integrateTimeBins() {
  m_plotController->setPlotType(DetectorPlotController::TubeIntegral);
  m_plotController->updatePlot();
  setPlotCaption();
}

/**
 * Set the selection type according to which tool button is checked.
 */
void InstrumentWidgetPickTab::setSelectionType() {
  ProjectionSurface::InteractionMode surfaceMode =
      ProjectionSurface::PickSingleMode;
  auto plotType = m_plotController->getPlotType();
  if (m_zoom->isChecked()) {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Navigation");
    surfaceMode = ProjectionSurface::MoveMode;
  } else if (m_one->isChecked()) {
    m_selectionType = Single;
    m_activeTool->setText("Tool: Pixel selection");
    surfaceMode = ProjectionSurface::PickSingleMode;
    plotType = DetectorPlotController::Single;
  } else if (m_tube->isChecked()) {
    m_selectionType = Tube;
    m_activeTool->setText("Tool: Tube/bank selection");
    surfaceMode = ProjectionSurface::PickTubeMode;
    if (plotType < DetectorPlotController::TubeSum) {
      plotType = DetectorPlotController::TubeSum;
    }
  } else if (m_peak->isChecked()) {
    m_selectionType = AddPeak;
    m_activeTool->setText("Tool: Add a single crystal peak");
    surfaceMode = ProjectionSurface::AddPeakMode;
    plotType = DetectorPlotController::Single;
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
    plotType = DetectorPlotController::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "rectangle", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ellipse->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Ellipse");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = DetectorPlotController::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ellipse", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ring_ellipse->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Elliptical ring");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = DetectorPlotController::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ring ellipse", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_ring_rectangle->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Rectangular ring");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = DetectorPlotController::Single;
    m_instrWidget->getSurface()->startCreatingShape2D(
        "ring rectangle", Qt::green, QColor(255, 255, 255, 80));
  } else if (m_free_draw->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Arbitrary shape");
    surfaceMode = ProjectionSurface::DrawFreeMode;
    plotType = DetectorPlotController::Single;
    m_instrWidget->getSurface()->startCreatingFreeShape(
        Qt::green, QColor(255, 255, 255, 80));
  } else if (m_edit->isChecked()) {
    m_selectionType = Draw;
    m_activeTool->setText("Tool: Shape editing");
    surfaceMode = ProjectionSurface::DrawRegularMode;
    plotType = DetectorPlotController::Single;
  }
  m_plotController->setPlotType(plotType);
  auto surface = m_instrWidget->getSurface();
  if (surface) {
    surface->setInteractionMode(surfaceMode);
    auto interactionMode = surface->getInteractionMode();
    if (interactionMode == ProjectionSurface::DrawRegularMode ||
        interactionMode == ProjectionSurface::MoveMode) {
      updatePlotMultipleDetectors();
    } else {
      m_plot->clearAll();
      m_plot->replot();
    }
    setPlotCaption();
  }
  m_instrWidget->updateInfoText();
}

/**
 * Respond to the show event.
 */
void InstrumentWidgetPickTab::showEvent(QShowEvent * /*unused*/) {
  // Make the state of the display view consistent with the current selection
  // type
  setSelectionType();
  // make sure picking updated
  m_instrWidget->updateInstrumentView(true);
  m_instrWidget->getSurface()->changeBorderColor(getShapeBorderColor());
}

/**
 * Keep current curve permanently displayed on the plot.
 */
void InstrumentWidgetPickTab::storeCurve() { m_plot->store(); }

/**
 * Remove a stored curve.
 * @param label :: The label of the curve to remove
 */
void InstrumentWidgetPickTab::removeCurve(const QString &label) {
  m_plot->removeCurve(label);
  m_plot->replot();
}

/**
 * Set the x units for the integrated tube plot.
 * @param units :: The x units in terms of TubeXUnits.
 */
void InstrumentWidgetPickTab::setTubeXUnits(int units) {
  if (units < 0 || units >= DetectorPlotController::NUMBER_OF_UNITS)
    return;
  auto tubeXUnits = static_cast<DetectorPlotController::TubeXUnits>(units);
  m_plotController->setTubeXUnits(tubeXUnits);
  m_plotController->updatePlot();
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
void InstrumentWidgetPickTab::changedIntegrationRange(double /*unused*/,
                                                      double /*unused*/) {
  m_plotController->updatePlot();
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
  connect(surface,
          SIGNAL(comparePeaks(
              const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                              std::vector<Mantid::Geometry::IPeak *>> &)),
          this,
          SLOT(comparePeaks(
              const std::pair<std::vector<Mantid::Geometry::IPeak *>,
                              std::vector<Mantid::Geometry::IPeak *>> &)));
  connect(surface,
          SIGNAL(alignPeaks(const std::vector<Mantid::Kernel::V3D> &,
                            const Mantid::Geometry::IPeak *)),
          this,
          SLOT(alignPeaks(const std::vector<Mantid::Kernel::V3D>,
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
  m_plotController = new DetectorPlotController(this, m_instrWidget, m_plot);
  m_plotController->setTubeXUnits(
      static_cast<DetectorPlotController::TubeXUnits>(m_tubeXUnitsCache));
  m_plotController->setPlotType(
      static_cast<DetectorPlotController::PlotType>(m_plotTypeCache));
  // miniplot X unit
  const auto &actor = m_instrWidget->getInstrumentActor();
  // default X axis label
  m_plot->setXLabel(QString::fromStdString(
      actor.getWorkspace()->getAxis(0)->unit()->unitID()));
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
  settings.setValue("TubeXUnits", m_plotController->getTubeXUnits());
  settings.setValue("PlotType", m_plotController->getPlotType());
}

/**
 * Restore (read and apply) tab's persistent settings from the provided
 * QSettings instance
 */
void InstrumentWidgetPickTab::loadSettings(const QSettings &settings) {
  // loadSettings is called when m_plotController is not created yet.
  // Cache the settings and apply them later
  m_tubeXUnitsCache = settings.value("TubeXUnits", 0).toInt();
  m_plotTypeCache =
      settings.value("PlotType", DetectorPlotController::Single).toInt();
}

/**
 * Fill in the context menu.
 * @param context :: A menu to fill.
 */
bool InstrumentWidgetPickTab::addToDisplayContextMenu(QMenu &context) const {
  m_freezePlot = true;
  bool res = false;
  if (m_plot->hasCurve()) {
    context.addAction(m_storeCurve);
    res = true;
  }
  if (m_plot->hasStored() || m_plot->hasCurve()) {
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
    m_plotController->setPlotData(pickID);
    m_plotController->updatePlot();
  }
}

void InstrumentWidgetPickTab::singleComponentPicked(size_t pickID) {
  m_infoController->displayInfo(pickID);
  m_plotController->setPlotData(pickID);
  m_plotController->zoomOutOnPlot();
  m_plotController->updatePlot();
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
    m_plotController->setPlotData(dets);
  } else {
    m_plotController->clear();
  }
  m_plot->replot();
}

/**
 * Save data plotted on the miniplot into a MatrixWorkspace.
 */
void InstrumentWidgetPickTab::savePlotToWorkspace() {
  m_plotController->savePlotToWorkspace();
}

/** Load pick tab state from a Mantid project file
 * @param lines :: lines from the project file to load state from
 */
void InstrumentWidgetPickTab::loadFromProject(const std::string &lines) {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  API::TSVSerialiser tsv(lines);

  if (!tsv.selectSection("picktab"))
    return;

  std::string tabLines;
  tsv >> tabLines;
  API::TSVSerialiser tab(tabLines);

  // load active push button
  std::vector<QPushButton *> buttons{
      m_zoom,         m_edit,           m_ellipse,   m_rectangle,
      m_ring_ellipse, m_ring_rectangle, m_free_draw, m_one,
      m_tube,         m_peak,           m_peakSelect};

  tab.selectLine("ActiveTools");
  for (auto button : buttons) {
    bool value;
    tab >> value;
    button->setChecked(value);
  }
#else
  Q_UNUSED(lines);
  throw std::runtime_error(
      "MaskBinsData::loadFromProject() not implemented for Qt >= 5");
#endif
}

/** Save the state of the pick tab to a Mantid project file
 * @return a string representing the state of the pick tab
 */
std::string InstrumentWidgetPickTab::saveToProject() const {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  API::TSVSerialiser tsv, tab;

  // save active push button
  std::vector<QPushButton *> buttons{
      m_zoom,         m_edit,           m_ellipse,   m_rectangle,
      m_ring_ellipse, m_ring_rectangle, m_free_draw, m_one,
      m_tube,         m_peak,           m_peakSelect};

  tab.writeLine("ActiveTools");
  for (auto button : buttons) {
    tab << button->isChecked();
  }

  tsv.writeSection("picktab", tab.outputLines());
  return tsv.outputLines();
#else
  throw std::runtime_error(
      "MaskBinsData::saveToProject() not implemented for Qt >= 5");
#endif
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

QString ComponentInfoController::displayPeakAngles(
    const std::pair<Mantid::Geometry::IPeak *, Mantid::Geometry::IPeak *>
        &peaks) {
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

//=====================================================================================//

/**
 * Constructor.
 * @param tab :: The parent tab.
 * @param instrWidget :: A pointer to a InstrumentWidget instance.
 * @param plot :: The plot widget.
 */
DetectorPlotController::DetectorPlotController(InstrumentWidgetPickTab *tab,
                                               InstrumentWidget *instrWidget,
                                               MiniPlot *plot)
    : QObject(tab), m_tab(tab), m_instrWidget(instrWidget), m_plot(plot),
      m_plotType(Single), m_enabled(true), m_tubeXUnits(DETECTOR_ID),
      m_currentPickID(std::numeric_limits<size_t>::max()) {
  connect(m_plot, SIGNAL(clickedAt(double, double)), this,
          SLOT(addPeak(double, double)));
}

/**
 * Update the miniplot for a selected detector. The curve data depend on the
 * plot type.
 * @param pickID :: A pick ID of an instrument component.
 */
void DetectorPlotController::setPlotData(size_t pickID) {
  if (m_plotType == DetectorSum) {
    m_plotType = Single;
  }

  if (!m_enabled) {
    m_currentPickID = std::numeric_limits<size_t>::max();
    m_plot->clearCurve();
    return;
  }

  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  if (componentInfo.isDetector(pickID)) {
    if (m_plotType == Single) {
      if (m_currentPickID != pickID) {
        m_currentPickID = pickID;
        plotSingle(pickID);
      }
    } else if (m_plotType == TubeSum || m_plotType == TubeIntegral) {
      m_currentPickID = std::numeric_limits<size_t>::max();
      plotTube(pickID);
    } else {
      throw std::logic_error("setPlotData: Unexpected plot type.");
    }
  } else {
    m_currentPickID = std::numeric_limits<size_t>::max();
    m_plot->clearCurve();
  }
}

/**
 * Set curev data from multiple detectors: sum their spectra.
 * @param detIndices :: A list of detector Indices.
 */
void DetectorPlotController::setPlotData(
    const std::vector<size_t> &detIndices) {
  setPlotType(DetectorSum);
  clear();
  std::vector<double> x, y;
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  const auto &actor = m_instrWidget->getInstrumentActor();
  actor.sumDetectors(detIndices, x, y, static_cast<size_t>(m_plot->width()));
  QApplication::restoreOverrideCursor();
  if (!x.empty()) {
    m_plot->setData(std::move(x), std::move(y),
                    QString::fromStdString(
                        actor.getWorkspace()->getAxis(0)->unit()->unitID()),
                    "multiple");
  }

  addPeakLabels(detIndices);
}

/**
 * Add peak labels to plot for each detector shown
 * @param detIndices :: A list of detector inidices.
 */
void DetectorPlotController::addPeakLabels(
    const std::vector<size_t> &detIndices) {
  auto surface = m_tab->getSurface();
  if (surface) {
    const auto &actor = m_instrWidget->getInstrumentActor();
    auto detIds = actor.getDetIDs(detIndices);
    for (const auto &detId : detIds) {
      auto markers = surface->getMarkersWithID(static_cast<int>(detId));
      for (const auto &marker : markers) {
        m_plot->addPeakLabel(marker);
      }
    }
  }
}

/**
 * Update the miniplot for a selected detector.
 */
void DetectorPlotController::updatePlot() {
  m_plot->recalcAxisDivs();
  m_plot->replot();
}

/**
 * Clear the plot.
 */
void DetectorPlotController::clear() {
  m_plot->clearCurve();
  m_plot->clearPeakLabels();
}

/**
 * Plot data for a detector.
 * @param detindex :: Index of the detector to be plotted.
 */
void DetectorPlotController::plotSingle(size_t detindex) {
  clear();
  std::vector<double> x, y;
  prepareDataForSinglePlot(detindex, x, y);
  if (x.empty() || y.empty())
    return;

  const auto &actor = m_instrWidget->getInstrumentActor();
  // set the data
  auto detid = actor.getDetID(detindex);
  m_plot->setData(std::move(x), std::move(y),
                  QString::fromStdString(
                      actor.getWorkspace()->getAxis(0)->unit()->unitID()),
                  "Detector " + QString::number(detid));

  // find any markers
  auto surface = m_tab->getSurface();
  if (surface) {
    QList<PeakMarker2D *> markers = surface->getMarkersWithID(detid);
    foreach (PeakMarker2D *marker, markers) { m_plot->addPeakLabel(marker); }
  }
}

/**
 * Plot data integrated either over the detectors in a tube or over time bins.
 * If m_plotSum == true the miniplot displays the accumulated data in a tube
 * against time of flight.
 * If m_plotSum == false the miniplot displays the data integrated over the time
 * bins. The values are
 * plotted against the length of the tube, but the units on the x-axis can be
 * one of the following: DETECTOR_ID LENGTH PHI The units can be set with
 * setTubeXUnits(...) method.
 * @param detindex :: A detector index. The miniplot will display data for a
 * component
 * containing the detector
 *   with this id.
 */
void DetectorPlotController::plotTube(size_t detindex) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();

  if (!componentInfo.hasParent(detindex)) {
    m_plot->clearCurve();
    return;
  }

  auto parent = componentInfo.parent(detindex);
  if (componentInfo.detectorsInSubtree(parent).size() > 0) {
    if (m_plotType == TubeSum) // plot sums over detectors vs time bins
    {
      plotTubeSums(detindex);
    } else // plot detector integrals vs detID or a function of detector
           // position in the tube
    {
      assert(m_plotType == TubeIntegral);
      plotTubeIntegrals(detindex);
    }
  }
}

/**
 * Plot the accumulated data in a tube against time of flight.
 * @param detindex :: A detector id. The miniplot will display data for a
 * component
 * containing the detector
 *   with this id.
 */
void DetectorPlotController::plotTubeSums(size_t detindex) {
  std::vector<double> x, y;
  prepareDataForSumsPlot(detindex, x, y);
  if (x.empty() || y.empty()) {
    clear();
    return;
  }
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  auto parent = componentInfo.parent(detindex);
  auto detid = actor.getDetID(detindex);
  QString label = QString::fromStdString(componentInfo.name(parent)) + " (" +
                  QString::number(detid) + ") Sum";
  m_plot->setData(std::move(x), std::move(y),
                  QString::fromStdString(
                      actor.getWorkspace()->getAxis(0)->unit()->unitID()),
                  std::move(label));
}

/**
 * Plot the data integrated over the time bins. The values are
 * plotted against the length of the tube, but the units on the x-axis can be
 * one of the following: DETECTOR_ID LENGTH PHI The units can be set with
 * setTubeXUnits(...) method.
 * @param detindex :: A detector index. The miniplot will display data for a
 * component
 * containing the detector
 *   with this id.
 */
void DetectorPlotController::plotTubeIntegrals(size_t detindex) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  std::vector<double> x, y;
  prepareDataForIntegralsPlot(detindex, x, y);
  if (x.empty() || y.empty()) {
    clear();
    return;
  }
  auto xAxisCaption = getTubeXUnitsName();
  auto xAxisUnits = getTubeXUnitsUnits();
  if (!xAxisUnits.isEmpty()) {
    xAxisCaption += " (" + xAxisUnits + ")";
  }
  auto parent = componentInfo.parent(detindex);
  // curve label: "tube_name (detid) Integrals"
  // detid is included to distiguish tubes with the same name
  QString label = QString::fromStdString(componentInfo.name(parent)) + " (" +
                  QString::number(actor.getDetID(detindex)) + ") Integrals/" +
                  getTubeXUnitsName();
  m_plot->setData(std::move(x), std::move(y), std::move(xAxisCaption),
                  std::move(label));
}

/**
 * Prepare data for plotting a spectrum of a single detector.
 * @param detindex :: Index of the detector to be plotted.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void DetectorPlotController::prepareDataForSinglePlot(
    size_t detindex, std::vector<double> &x, std::vector<double> &y,
    std::vector<double> *err) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr ws = actor.getWorkspace();
  auto wi = actor.getWorkspaceIndex(detindex);
  if (wi == InstrumentActor::INVALID_INDEX)
    return;
  // get the data
  const auto &XPoints = ws->points(wi);
  const auto &Y = ws->y(wi);
  const auto &E = ws->e(wi);

  // find min and max for x
  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  x.assign(XPoints.begin() + imin, XPoints.begin() + imax);
  y.assign(Y.begin() + imin, Y.begin() + imax);
  if (err) {
    err->assign(E.begin() + imin, E.begin() + imax);
  }
}

/**
 * Prepare data for plotting accumulated data in a tube against time of flight.
 * @param detindex :: A detector index. The miniplot will display data for a
 * component
 * containing the detector with this index.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void DetectorPlotController::prepareDataForSumsPlot(size_t detindex,
                                                    std::vector<double> &x,
                                                    std::vector<double> &y,
                                                    std::vector<double> *err) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  auto ws = actor.getWorkspace();
  const auto &componentInfo = actor.componentInfo();
  auto parent = componentInfo.parent(detindex);
  auto ass = componentInfo.detectorsInSubtree(parent);

  auto wi = actor.getWorkspaceIndex(detindex);

  if (wi == InstrumentActor::INVALID_INDEX)
    return;

  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  const auto &XPoints = ws->points(wi);
  x.assign(XPoints.begin() + imin, XPoints.begin() + imax);
  y.resize(x.size(), 0);
  if (err)
    err->resize(x.size(), 0);

  for (auto det : ass) {
    if (componentInfo.isDetector(det)) {
      auto index = actor.getWorkspaceIndex(det);
      if (index == InstrumentActor::INVALID_INDEX)
        continue;
      const auto &Y = ws->y(index);
      std::transform(y.begin(), y.end(), Y.begin() + imin, y.begin(),
                     std::plus<double>());
      if (err) {
        const auto &E = ws->e(index);
        std::vector<double> tmp;
        tmp.assign(E.begin() + imin, E.begin() + imax);
        std::transform(tmp.begin(), tmp.end(), tmp.begin(), tmp.begin(),
                       std::multiplies<double>());
        std::transform(err->begin(), err->end(), tmp.begin(), err->begin(),
                       std::plus<double>());
      }
    }
  }

  if (err)
    std::transform(err->begin(), err->end(), err->begin(),
                   [](double val) { return sqrt(val); });
}

/**
 * Prepare data for plotting the data integrated over the time bins. The values
 * are
 * plotted against the length of the tube, but the units on the x-axis can be
 * one of the following: DETECTOR_ID LENGTH PHI OUT_OF_PLANE_ANGLE The units can
 * be set with setTubeXUnits(...) method.
 * @param detid :: A detector id. The miniplot will display data for a component
 * containing the detector with this index.
 * @param x :: Vector of x coordinates (output)
 * @param y :: Vector of y coordinates (output)
 * @param err :: Optional pointer to a vector of errors (output)
 */
void DetectorPlotController::prepareDataForIntegralsPlot(
    size_t detindex, std::vector<double> &x, std::vector<double> &y,
    std::vector<double> *err) {

#define PREPAREDATAFORINTEGRALSPLOT_RETURN_FAILED                              \
  x.clear();                                                                   \
  y.clear();                                                                   \
  if (err)                                                                     \
    err->clear();                                                              \
  return;

  const auto &actor = m_instrWidget->getInstrumentActor();
  const auto &componentInfo = actor.componentInfo();
  Mantid::API::MatrixWorkspace_const_sptr ws = actor.getWorkspace();

  // Does the instrument definition specify that psi should be offset.
  std::vector<std::string> parameters = actor.getStringParameter("offset-phi");
  const bool bOffsetPsi =
      (!parameters.empty()) && std::find(parameters.begin(), parameters.end(),
                                         "Always") != parameters.end();
  auto parent = componentInfo.parent(detindex);
  auto ass = componentInfo.detectorsInSubtree(parent);
  auto wi = actor.getWorkspaceIndex(detindex);
  if (wi == InstrumentActor::INVALID_INDEX)
    return;
  // imin and imax give the bin integration range
  size_t imin, imax;
  actor.getBinMinMaxIndex(wi, imin, imax);

  auto samplePos = actor.componentInfo().samplePosition();

  auto n = ass.size();
  if (n == 0) {
    // don't think it's ever possible but...
    throw std::runtime_error("PickTab miniplot: empty instrument assembly");
  }
  if (n == 1) {
    // if assembly has just one element there is nothing to plot
    PREPAREDATAFORINTEGRALSPLOT_RETURN_FAILED
  }
  // collect and sort xy pairs in xymap
  std::map<double, double> xymap, errmap;
  // get the first detector in the tube for lenth calculation
  if (!componentInfo.isDetector(ass[0])) {
    // it's not an assembly of detectors,
    // could be a mixture of monitors and other components
    PREPAREDATAFORINTEGRALSPLOT_RETURN_FAILED
  }

  const auto normal = normalize(componentInfo.position(ass[1]) -
                                componentInfo.position(ass[0]));
  const auto &detectorInfo = actor.detectorInfo();
  for (auto det : ass) {
    if (componentInfo.isDetector(det)) {
      auto id = detectorInfo.detectorIDs()[det];
      // get the x-value for detector idet
      double xvalue = 0;
      auto pos = detectorInfo.position(det);
      switch (m_tubeXUnits) {
      case LENGTH:
        xvalue = pos.distance(detectorInfo.position(ass[0]));
        break;
      case PHI:
        xvalue = bOffsetPsi ? getPhiOffset(pos, M_PI) : getPhi(pos);
        break;
      case OUT_OF_PLANE_ANGLE: {
        xvalue = getOutOfPlaneAngle(pos, samplePos, normal);
        break;
      }
      default:
        xvalue = static_cast<double>(id);
      }
      auto index = actor.getWorkspaceIndex(det);
      if (index == InstrumentActor::INVALID_INDEX)
        continue;
      // get the y-value for detector idet
      const auto &Y = ws->y(index);
      xymap[xvalue] = std::accumulate(Y.begin() + imin, Y.begin() + imax, 0);
      if (err) {
        const auto &E = ws->e(index);
        std::vector<double> tmp(imax - imin);
        // take squares of the errors
        std::transform(E.begin() + imin, E.begin() + imax, E.begin() + imin,
                       tmp.begin(), std::multiplies<double>());
        // sum them
        const double sum = std::accumulate(tmp.begin(), tmp.end(), 0);
        // take sqrt
        errmap[xvalue] = sqrt(sum);
      }
    }
  }
  if (!xymap.empty()) {
    // set the plot curve data
    x.resize(xymap.size());
    y.resize(xymap.size());
    std::map<double, double>::const_iterator xy = xymap.begin();
    for (size_t i = 0; xy != xymap.end(); ++xy, ++i) {
      x[i] = xy->first;
      y[i] = xy->second;
    }
    if (err) {
      err->resize(errmap.size());
      std::map<double, double>::const_iterator e = errmap.begin();
      for (size_t i = 0; e != errmap.end(); ++e, ++i) {
        (*err)[i] = e->second;
      }
    }
  } else {
    PREPAREDATAFORINTEGRALSPLOT_RETURN_FAILED
  }
#undef PREPAREDATAFORINTEGRALSPLOT_RETURN_FAILED
}

/**
 * Save data plotted on the miniplot into a MatrixWorkspace.
 */
void DetectorPlotController::savePlotToWorkspace() {
  if (!m_plot->hasCurve() && !m_plot->hasStored()) {
    // nothing to save
    return;
  }
  const auto &actor = m_instrWidget->getInstrumentActor();
  Mantid::API::MatrixWorkspace_const_sptr parentWorkspace =
      actor.getWorkspace();
  // interpret curve labels and reconstruct the data to be saved
  QStringList labels = m_plot->getLabels();
  if (m_plot->hasCurve()) {
    labels << m_plot->label();
  }
  std::vector<double> X, Y, E;
  size_t nbins = 0;
  // to keep det ids for spectrum-detector mapping in the output workspace
  std::vector<Mantid::detid_t> detids;
  // unit id for x vector in the created workspace
  std::string unitX;
  foreach (QString label, labels) {
    std::vector<double> x, y, e;
    // split the label to get the detector id and selection type
    QStringList parts = label.split(QRegExp("[()]"));
    if (label == "multiple") {
      if (X.empty()) {
        // label doesn't have any info on how to reproduce the curve:
        // only the current curve can be saved
        std::vector<size_t> dets;
        m_tab->getSurface()->getMaskedDetectors(dets);
        actor.sumDetectors(dets, x, y);
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      } else {
        QMessageBox::warning(nullptr, "Mantid - Warning",
                             "Cannot save the stored curves.\nOnly the current "
                             "curve will be saved.");
      }
    } else if (parts.size() == 3) {
      const auto detindex = actor.getDetectorByDetID(parts[1].toInt());
      QString SumOrIntegral = parts[2].trimmed();
      if (SumOrIntegral == "Sum") {
        prepareDataForSumsPlot(detindex, x, y, &e);
        unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      } else {
        prepareDataForIntegralsPlot(detindex, x, y, &e);
        unitX = SumOrIntegral.split('/')[1].toStdString();
      }
    } else if (parts.size() == 1) {
      // second word is detector id
      const auto detid = parts[0].split(QRegExp("\\s+"))[1].toInt();
      prepareDataForSinglePlot(actor.getDetectorByDetID(detid), x, y, &e);
      unitX = parentWorkspace->getAxis(0)->unit()->unitID();
      // save det ids for the output workspace
      detids.push_back(static_cast<Mantid::detid_t>(detid));
    } else {
      continue;
    }
    if (!x.empty()) {
      if (nbins > 0 && x.size() != nbins) {
        QMessageBox::critical(nullptr, "MantidPlot - Error",
                              "Curves have different sizes.");
        return;
      } else {
        nbins = x.size();
      }
      X.insert(X.end(), x.begin(), x.end());
      Y.insert(Y.end(), y.begin(), y.end());
      E.insert(E.end(), e.begin(), e.end());
    }
  }
  // call CreateWorkspace algorithm. Created worksapce will have name "Curves"
  if (!X.empty()) {
    if (nbins == 0)
      nbins = 1;
    E.resize(Y.size(), 1.0);
    Mantid::API::IAlgorithm_sptr alg =
        Mantid::API::AlgorithmFactory::Instance().create("CreateWorkspace", -1);
    alg->initialize();
    alg->setPropertyValue("OutputWorkspace", "Curves");
    alg->setProperty("DataX", X);
    alg->setProperty("DataY", Y);
    alg->setProperty("DataE", E);
    alg->setProperty("NSpec", static_cast<int>(X.size() / nbins));
    alg->setProperty("UnitX", unitX);
    alg->setPropertyValue("ParentWorkspace", parentWorkspace->getName());
    alg->execute();

    if (!detids.empty()) {
      // set up spectra - detector mapping
      Mantid::API::MatrixWorkspace_sptr ws =
          boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
              Mantid::API::AnalysisDataService::Instance().retrieve("Curves"));
      if (!ws) {
        throw std::runtime_error("Failed to create Curves workspace");
      }

      if (detids.size() == ws->getNumberHistograms()) {
        size_t i = 0;
        for (std::vector<Mantid::detid_t>::const_iterator id = detids.begin();
             id != detids.end(); ++id, ++i) {
          ws->getSpectrum(i).setDetectorID(*id);
        }
      }

    } // !detids.empty()
  }
}

/**
 * Calculate the angle between a vector ( == pos - origin ) and a plane (
 * orthogonal to normal ).
 * The angle is positive if the vector and the normal make an acute angle.
 * @param pos :: Vector's end.
 * @param origin :: Vector's origin.
 * @param normal :: Normal to the plane.
 * @return :: Angle between the vector and the plane in radians in [-pi/2,
 * pi/2].
 */
double
DetectorPlotController::getOutOfPlaneAngle(const Mantid::Kernel::V3D &pos,
                                           const Mantid::Kernel::V3D &origin,
                                           const Mantid::Kernel::V3D &normal) {
  const auto vec = normalize(pos - origin);
  return asin(vec.scalar_prod(normal));
}

/**
 * Return symbolic name of current TubeXUnit.
 */
QString DetectorPlotController::getTubeXUnitsName() const {
  switch (m_tubeXUnits) {
  case LENGTH:
    return "Length";
  case PHI:
    return "Phi";
  case OUT_OF_PLANE_ANGLE:
    return "Out of plane angle";
  default:
    break;
  }
  return "Detector ID";
}

/**
 * Return symbolic name of units of current TubeXUnit.
 */
QString DetectorPlotController::getTubeXUnitsUnits() const {
  switch (m_tubeXUnits) {
  case LENGTH:
    return "m";
  case PHI:
    return "radians";
  case OUT_OF_PLANE_ANGLE:
    return "radians";
  default:
    return "";
  }
}

/**
 * Get the plot caption for the current plot type.
 */
QString DetectorPlotController::getPlotCaption() const {

  switch (m_plotType) {
  case Single:
    return "Plotting detector spectra";
  case DetectorSum:
    return "Plotting multiple detector sum";
  case TubeSum:
    return "Plotting sum";
  case TubeIntegral:
    return "Plotting integral";
  default:
    throw std::logic_error("getPlotCaption: Unknown plot type.");
  }
}

/**
 * Add a peak to the single crystal peak table.
 * @param x :: Time of flight
 * @param y :: Peak height (counts)
 */
void DetectorPlotController::addPeak(double x, double y) {
  if (m_currentPickID == std::numeric_limits<size_t>::max())
    return;

  try {
    auto surface = m_tab->getSurface();
    if (!surface)
      return;
    const auto &actor = m_instrWidget->getInstrumentActor();
    Mantid::API::IPeaksWorkspace_sptr tw = surface->getEditPeaksWorkspace();
    Mantid::API::MatrixWorkspace_const_sptr ws = actor.getWorkspace();
    std::string peakTableName;
    bool newPeaksWorkspace = false;
    if (tw) {
      peakTableName = tw->getName();
    } else {
      peakTableName = "SingleCrystalPeakTable";
      // This does need to get the instrument from the workspace as it's doing
      // calculations
      // .....and this method should be an algorithm! Or at least somewhere
      // different to here.
      Mantid::Geometry::Instrument_const_sptr instr = ws->getInstrument();

      if (!Mantid::API::AnalysisDataService::Instance().doesExist(
              peakTableName)) {
        tw = Mantid::API::WorkspaceFactory::Instance().createPeaks(
            "PeaksWorkspace");
        tw->setInstrument(instr);
        Mantid::API::AnalysisDataService::Instance().add(peakTableName, tw);
        newPeaksWorkspace = true;
      } else {
        tw = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                peakTableName));
        if (!tw) {
          QMessageBox::critical(m_tab, "Mantid - Error",
                                "Workspace " +
                                    QString::fromStdString(peakTableName) +
                                    " is not a TableWorkspace");
          return;
        }
      }
      auto unwrappedSurface = dynamic_cast<UnwrappedSurface *>(surface.get());
      if (unwrappedSurface) {
        unwrappedSurface->setPeaksWorkspace(
            boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(tw));
      }
    }

    // Run the AddPeak algorithm
    auto alg =
        Mantid::API::FrameworkManager::Instance().createAlgorithm("AddPeak");
    const auto &detIDs =
        m_instrWidget->getInstrumentActor().detectorInfo().detectorIDs();
    alg->setPropertyValue("RunWorkspace", ws->getName());
    alg->setPropertyValue("PeaksWorkspace", peakTableName);
    alg->setProperty("DetectorID", detIDs[m_currentPickID]);
    alg->setProperty("TOF", x);
    alg->setProperty("Height", actor.getIntegratedCounts(m_currentPickID));
    alg->setProperty("BinCount", y);
    alg->execute();

    // if data WS has UB copy it to the new peaks workspace
    if (newPeaksWorkspace && ws->sample().hasOrientedLattice()) {
      auto UB = ws->sample().getOrientedLattice().getUB();
      auto lattice = new Mantid::Geometry::OrientedLattice;
      lattice->setUB(UB);
      tw->mutableSample().setOrientedLattice(lattice);
    }

    // if there is a UB available calculate HKL for the new peak
    if (tw->sample().hasOrientedLattice()) {
      alg = Mantid::API::FrameworkManager::Instance().createAlgorithm(
          "CalculatePeaksHKL");
      alg->setPropertyValue("PeaksWorkspace", peakTableName);
      alg->execute();
    }
  } catch (std::exception &e) {
    QMessageBox::critical(
        m_tab, "MantidPlot -Error",
        "Cannot create a Peak object because of the error:\n" +
            QString(e.what()));
  }
}

/**
 * Zoom out back to the natural home of the mini plot
 */
void DetectorPlotController::zoomOutOnPlot() {
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
// Do nothing if in Qt4 or below.
#else
  m_plot->zoomOutOnPlot();
#endif
}
} // namespace MantidWidgets
} // namespace MantidQt
