#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <limits>
#include <sstream>
#include <vector>
#include <boost/make_shared.hpp>
#include <boost/math/special_functions/fpclassify.hpp>

#include "MantidAPI/CoordTransform.h"
#include "MantidAPI/IMDIterator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/PeakTransformHKL.h"
#include "MantidAPI/PeakTransformQSample.h"
#include "MantidAPI/PeakTransformQLab.h"
#include "MantidAPI/IPeaksWorkspace.h"
#include "MantidAPI/IMDHistoWorkspace.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/MDTypes.h"
#include "MantidKernel/ReadLock.h"
#include "MantidQtAPI/FileDialogHandler.h"
#include "MantidQtAPI/PlotAxis.h"
#include "MantidQtAPI/SignalRange.h"
#include "MantidQtSliceViewer/SliceViewer.h"
#include "MantidQtSliceViewer/CustomTools.h"
#include "MantidQtSliceViewer/DimensionSliceWidget.h"
#include "MantidQtSliceViewer/LineOverlay.h"
#include "MantidQtSliceViewer/SnapToGridDialog.h"
#include "MantidQtSliceViewer/XYLimitsDialog.h"
#include "MantidQtSliceViewer/ConcretePeaksPresenter.h"
#include "MantidQtSliceViewer/CompositePeaksPresenter.h"
#include "MantidQtSliceViewer/ProxyCompositePeaksPresenter.h"
#include "MantidQtSliceViewer/PeakOverlayMultiCrossFactory.h"
#include "MantidQtSliceViewer/PeakOverlayMultiSphereFactory.h"
#include "MantidQtSliceViewer/FirstExperimentInfoQuery.h"
#include "MantidQtSliceViewer/PeakBoundingBox.h"
#include "MantidQtSliceViewer/PeaksViewerOverlayDialog.h"
#include "MantidQtSliceViewer/PeakOverlayViewFactorySelector.h"
#include "MantidQtMantidWidgets/SelectWorkspacesDialog.h"

#include <qwt_plot_panner.h>
#include <Poco/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NodeIterator.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/Exception.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using MantidQt::API::SyncedCheckboxes;
using Poco::XML::DOMParser;
using Poco::XML::Document;
using Poco::XML::Element;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using MantidQt::API::AlgorithmRunner;

namespace MantidQt {
namespace SliceViewer {

//------------------------------------------------------------------------------------
/** Constructor */
SliceViewer::SliceViewer(QWidget *parent)
    : QWidget(parent), m_ws(), m_firstWorkspaceOpen(false), m_dimensions(),
      m_data(NULL), m_X(), m_Y(), m_dimX(0), m_dimY(1), m_logColor(false),
      m_fastRender(true), m_rebinMode(false), m_rebinLocked(true),
      m_peaksPresenter(boost::make_shared<CompositePeaksPresenter>(this)),
      m_proxyPeaksPresenter(
          boost::make_shared<ProxyCompositePeaksPresenter>(m_peaksPresenter)),
      m_peaksSliderWidget(NULL) {
  ui.setupUi(this);

  m_inf = std::numeric_limits<double>::infinity();

  // Point m_plot to the plot created in QtDesigner
  m_plot = ui.safeQwtPlot;
  // Add a spectrograph
  m_spect = new QwtPlotSpectrogram();
  m_spect->attach(m_plot);

  // Set up the ColorBarWidget
  m_colorBar = ui.colorBarWidget;
  m_colorBar->setViewRange(0, 10);
  m_colorBar->setLog(true);
  QObject::connect(m_colorBar, SIGNAL(changedColorRange(double, double, bool)),
                   this, SLOT(colorRangeChanged()));

  // ---- Set the color map on the data ------
  m_data = new API::QwtRasterDataMD();
  m_spect->setColorMap(m_colorBar->getColorMap());
  m_plot->autoRefresh();

  // Make the splitter use the minimum size for the controls and not stretch out
  ui.splitter->setStretchFactor(0, 0);
  ui.splitter->setStretchFactor(1, 1);
  initZoomer();
  ui.btnZoom->hide();

  // ----------- Toolbar button signals ----------------
  QObject::connect(ui.btnResetZoom, SIGNAL(clicked()), this, SLOT(resetZoom()));
  QObject::connect(ui.btnClearLine, SIGNAL(clicked()), this, SLOT(clearLine()));
  QObject::connect(ui.btnRangeFull, SIGNAL(clicked()), this,
                   SLOT(setColorScaleAutoFull()));
  QObject::connect(ui.btnRangeSlice, SIGNAL(clicked()), this,
                   SLOT(setColorScaleAutoSlice()));
  QObject::connect(ui.btnRebinRefresh, SIGNAL(clicked()), this,
                   SLOT(rebinParamsChanged()));
  QObject::connect(ui.btnAutoRebin, SIGNAL(toggled(bool)), this,
                   SLOT(autoRebin_toggled(bool)));
  // ----------- Other signals ----------------
  QObject::connect(m_colorBar, SIGNAL(colorBarDoubleClicked()), this,
                   SLOT(loadColorMapSlot()));

  m_algoRunner = new AlgorithmRunner(this);
  QObject::connect(m_algoRunner, SIGNAL(algorithmComplete(bool)), this,
                   SLOT(dynamicRebinComplete(bool)));

  initMenus();

  loadSettings();

  updateDisplay();

  // -------- Line Overlay ----------------
  m_lineOverlay = new LineOverlay(m_plot, m_plot->canvas());
  m_lineOverlay->setShown(false);

  m_overlayWSOutline = new LineOverlay(m_plot, m_lineOverlay);
  m_overlayWSOutline->setShowHandles(false);
  m_overlayWSOutline->setShowLine(false);
  m_overlayWSOutline->setShown(false);

  // -------- Peak Overlay ----------------
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<PeakTransformHKLFactory>());
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<PeakTransformQSampleFactory>());
  m_peakTransformSelector.registerCandidate(
      boost::make_shared<PeakTransformQLabFactory>());
  this->setAcceptDrops(true);
}

//------------------------------------------------------------------------------------
/// Destructor
SliceViewer::~SliceViewer() {
  saveSettings();
  delete m_data;
  // Don't delete Qt objects, I think these are auto-deleted
}

//------------------------------------------------------------------------------------
/** Load QSettings from .ini-type files */
void SliceViewer::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/SliceViewer");
  bool scaleType = (bool)settings.value("LogColorScale", 0).toInt();
  // Load Colormap. If the file is invalid the default stored colour map is used
  m_currentColorMapFile = settings.value("ColormapFile", "").toString();
  // Set values from settings
  if (!m_currentColorMapFile.isEmpty())
    loadColorMap(m_currentColorMapFile);
  m_colorBar->setLog(scaleType);
  // Last saved image file
  m_lastSavedFile = settings.value("LastSavedImagePath", "").toString();

  bool transparentZeros = settings.value("TransparentZeros", 1).toInt();
  this->setTransparentZeros(transparentZeros);

  int norm = settings.value("Normalization", 1).toInt();
  Mantid::API::MDNormalization normaliz =
      static_cast<Mantid::API::MDNormalization>(norm);
  this->setNormalization(normaliz);

  settings.endGroup();
}

//------------------------------------------------------------------------------------
/** Save settings for next time. */
void SliceViewer::saveSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/SliceViewer");
  settings.setValue("ColormapFile", m_currentColorMapFile);
  settings.setValue("LogColorScale", (int)m_colorBar->getLog());
  settings.setValue("LastSavedImagePath", m_lastSavedFile);
  settings.setValue("TransparentZeros",
                    (m_actionTransparentZeros->isChecked() ? 1 : 0));
  settings.setValue("Normalization",
                    static_cast<int>(this->getNormalization()));
  settings.endGroup();
}

//------------------------------------------------------------------------------------
/** Create the menus */
void SliceViewer::initMenus() {
  // ---------------------- Build the menu bar -------------------------

  // Find the top-level parent
  QWidget *widget = this;
  while (widget && widget->parentWidget())
    widget = widget->parentWidget();
  QMainWindow *parentWindow = dynamic_cast<QMainWindow *>(widget);

  QMenuBar *bar;
  if (parentWindow)
    // Use the QMainWindow menu bar
    bar = parentWindow->menuBar();
  else {
    // Widget is not in a QMainWindow. Make a menu bar
    bar = new QMenuBar(this, "Main Menu Bar");
    ui.verticalLayout->insertWidget(0, bar);
  }

  QAction *action;

  // --------------- File Menu ----------------------------------------
  m_menuFile = new QMenu("&File", this);
  action = new QAction(QPixmap(), "&Close", this);
  connect(action, SIGNAL(triggered()), this, SLOT(close()));
  m_actionFileClose = action;
  m_menuFile->addAction(action);

  action = new QAction(QPixmap(), "&Save to image file", this);
  action->setShortcut(Qt::Key_S + Qt::ControlModifier);
  connect(action, SIGNAL(triggered()), this, SLOT(saveImage()));
  m_menuFile->addAction(action);

  action = new QAction(QPixmap(), "Copy image to &Clipboard", this);
  action->setShortcut(Qt::Key_C + Qt::ControlModifier);
  connect(action, SIGNAL(triggered()), this, SLOT(copyImageToClipboard()));
  m_menuFile->addAction(action);

  // --------------- View Menu ----------------------------------------
  m_menuView = new QMenu("&View", this);
  action = new QAction(QPixmap(), "&Reset Zoom", this);
  connect(action, SIGNAL(triggered()), this, SLOT(resetZoom()));
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/SliceViewer/icons/view-fullscreen.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    action->setIcon(icon);
  }
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "&Set X/Y View Size", this);
  connect(action, SIGNAL(triggered()), this, SLOT(setXYLimitsDialog()));
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "Zoom &In", this);
  action->setShortcut(Qt::Key_Plus + Qt::ControlModifier);
  connect(action, SIGNAL(triggered()), this, SLOT(zoomInSlot()));
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "Zoom &Out", this);
  action->setShortcut(Qt::Key_Minus + Qt::ControlModifier);
  connect(action, SIGNAL(triggered()), this, SLOT(zoomOutSlot()));
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "&Fast Rendering Mode", this);
  action->setShortcut(Qt::Key_F + Qt::ControlModifier);
  action->setCheckable(true);
  action->setChecked(true);
  connect(action, SIGNAL(toggled(bool)), this, SLOT(setFastRender(bool)));
  m_menuView->addAction(action);

  m_menuView->addSeparator();

  action = new QAction(QPixmap(), "Dynamic R&ebin Mode", this);
  m_syncRebinMode = new SyncedCheckboxes(action, ui.btnRebinMode, false);
  connect(m_syncRebinMode, SIGNAL(toggled(bool)), this,
          SLOT(RebinMode_toggled(bool)));
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "&Lock Rebinned WS", this);
  m_syncRebinLock = new SyncedCheckboxes(action, ui.btnRebinLock, true);
  connect(m_syncRebinLock, SIGNAL(toggled(bool)), this,
          SLOT(RebinLock_toggled(bool)));
  m_menuView->addAction(action);

  action = new QAction(QPixmap(), "Refresh Rebin", this);
  action->setShortcut(Qt::Key_R + Qt::ControlModifier);
  action->setEnabled(false);
  connect(action, SIGNAL(triggered()), this, SLOT(rebinParamsChanged()));
  m_menuView->addAction(action);
  m_actionRefreshRebin = action;

  action = new QAction(QPixmap(), "Auto Rebin", this);
  m_syncAutoRebin = new SyncedCheckboxes(action, ui.btnAutoRebin, false);
  connect(action, SIGNAL(toggled(bool)), this, SLOT(autoRebin_toggled(bool)));
  m_syncAutoRebin->setEnabled(false); // Cannot auto rebin by default.
  m_menuView->addAction(action);

  m_menuView->addSeparator();

  QActionGroup *group = new QActionGroup(this);

  action = new QAction(QPixmap(), "No Normalization", this);
  m_menuView->addAction(action);
  action->setActionGroup(group);
  action->setCheckable(true);
  connect(action, SIGNAL(triggered()), this, SLOT(changeNormalizationNone()));
  m_actionNormalizeNone = action;

  action = new QAction(QPixmap(), "Volume Normalization", this);
  m_menuView->addAction(action);
  action->setActionGroup(group);
  action->setCheckable(true);
  action->setChecked(true);
  connect(action, SIGNAL(triggered()), this, SLOT(changeNormalizationVolume()));
  m_actionNormalizeVolume = action;

  action = new QAction(QPixmap(), "Num. Events Normalization", this);
  m_menuView->addAction(action);
  action->setActionGroup(group);
  action->setCheckable(true);
  connect(action, SIGNAL(triggered()), this,
          SLOT(changeNormalizationNumEvents()));
  m_actionNormalizeNumEvents = action;

  // --------------- Color options Menu ----------------------------------------
  m_menuColorOptions = new QMenu("&ColorMap", this);

  action = new QAction(QPixmap(), "&Load Colormap", this);
  connect(action, SIGNAL(triggered()), this, SLOT(loadColorMapSlot()));
  m_menuColorOptions->addAction(action);

  action = new QAction(QPixmap(), "&Full range", this);
  connect(action, SIGNAL(triggered()), this, SLOT(setColorScaleAutoFull()));
  {
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/SliceViewer/icons/color-pallette.png"),
                 QSize(), QIcon::Normal, QIcon::Off);
    action->setIcon(icon);
  }
  m_menuColorOptions->addAction(action);

  action = new QAction(QPixmap(), "&Slice range", this);
  connect(action, SIGNAL(triggered()), this, SLOT(setColorScaleAutoSlice()));
  action->setIconVisibleInMenu(true);
  {
    QIcon icon;
    icon.addFile(
        QString::fromUtf8(":/SliceViewer/icons/color-pallette-part.png"),
        QSize(), QIcon::Normal, QIcon::Off);
    action->setIcon(icon);
  }
  m_menuColorOptions->addAction(action);

  action = new QAction(QPixmap(), "Transparent &Zeros", this);
  action->setCheckable(true);
  action->setChecked(true);
  m_actionTransparentZeros = action;
  connect(action, SIGNAL(toggled(bool)), this, SLOT(setTransparentZeros(bool)));
  m_menuColorOptions->addAction(action);

  // --------------- Help Menu ----------------------------------------
  m_menuHelp = new QMenu("&Help", this);
  action = new QAction(QPixmap(), "&Slice Viewer Help (browser)", this);
  action->setShortcut(Qt::Key_F1);
  connect(action, SIGNAL(triggered()), this, SLOT(helpSliceViewer()));
  m_menuHelp->addAction(action);

  action = new QAction(QPixmap(), "&Line Viewer Help (browser)", this);
  connect(action, SIGNAL(triggered()), this, SLOT(helpLineViewer()));
  m_menuHelp->addAction(action);

  action = new QAction(QPixmap(), "&Peaks Viewer Help (browser)", this);
  connect(action, SIGNAL(triggered()), this, SLOT(helpPeaksViewer()));
  m_menuHelp->addAction(action);

  // --------------- Line Menu ----------------------------------------
  m_menuLine = new QMenu("&Line", this);

  // Line mode menu, synced to the button
  action = new QAction(QPixmap(), "&Line Mode", this);
  action->setShortcut(Qt::Key_L + Qt::ControlModifier);
  m_syncLineMode = new SyncedCheckboxes(action, ui.btnDoLine, false);
  connect(m_syncLineMode, SIGNAL(toggled(bool)), this,
          SLOT(LineMode_toggled(bool)));
  m_menuLine->addAction(action);

  // Snap-to-grid, synced to the button
  action = new QAction(QPixmap(), "&Snap to Grid", this);
  m_syncSnapToGrid = new SyncedCheckboxes(action, ui.btnSnapToGrid, false);
  connect(m_syncSnapToGrid, SIGNAL(toggled(bool)), this,
          SLOT(SnapToGrid_toggled(bool)));
  m_menuLine->addAction(action);

  // --------------- Peaks Menu ----------------------------------------
  m_menuPeaks = new QMenu("&Peak", this);
  action = new QAction(QPixmap(), "&Overlay Options", this);
  connect(action, SIGNAL(triggered()), this,
          SLOT(onPeaksViewerOverlayOptions()));
  m_menuPeaks->addAction(action);
  action = new QAction(QPixmap(), "&Visible Columns", this);
  connect(action, SIGNAL(triggered()), this,
          SIGNAL(peaksTableColumnOptions())); // just re-emit
  m_menuPeaks->addAction(action);
  m_menuPeaks->setEnabled(false); // Until a PeaksWorkspace is selected.

  // Add all the needed menus
  bar->addMenu(m_menuFile);
  bar->addMenu(m_menuView);
  bar->addMenu(m_menuColorOptions);
  bar->addMenu(m_menuLine);
  bar->addMenu(m_menuPeaks);
  bar->addMenu(m_menuHelp);
}

//------------------------------------------------------------------------------------
/** Intialize the zooming/panning tools */
void SliceViewer::initZoomer() {
  //  QwtPlotZoomer * zoomer = new CustomZoomer(m_plot->canvas());
  //  zoomer->setMousePattern(QwtEventPattern::MouseSelect1,  Qt::LeftButton);
  //  zoomer->setTrackerMode(QwtPicker::AlwaysOff);
  //  const QColor c(Qt::darkBlue);
  //  zoomer->setRubberBandPen(c);
  //  zoomer->setTrackerPen(c);
  //  QObject::connect(zoomer, SIGNAL(zoomed(const QRectF &)),
  //      this, SLOT(zoomRectSlot(const QRectF &)));

  QwtPlotPicker *zoomer = new QwtPlotPicker(m_plot->canvas());
  zoomer->setSelectionFlags(QwtPicker::RectSelection |
                            QwtPicker::DragSelection);
  zoomer->setMousePattern(QwtEventPattern::MouseSelect1, Qt::LeftButton);
  zoomer->setTrackerMode(QwtPicker::AlwaysOff);
  const QColor c(Qt::darkBlue);
  zoomer->setRubberBand(QwtPicker::RectRubberBand);
  zoomer->setRubberBandPen(c);
  QObject::connect(zoomer, SIGNAL(selected(const QwtDoubleRect &)), this,
                   SLOT(zoomRectSlot(const QwtDoubleRect &)));

  // Zoom in/out using middle-click+drag or the mouse wheel
  CustomMagnifier *magnif = new CustomMagnifier(m_plot->canvas());
  magnif->setAxisEnabled(QwtPlot::yRight, false); // Don't do the colorbar axis
  magnif->setWheelFactor(0.9);
  magnif->setMouseButton(Qt::MidButton);
  // Have to flip the keys to match our flipped mouse wheel
  magnif->setZoomInKey(Qt::Key_Minus, Qt::NoModifier);
  magnif->setZoomOutKey(Qt::Key_Equal, Qt::NoModifier);
  // Hook-up listener to rescaled event
  QObject::connect(magnif, SIGNAL(rescaled(double)), this,
                   SLOT(magnifierRescaled(double)));

  // Pan using the right mouse button + drag
  QwtPlotPanner *panner = new QwtPlotPanner(m_plot->canvas());
  panner->setMouseButton(Qt::RightButton);
  panner->setAxisEnabled(QwtPlot::yRight, false); // Don't do the colorbar axis
  QObject::connect(panner, SIGNAL(panned(int, int)), this,
                   SLOT(panned(int, int))); // Handle panning.

  // Custom picker for showing the current coordinates
  CustomPicker *picker =
      new CustomPicker(m_spect->xAxis(), m_spect->yAxis(), m_plot->canvas());
  QObject::connect(picker, SIGNAL(mouseMoved(double, double)), this,
                   SLOT(showInfoAt(double, double)));
}

//------------------------------------------------------------------------------------
/** Programmatically show/hide the controls (sliders etc)
 *
 * @param visible :: true if you want to show the controls.
 */
void SliceViewer::showControls(bool visible) {
  ui.frmControls->setVisible(visible);
}

//------------------------------------------------------------------------------------
/** Add (as needed) and update DimensionSliceWidget's. */
void SliceViewer::updateDimensionSliceWidgets() {
  // Create all necessary widgets
  if (m_dimWidgets.size() < m_ws->getNumDims()) {
    for (size_t d = m_dimWidgets.size(); d < m_ws->getNumDims(); d++) {
      DimensionSliceWidget *widget = new DimensionSliceWidget(this);

      ui.verticalLayoutControls->insertWidget(int(d), widget);

      // Slots for changes on the dimension widget
      QObject::connect(widget, SIGNAL(changedShownDim(int, int, int)), this,
                       SLOT(changedShownDim(int, int, int)));
      QObject::connect(widget, SIGNAL(changedSlicePoint(int, double)), this,
                       SLOT(updateDisplaySlot(int, double)));
      // Slots for dynamic rebinning
      QObject::connect(widget, SIGNAL(changedThickness(int, double)), this,
                       SLOT(rebinParamsChanged()));
      QObject::connect(widget, SIGNAL(changedNumBins(int, int)), this,
                       SLOT(rebinParamsChanged()));

      // Save in this list
      m_dimWidgets.push_back(widget);
    }
  }
  // Hide unnecessary ones
  for (size_t d = m_ws->getNumDims(); d < m_dimWidgets.size(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    widget->hide();
  }

  int maxLabelWidth = 10;
  int maxUnitsWidth = 10;
  // Set each dimension
  for (size_t d = 0; d < m_dimensions.size(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    widget->blockSignals(true);

    widget->setDimension(int(d), m_dimensions[d]);
    // Default slicing layout
    if (d == m_dimX)
      widget->setShownDim(0);
    else if (d == m_dimY)
      widget->setShownDim(1);
    else
      widget->setShownDim(-1);

    // To harmonize the layout, find the largest label
    int w;
    w = widget->ui.lblName->sizeHint().width();
    if (w > maxLabelWidth)
      maxLabelWidth = w;
    w = widget->ui.lblUnits->sizeHint().width();
    if (w > maxUnitsWidth)
      maxUnitsWidth = w;

    widget->blockSignals(false);
  }

  // Make the labels all the same width
  for (size_t d = 0; d < m_ws->getNumDims(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    widget->ui.lblName->setMinimumSize(QSize(maxLabelWidth, 0));
    widget->ui.lblUnits->setMinimumSize(QSize(maxUnitsWidth, 0));
  }
}

//------------------------------------------------------------------------------------
/** Set the displayed workspace. Updates UI.
 *
 * @param ws :: IMDWorkspace to show.
 */
void SliceViewer::setWorkspace(Mantid::API::IMDWorkspace_sptr ws) {
  m_ws = ws;
  m_data->setWorkspace(ws);
  m_plot->setWorkspace(ws);

  // Only allow perpendicular lines if looking at a matrix workspace.
  bool matrix = bool(boost::dynamic_pointer_cast<MatrixWorkspace>(m_ws));
  m_lineOverlay->setAngleSnapMode(matrix);
  m_lineOverlay->setAngleSnap(matrix ? 90 : 45);

  // Can't use dynamic rebin mode with a MatrixWorkspace
  m_syncRebinMode->setEnabled(!matrix);
  m_syncRebinLock->setEnabled(!matrix);

  // Go to no normalization by default for MatrixWorkspaces
  if (matrix)
    this->setNormalization(Mantid::API::NoNormalization,
                           false /* without updating */);

  // Emit the signal that we changed the workspace
  emit workspaceChanged();

  // For MDEventWorkspace, estimate the resolution and change the # of bins
  // accordingly
  IMDEventWorkspace_sptr mdew =
      boost::dynamic_pointer_cast<IMDEventWorkspace>(m_ws);
  std::vector<coord_t> binSizes = m_ws->estimateResolution();

  // Copy the dimensions to this so they can be modified
  m_dimensions.clear();
  std::ostringstream mess;
  for (size_t d = 0; d < m_ws->getNumDims(); d++) {
    // Choose the number of bins based on the resolution of the workspace (for
    // MDEWs)
    coord_t min = m_ws->getDimension(d)->getMinimum();
    coord_t max = m_ws->getDimension(d)->getMaximum();
    if (boost::math::isnan(min) || boost::math::isinf(min) ||
        boost::math::isnan(max) || boost::math::isinf(max)) {
      mess << "Dimension " << m_ws->getDimension(d)->getName()
           << " has a bad range: (";
      mess << min << ", " << max << ")" << std::endl;
    }
    size_t numBins = static_cast<size_t>((max - min) / binSizes[d]);
    MDHistoDimension_sptr dim(
        new MDHistoDimension(m_ws->getDimension(d).get()));
    dim->setRange(numBins, min, max);
    m_dimensions.push_back(dim);
  }

  if (!mess.str().empty()) {
    mess << "Bad ranges could cause memory allocation errors. Please fix the "
            "workspace.";
    mess << std::endl << "You can continue using Mantid.";
    throw std::out_of_range(mess.str());
  }

  // Adjust the range to that of visible data
  if (mdew) {
    std::vector<Mantid::Geometry::MDDimensionExtents<coord_t>> ext =
        mdew->getMinimumExtents();
    for (size_t d = 0; d < mdew->getNumDims(); d++) {
      size_t newNumBins =
          size_t(ext[d].getSize() / m_dimensions[d]->getBinWidth() + 1);
      m_dimensions[d]->setRange(newNumBins, ext[d].getMin(), ext[d].getMax());
    }
  }

  // Build up the widgets
  this->updateDimensionSliceWidgets();

  // Find the full range. And use it
  findRangeFull();
  m_colorBar->setViewRange(m_colorRangeFull);
  // Initial display update
  this->updateDisplay(
      !m_firstWorkspaceOpen /*Force resetting the axes, the first time*/);

  // Don't reset axes next time
  m_firstWorkspaceOpen = true;

  // For showing the original coordinates
  ui.frmMouseInfo->setVisible(false);
  if (m_ws->hasOriginalWorkspace()) {
    IMDWorkspace_sptr origWS =
        boost::dynamic_pointer_cast<IMDWorkspace>(m_ws->getOriginalWorkspace());
    CoordTransform *toOrig = m_ws->getTransformToOriginal();
    if (toOrig) {
      ui.frmMouseInfo->setVisible(true);
      ui.lblOriginalWorkspace->setText(
          QString::fromStdString("in '" + origWS->getName() + "'"));
    }
  }

  // Enable peaks overlays according to the dimensionality and the displayed
  // dimensions.
  enablePeakOverlaysIfAppropriate();

  // Send out a signal
  emit changedShownDim(m_dimX, m_dimY);
}

//------------------------------------------------------------------------------------
/** Set the workspace to view using its name.
 * The workspace should be a MDHistoWorkspace or a MDEventWorkspace,
 * with at least 2 dimensions.
 *
 * @param wsName :: name of the MDWorkspace to look for
 * @throw std::runtime_error if the workspace is not found or is a
 *MatrixWorkspace
 */
void SliceViewer::setWorkspace(const QString &wsName) {
  IMDWorkspace_sptr ws = boost::dynamic_pointer_cast<IMDWorkspace>(
      AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  if (!ws)
    throw std::runtime_error("SliceViewer can only view MDWorkspaces.");
  if (boost::dynamic_pointer_cast<MatrixWorkspace>(ws))
    throw std::runtime_error(
        "SliceViewer cannot view MatrixWorkspaces. "
        "Please select a MDEventWorkspace or a MDHistoWorkspace.");
  this->setWorkspace(ws);
}

//------------------------------------------------------------------------------------
/** @return the workspace in the SliceViewer */
Mantid::API::IMDWorkspace_sptr SliceViewer::getWorkspace() { return m_ws; }

//------------------------------------------------------------------------------------
/** Load a color map from a file
 *
 * @param filename :: file to open; empty to ask via a dialog box.
 */
void SliceViewer::loadColorMap(QString filename) {
  QString fileselection;
  if (filename.isEmpty()) {
    fileselection = MantidColorMap::loadMapDialog(m_currentColorMapFile, this);
    if (fileselection.isEmpty())
      return;
  } else
    fileselection = filename;

  m_currentColorMapFile = fileselection;

  // Load from file
  m_colorBar->getColorMap().loadMap(fileselection);
  m_spect->setColorMap(m_colorBar->getColorMap());
  m_colorBar->updateColorMap();
  this->updateDisplay();
}

//=================================================================================================
//========================================== SLOTS
//================================================
//=================================================================================================

//------------------------------------------------------------------------------------
/** Automatically sets the min/max of the color scale,
 * using the limits in the entire data set of the workspace
 * (every bin, even those not currently visible).
 */
void SliceViewer::setColorScaleAutoFull() {
  this->findRangeFull();
  m_colorBar->setViewRange(m_colorRangeFull);
  this->updateDisplay();
}

//------------------------------------------------------------------------------------
/** Automatically sets the min/max of the color scale,
 * using the limits in the data that is currently visible
 * in the plot (only the bins in this slice and within the
 * view limits)
 */
void SliceViewer::setColorScaleAutoSlice() {
  this->findRangeSlice();
  m_colorBar->setViewRange(m_colorRangeSlice);
  this->updateDisplay();
}

//------------------------------------------------------------------------------------
/// Slot called when the ColorBarWidget changes the range of colors
void SliceViewer::colorRangeChanged() {
  m_spect->setColorMap(m_colorBar->getColorMap());
  this->updateDisplay();
}

//------------------------------------------------------------------------------------
/** Set whether to display 0 signal as "transparent" color.
 *
 * @param transparent :: true if you want zeros to be transparent.
 */
void SliceViewer::setTransparentZeros(bool transparent) {
  m_actionTransparentZeros->blockSignals(true);
  m_actionTransparentZeros->setChecked(transparent);
  m_actionTransparentZeros->blockSignals(false);
  // Set and display
  m_data->setZerosAsNan(transparent);
  this->updateDisplay();
}

//------------------------------------------------------------------------------------
/// Slot called when changing the normalization menu
void SliceViewer::changeNormalizationNone() {
  this->setNormalization(Mantid::API::NoNormalization, true);
}

void SliceViewer::changeNormalizationVolume() {
  this->setNormalization(Mantid::API::VolumeNormalization, true);
}

void SliceViewer::changeNormalizationNumEvents() {
  this->setNormalization(Mantid::API::NumEventsNormalization, true);
}

//------------------------------------------------------------------------------------
/** Set the normalization mode for viewing the data
 *
 * @param norm :: MDNormalization enum. 0=none; 1=volume; 2=# of events
 * @param update :: update the displayed image. If false, just sets it and shows
 *the checkboxes.
 */
void SliceViewer::setNormalization(Mantid::API::MDNormalization norm,
                                   bool update) {
  m_actionNormalizeNone->blockSignals(true);
  m_actionNormalizeVolume->blockSignals(true);
  m_actionNormalizeNumEvents->blockSignals(true);

  m_actionNormalizeNone->setChecked(norm == Mantid::API::NoNormalization);
  m_actionNormalizeVolume->setChecked(norm == Mantid::API::VolumeNormalization);
  m_actionNormalizeNumEvents->setChecked(norm ==
                                         Mantid::API::NumEventsNormalization);

  m_actionNormalizeNone->blockSignals(false);
  m_actionNormalizeVolume->blockSignals(false);
  m_actionNormalizeNumEvents->blockSignals(false);

  m_data->setNormalization(norm);
  if (update)
    this->updateDisplay();
}

//------------------------------------------------------------------------------------
/** @return the current normalization */
Mantid::API::MDNormalization SliceViewer::getNormalization() const {
  return m_data->getNormalization();
}

//------------------------------------------------------------------------------------
/** Set the thickness (above and below the plane) for dynamic rebinning.
 *
 * @param dim :: index of the dimension to adjust
 * @param thickness :: thickness to set, in units of the dimension.
 * @throw runtime_error if the dimension index is invalid or the thickness is <=
 *0.0.
 */
void SliceViewer::setRebinThickness(int dim, double thickness) {
  if (dim < 0 || dim >= static_cast<int>(m_dimWidgets.size()))
    throw std::runtime_error(
        "SliceViewer::setRebinThickness(): Invalid dimension index");
  if (thickness <= 0.0)
    throw std::runtime_error(
        "SliceViewer::setRebinThickness(): Thickness must be > 0.0");
  m_dimWidgets[dim]->setThickness(thickness);
}

//------------------------------------------------------------------------------------
/** Set the number of bins for dynamic rebinning.
 *
 * @param xBins :: number of bins in the viewed X direction
 * @param yBins :: number of bins in the viewed Y direction
 * @throw runtime_error if the number of bins is < 1
 */
void SliceViewer::setRebinNumBins(int xBins, int yBins) {
  if (xBins < 1 || yBins < 1)
    throw std::runtime_error(
        "SliceViewer::setRebinNumBins(): Number of bins must be >= 1");
  m_dimWidgets[m_dimX]->setNumBins(xBins);
  m_dimWidgets[m_dimY]->setNumBins(yBins);
}

//------------------------------------------------------------------------------------
/** Sets the SliceViewer in dynamic rebin mode.
 * In this mode, the current view area (see setXYLimits()) is used as the
 * limits to rebin.
 * See setRebinNumBins() to adjust the number of bins in the X/Y dimensions.
 * See setRebinThickness() to adjust the thickness in other dimensions.
 *
 * @param mode :: true for rebinning mode
 * @param locked :: if true, then the rebinned area is only refreshed manually
 *        or when changing rebinning parameters.
 */
void SliceViewer::setRebinMode(bool mode, bool locked) {
  // The events associated with these controls will trigger a re-draw
  m_syncRebinMode->toggle(mode);
  m_syncRebinLock->toggle(locked);
}

//------------------------------------------------------------------------------------
/** When in dynamic rebinning mode, this refreshes the rebinned area to be the
 * currently viewed area. See setXYLimits(), setRebinNumBins(),
 * setRebinThickness()
 */
void SliceViewer::refreshRebin() { this->rebinParamsChanged(); }

//------------------------------------------------------------------------------------
/// Slot called when the btnDoLine button is checked/unchecked
void SliceViewer::LineMode_toggled(bool checked) {
  m_lineOverlay->setShown(checked);
  if (checked) {
    QString text;
    if (m_lineOverlay->getCreationMode())
      text = "Click and drag to draw an integration line.\n"
             "Hold Shift key to limit to 45 degree angles.";
    else
      text = "Drag the existing line with its handles,\n"
             "or click the red X to delete it.";
    // Show a tooltip near the button
    QToolTip::showText(ui.btnDoLine->mapToGlobal(ui.btnDoLine->pos()), text,
                       this);
  }
  emit showLineViewer(checked);
}

//------------------------------------------------------------------------------------
/** Toggle "line-drawing" mode (to draw 1D lines using the mouse)
 *
 * @param lineMode :: True to go into line mode, False to exit it.
 */
void SliceViewer::toggleLineMode(bool lineMode) {
  // This should send events to start line mode
  m_syncLineMode->toggle(lineMode);
  m_lineOverlay->setCreationMode(false);
}

//------------------------------------------------------------------------------------
/// Slot called to clear the line in the line overlay
void SliceViewer::clearLine() {
  m_lineOverlay->reset();
  m_plot->update();
}

//------------------------------------------------------------------------------------
/// Slot called when the snap to grid is checked
void SliceViewer::SnapToGrid_toggled(bool checked) {
  if (checked) {
    SnapToGridDialog *dlg = new SnapToGridDialog(this);
    dlg->setSnap(m_lineOverlay->getSnapX(), m_lineOverlay->getSnapY());
    if (dlg->exec() == QDialog::Accepted) {
      m_lineOverlay->setSnapEnabled(true);
      m_lineOverlay->setSnapX(dlg->getSnapX());
      m_lineOverlay->setSnapY(dlg->getSnapY());
    } else {
      // Uncheck - the user clicked cancel
      ui.btnSnapToGrid->setChecked(false);
      m_lineOverlay->setSnapEnabled(false);
    }
  } else {
    m_lineOverlay->setSnapEnabled(false);
  }
}

//------------------------------------------------------------------------------------
/** Slot called when going into or out of dynamic rebinning mode */
void SliceViewer::RebinMode_toggled(bool checked) {
  for (size_t d = 0; d < m_dimWidgets.size(); d++)
    m_dimWidgets[d]->showRebinControls(checked);
  ui.btnRebinRefresh->setEnabled(checked);
  ui.btnRebinLock->setEnabled(checked);
  m_syncAutoRebin->setEnabled(checked);
  m_actionRefreshRebin->setEnabled(checked);
  m_rebinMode = checked;

  if (!m_rebinMode) {
    // uncheck auto-rebin
    ui.btnAutoRebin->setChecked(false);
    // Remove the overlay WS
    this->m_overlayWS.reset();
    this->m_data->setOverlayWorkspace(m_overlayWS);
    this->updateDisplay();
  } else {
    // Start the rebin
    this->rebinParamsChanged();
  }
}

//------------------------------------------------------------------------------------
/** Slot called when locking/unlocking the dynamically rebinned
 * overlaid workspace
 * @param checked :: DO lock the workspace in place
 */
void SliceViewer::RebinLock_toggled(bool checked) {
  m_rebinLocked = checked;
  // Rebin immediately
  if (!m_rebinLocked && m_rebinMode)
    this->rebinParamsChanged();
}

//------------------------------------------------------------------------------------
/// Slot for zooming into
void SliceViewer::zoomInSlot() { this->zoomBy(1.1); }

/// Slot for zooming out
void SliceViewer::zoomOutSlot() { this->zoomBy(1.0 / 1.1); }

/** Slot called when zooming using QwtPlotZoomer (rubber-band method)
 *
 * @param rect :: rectangle to zoom to
 */
void SliceViewer::zoomRectSlot(const QwtDoubleRect &rect) {
  if ((rect.width() == 0) || (rect.height() == 0))
    return;
  this->setXYLimits(rect.left(), rect.right(), rect.top(), rect.bottom());
  autoRebinIfRequired();
}

/// Slot for opening help page
void SliceViewer::helpSliceViewer() {
  QString helpPage = "MantidPlot:_SliceViewer";
  QDesktopServices::openUrl(
      QUrl(QString("http://www.mantidproject.org/") + helpPage));
}

/// Slot for opening help page
void SliceViewer::helpLineViewer() {
  QString helpPage = "MantidPlot:_LineViewer";
  QDesktopServices::openUrl(
      QUrl(QString("http://www.mantidproject.org/") + helpPage));
}

void SliceViewer::helpPeaksViewer() {
  QString helpPage = "PeaksViewer";
  QDesktopServices::openUrl(
      QUrl(QString("http://www.mantidproject.org/") + helpPage));
}

//------------------------------------------------------------------------------------
/** Automatically resets the zoom view to full axes.
 * This will reset the XY limits to the full range of the workspace.
 * Use zoomBy() or setXYLimits() to modify the view range.
 * This corresponds to the "View Extents" button.
 */
void SliceViewer::resetZoom() {
  // Reset the 2 axes to full scale
  resetAxis(m_spect->xAxis(), m_X);
  resetAxis(m_spect->yAxis(), m_Y);
  // Make sure the view updates
  m_plot->replot();
  autoRebinIfRequired();
  updatePeaksOverlay();
}

//------------------------------------------------------------------------------------
/// SLOT to open a dialog to set the XY limits
void SliceViewer::setXYLimitsDialog() {
  // Initialize the dialog with the current values
  XYLimitsDialog *dlg = new XYLimitsDialog(this);
  dlg->setXDim(m_X);
  dlg->setYDim(m_Y);
  QwtDoubleInterval xint = this->getXLimits();
  QwtDoubleInterval yint = this->getYLimits();
  dlg->setLimits(xint.minValue(), xint.maxValue(), yint.minValue(),
                 yint.maxValue());
  // Show the dialog
  if (dlg->exec() == QDialog::Accepted) {
    this->setXYLimits(dlg->getXMin(), dlg->getXMax(), dlg->getYMin(),
                      dlg->getYMax());
  }
}

//------------------------------------------------------------------------------------
/** Slot to redraw when the slice point changes */
void SliceViewer::updateDisplaySlot(int index, double value) {
  UNUSED_ARG(index)
  UNUSED_ARG(value)
  this->updateDisplay();
  // Trigger a rebin on each movement of the slice point
  if (m_rebinMode && !m_rebinLocked)
    this->rebinParamsChanged();
}

//------------------------------------------------------------------------------------
/** SLOT to open a dialog to choose a file, load a color map from that file */
void SliceViewer::loadColorMapSlot() { this->loadColorMap(QString()); }

//------------------------------------------------------------------------------------
/** Grab the 2D view as an image. The image is rendered at the current window
 * size, with the color scale but without the text boxes for changing them.
 *
 * See also saveImage() and copyImageToClipboard()
 *
 * @return QPixmap containing the image.
 */
QPixmap SliceViewer::getImage() {

  // Switch to full resolution rendering
  bool oldFast = this->getFastRender();
  this->setFastRender(false);
  // Hide the line overlay handles
  this->m_lineOverlay->setShowHandles(false);
  this->m_colorBar->setRenderMode(true);

  // Grab it
  QCoreApplication::processEvents();
  QCoreApplication::processEvents();
  QPixmap pix = QPixmap::grabWidget(this->ui.frmPlot);

  // Back to previous mode
  this->m_lineOverlay->setShowHandles(true);
  this->m_colorBar->setRenderMode(false);
  this->setFastRender(oldFast);

  return pix;
}

//------------------------------------------------------------------------------------
/** Copy the rendered 2D image to the clipboard
 */
void SliceViewer::copyImageToClipboard() {
  // Create the image
  QPixmap pix = this->getImage();
  // Set the clipboard
  QApplication::clipboard()->setImage(pix, QClipboard::Clipboard);
}

/**
 * Adds .png extension if not already included
 *
 * @param fname :: a file name to save an (png) image
 * @return input file name with '.png' appended if needed
 **/
QString SliceViewer::ensurePngExtension(const QString &fname) const {
  const QString goodExt = "png";

  QString res = fname;
  if (QFileInfo(fname).suffix() != goodExt) {
    res = res + "." + goodExt;
  }
  return res;
}

//------------------------------------------------------------------------------------
/** Save the rendered 2D slice to an image file.
 *
 * @param filename :: full path to the file to save, including extension
 *        (e.g. .png). If not specified or empty, then a dialog will prompt
 *        the user to pick a file.
 */
void SliceViewer::saveImage(const QString &filename) {
  QString fileselection;
  if (filename.isEmpty()) {
    fileselection = MantidQt::API::FileDialogHandler::getSaveFileName(
        this, tr("Pick a file to which to save the image"),
        QFileInfo(m_lastSavedFile).absoluteFilePath(),
        tr("PNG files(*.png *.png)"));
    // User cancelled if filename is still empty
    if (fileselection.isEmpty())
      return;
    m_lastSavedFile = fileselection;
  } else
    fileselection = filename;

  // append '.png' if needed
  QString finalName = ensurePngExtension(fileselection);

  // Create the image
  QPixmap pix = this->getImage();
  // And save to the file
  pix.save(finalName);
}

//=================================================================================================
//=================================================================================================
//=================================================================================================
/** Zoom in or out, keeping the center of the plot in the same position.
 *
 * @param factor :: double, if > 1 : zoom in by this factor.
 *                  if < 1 : it will zoom out.
 */
void SliceViewer::zoomBy(double factor) {
  QwtDoubleInterval xint = this->getXLimits();
  QwtDoubleInterval yint = this->getYLimits();

  double newHalfWidth = (xint.width() / factor) * 0.5;
  double middle = (xint.minValue() + xint.maxValue()) * 0.5;
  double x_min = middle - newHalfWidth;
  double x_max = middle + newHalfWidth;

  newHalfWidth = (yint.width() / factor) * 0.5;
  middle = (yint.minValue() + yint.maxValue()) * 0.5;
  double y_min = middle - newHalfWidth;
  double y_max = middle + newHalfWidth;
  // Perform the move
  this->setXYLimits(x_min, x_max, y_min, y_max);
  autoRebinIfRequired();

  // Peaks in region will change.
  this->updatePeaksOverlay();
}

//------------------------------------------------------------------------------------
/** Manually set the center of the plot, in X Y coordinates.
 * This keeps the plot the same size as previously.
 * Use setXYLimits() to modify the size of the plot by setting the X/Y edges,
 * or you can use zoomBy() to zoom in/out
 *
 * @param x :: new position of the center in X
 * @param y :: new position of the center in Y
 */
void SliceViewer::setXYCenter(double x, double y) {
  QwtDoubleInterval xint = this->getXLimits();
  QwtDoubleInterval yint = this->getYLimits();
  double halfWidthX = xint.width() * 0.5;
  double halfWidthY = yint.width() * 0.5;
  // Perform the move
  this->setXYLimits(x - halfWidthX, x + halfWidthX, y - halfWidthY,
                    y + halfWidthY);
}

//------------------------------------------------------------------------------------
/** Reset the axis and scale it
 *
 * @param axis :: int for X or Y
 * @param dim :: dimension to show
 */
void SliceViewer::resetAxis(int axis, const IMDDimension_const_sptr &dim) {
  m_plot->setAxisScale(axis, dim->getMinimum(), dim->getMaximum());
  m_plot->setAxisTitle(axis, API::PlotAxis(*dim).title());
}

//------------------------------------------------------------------------------------
/// Find the full range of values in the workspace
void SliceViewer::findRangeFull() {
  IMDWorkspace_sptr workspace_used = m_ws;
  if (m_rebinMode) {
    workspace_used = this->m_overlayWS;
  }

  if (!workspace_used)
    return;

  // Acquire a scoped read-only lock on the workspace, preventing it from being
  // written
  // while we iterate through.
  ReadLock lock(*workspace_used);

  // Iterate through the entire workspace
  m_colorRangeFull =
      API::SignalRange(*workspace_used, this->getNormalization()).interval();
}

//------------------------------------------------------------------------------------
/** Find the full range of values ONLY in the currently visible
part of the workspace */
void SliceViewer::findRangeSlice() {
  IMDWorkspace_sptr workspace_used = m_ws;
  if (m_rebinMode) {
    workspace_used = this->m_overlayWS;
  }

  if (!workspace_used)
    return;
  // Acquire a scoped read-only lock on the workspace, preventing it from being
  // written
  // while we iterate through.
  ReadLock lock(*workspace_used);

  m_colorRangeSlice = QwtDoubleInterval(0., 1.0);

  // This is what is currently visible on screen
  QwtDoubleInterval xint = m_plot->axisScaleDiv(m_spect->xAxis())->interval();
  QwtDoubleInterval yint = m_plot->axisScaleDiv(m_spect->yAxis())->interval();

  // Find the min-max extents in each dimension
  VMD min(workspace_used->getNumDims());
  VMD max(workspace_used->getNumDims());
  for (size_t d = 0; d < m_dimensions.size(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    IMDDimension_const_sptr dim = m_dimensions[d];
    if (widget->getShownDim() == 0) {
      min[d] = VMD_t(xint.minValue());
      max[d] = VMD_t(xint.maxValue());
    } else if (widget->getShownDim() == 1) {
      min[d] = VMD_t(yint.minValue());
      max[d] = VMD_t(yint.maxValue());
    } else {
      // Is a slice. Take a slice of widht = binWidth
      min[d] = VMD_t(widget->getSlicePoint()) - dim->getBinWidth() * 0.45f;
      max[d] = min[d] + dim->getBinWidth();
    }
  }
  // This builds the implicit function for just this slice
  MDBoxImplicitFunction *function = new MDBoxImplicitFunction(min, max);

  // Iterate through the slice
  m_colorRangeSlice = API::SignalRange(*workspace_used, *function,
                                       this->getNormalization()).interval();
  delete function;
  // In case of failure, use the full range instead
  if (m_colorRangeSlice == QwtDoubleInterval(0.0, 1.0))
    m_colorRangeSlice = m_colorRangeFull;
}

//------------------------------------------------------------------------------------
/** Slot to show the mouse info at the mouse position
 *
 * @param x :: position of the mouse in plot coords
 * @param y :: position of the mouse in plot coords
 */
void SliceViewer::showInfoAt(double x, double y) {
  // Show the coordinates in the viewed workspace
  if (!m_ws)
    return;
  VMD coords(m_ws->getNumDims());
  for (size_t d = 0; d < m_ws->getNumDims(); d++)
    coords[d] = VMD_t(m_dimWidgets[d]->getSlicePoint());
  coords[m_dimX] = VMD_t(x);
  coords[m_dimY] = VMD_t(y);
  signal_t signal =
      m_ws->getSignalAtVMD(coords, this->m_data->getNormalization());
  ui.lblInfoX->setText(QString::number(x, 'g', 4));
  ui.lblInfoY->setText(QString::number(y, 'g', 4));
  ui.lblInfoSignal->setText(QString::number(signal, 'g', 4));

  // Now show the coords in the original workspace
  if (m_ws->hasOriginalWorkspace()) {
    IMDWorkspace_sptr origWS =
        boost::dynamic_pointer_cast<IMDWorkspace>(m_ws->getOriginalWorkspace());
    CoordTransform *toOrig = m_ws->getTransformToOriginal();
    if (toOrig) {
      // Transform the coordinates
      VMD orig = toOrig->applyVMD(coords);

      QString text;
      for (size_t d = 0; d < origWS->getNumDims(); d++) {
        text += QString::fromStdString(origWS->getDimension(d)->getName());
        text += ": ";
        text += (orig[d] < 0) ? "-" : " ";
        text += QString::number(fabs(orig[d]), 'g', 3).leftJustified(8, ' ');
        if (d != origWS->getNumDims() - 1)
          text += " ";
      }
      ui.lblOriginalCoord->setText(text);
    }
  }
}

//------------------------------------------------------------------------------------
/** Update the 2D plot using all the current controls settings */
void SliceViewer::updateDisplay(bool resetAxes) {
  if (!m_ws)
    return;
  size_t oldX = m_dimX;
  size_t oldY = m_dimY;

  m_dimX = 0;
  m_dimY = 1;
  std::vector<coord_t> slicePoint;

  for (size_t d = 0; d < m_ws->getNumDims(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    if (widget->getShownDim() == 0) {
      m_dimX = d;
    } else if (widget->getShownDim() == 1) {
      m_dimY = d;
    }
    slicePoint.push_back(VMD_t(widget->getSlicePoint()));
  }
  // Avoid going out of range
  if (m_dimX >= m_ws->getNumDims())
    m_dimX = m_ws->getNumDims() - 1;
  if (m_dimY >= m_ws->getNumDims())
    m_dimY = m_ws->getNumDims() - 1;
  m_X = m_dimensions[m_dimX];
  m_Y = m_dimensions[m_dimY];

  m_data->setSliceParams(m_dimX, m_dimY, m_X, m_Y, slicePoint);
  m_slicePoint = VMD(slicePoint);

  // Was there a change of which dimensions are shown?
  if (resetAxes || oldX != m_dimX || oldY != m_dimY) {
    this->resetAxis(m_spect->xAxis(), m_X);
    this->resetAxis(m_spect->yAxis(), m_Y);

    // The dimensionality has changed. It might no longer be possible to plot
    // peaks.
    enablePeakOverlaysIfAppropriate();

    // Transform the peak overlays according to the new plotting.
    m_peaksPresenter->changeShownDim();

    // Update the pointer to the slider widget.
    updatePeakOverlaySliderWidget();
  }

  // Set the color range
  m_data->setRange(m_colorBar->getViewRange());

  //  m_colorBar->setColorMap(m_colorRange, m_colorMap);
  //  m_plot->setAxisScale(QwtPlot::yRight, m_colorRange.minValue(),
  //  m_colorRange.maxValue() );

  // Is the overlay workspace visible at all from this slice point?
  if (m_overlayWS) {
    bool overlayInSlice = true;
    for (size_t d = 0; d < m_overlayWS->getNumDims(); d++) {
      if ((d != m_dimX && d != m_dimY) &&
          (m_slicePoint[d] < m_overlayWS->getDimension(d)->getMinimum() ||
           m_slicePoint[d] >= m_overlayWS->getDimension(d)->getMaximum()))
        overlayInSlice = false;
    }
    m_overlayWSOutline->setShown(overlayInSlice);
  }

  // Notify the graph that the underlying data changed
  m_spect->setData(*m_data);
  m_spect->itemChanged();
  m_plot->replot();

  // Peaks overlays may need redrawing
  updatePeaksOverlay();

  // Send out a signal
  emit changedSlicePoint(m_slicePoint);
}

//------------------------------------------------------------------------------------
/** The user changed the shown dimension somewhere.
 *
 * @param index :: index of the dimension
 * @param dim :: shown dimension, 0=X, 1=Y, -1 sliced
 * @param oldDim :: previous shown dimension, 0=X, 1=Y, -1 sliced
 */
void SliceViewer::changedShownDim(int index, int dim, int oldDim) {
  if (dim >= 0) {
    // Swap from X to Y
    if (oldDim >= 0 && oldDim != dim) {
      for (size_t d = 0; d < m_ws->getNumDims(); d++) {
        // A different dimension had the same shown dimension
        if ((size_t(index) != d) && (m_dimWidgets[d]->getShownDim() == dim)) {
          // So flip it. If the new one is X, the old one becomes Y
          m_dimWidgets[d]->setShownDim((dim == 0) ? 1 : 0);
          break;
        }
      }
    }
    // Make sure no other dimension is showing the same one
    for (size_t d = 0; d < m_ws->getNumDims(); d++) {
      // A different dimension had the same shown dimension
      if ((size_t(index) != d) && (m_dimWidgets[d]->getShownDim() == dim)) {
        m_dimWidgets[d]->setShownDim(-1);
      }
    }
  }
  // Show the new slice. This finds m_dimX and m_dimY
  this->updateDisplay();
  // Send out a signal
  emit changedShownDim(m_dimX, m_dimY);
}

//=================================================================================================
//========================================== PYTHON METHODS
//=======================================
//=================================================================================================

/** @return the name of the workspace selected, or a blank string
 * if no workspace is set.
 */
QString SliceViewer::getWorkspaceName() const {
  if (m_ws)
    return QString::fromStdString(m_ws->getName());
  else
    return QString();
}

//------------------------------------------------------------------------------------
/** @return the index of the dimension that is currently
 * being shown as the X axis of the plot.
 */
int SliceViewer::getDimX() const { return int(m_dimX); }

/** @return the index of the dimension that is currently
 * being shown as the Y axis of the plot.
 */
int SliceViewer::getDimY() const { return int(m_dimY); }

//------------------------------------------------------------------------------------
/** Set the index of the dimensions that will be shown as
 * the X and Y axis of the plot.
 * You cannot set both axes to be the same.
 *
 * To be called from Python, primarily.
 *
 * @param indexX :: index of the X dimension, from 0 to NDims-1.
 * @param indexY :: index of the Y dimension, from 0 to NDims-1.
 * @throw std::invalid_argument if an index is invalid or repeated.
 */
void SliceViewer::setXYDim(int indexX, int indexY) {
  if (indexX >= int(m_dimWidgets.size()) || indexX < 0)
    throw std::invalid_argument("There is no dimension # " +
                                Strings::toString(indexX) +
                                " in the workspace.");
  if (indexY >= int(m_dimWidgets.size()) || indexY < 0)
    throw std::invalid_argument("There is no dimension # " +
                                Strings::toString(indexY) +
                                " in the workspace.");
  if (indexX == indexY)
    throw std::invalid_argument(
        "X dimension must be different than the Y dimension index.");

  // Set the X and Y widgets
  m_dimWidgets[indexX]->setShownDim(0);
  m_dimWidgets[indexY]->setShownDim(1);

  // Set all other dimensions as slice points
  for (int d = 0; d < int(m_dimWidgets.size()); d++)
    if (d != indexX && d != indexY)
      m_dimWidgets[d]->setShownDim(-1);

  // Show the new slice. This finds m_dimX and m_dimY
  this->updateDisplay();
  emit changedShownDim(m_dimX, m_dimY);
}

//------------------------------------------------------------------------------------
/** Set the dimensions that will be shown as the X and Y axes
 *
 * @param dimX :: name of the X dimension. Must match the workspace dimension
 *names.
 * @param dimY :: name of the Y dimension. Must match the workspace dimension
 *names.
 * @throw std::runtime_error if the dimension name is not found.
 */
void SliceViewer::setXYDim(const QString &dimX, const QString &dimY) {
  if (!m_ws)
    return;
  int indexX = int(m_ws->getDimensionIndexByName(dimX.toStdString()));
  int indexY = int(m_ws->getDimensionIndexByName(dimY.toStdString()));
  this->setXYDim(indexX, indexY);
}

//------------------------------------------------------------------------------------
/** Sets the slice point in the given dimension:
 * that is, what is the position of the plane in that dimension
 *
 * @param dim :: index of the dimension to change
 * @param value :: value of the slice point, in the units of the given
 *dimension.
 *        This should be within the range of min/max for that dimension.
 * @throw std::invalid_argument if the index is invalid
 */
void SliceViewer::setSlicePoint(int dim, double value) {
  if (dim >= int(m_dimWidgets.size()) || dim < 0)
    throw std::invalid_argument("There is no dimension # " +
                                Strings::toString(dim) + " in the workspace.");
  m_dimWidgets[dim]->setSlicePoint(value);
}

//------------------------------------------------------------------------------------
/** Returns the slice point in the given dimension
 *
 * @param dim :: index of the dimension
 * @return slice point for that dimension. Value has no significance for the
 *         X or Y display dimensions.
 * @throw std::invalid_argument if the index is invalid
 */
double SliceViewer::getSlicePoint(int dim) const {
  if (dim >= int(m_dimWidgets.size()) || dim < 0)
    throw std::invalid_argument("There is no dimension # " +
                                Strings::toString(dim) + " in the workspace.");
  return m_slicePoint[dim];
}

//------------------------------------------------------------------------------------
/** Sets the slice point in the given dimension:
 * that is, what is the position of the plane in that dimension
 *
 * @param dim :: name of the dimension to change
 * @param value :: value of the slice point, in the units of the given
 *dimension.
 *        This should be within the range of min/max for that dimension.
 * @throw std::runtime_error if the name is not found in the workspace
 */
void SliceViewer::setSlicePoint(const QString &dim, double value) {
  if (!m_ws)
    return;
  int index = int(m_ws->getDimensionIndexByName(dim.toStdString()));
  return this->setSlicePoint(index, value);
}

//------------------------------------------------------------------------------------
/** Returns the slice point in the given dimension
 *
 * @param dim :: name of the dimension
 * @return slice point for that dimension. Value has no significance for the
 *         X or Y display dimensions.
 * @throw std::runtime_error if the name is not found in the workspace
 */
double SliceViewer::getSlicePoint(const QString &dim) const {
  if (!m_ws)
    return 0;
  int index = int(m_ws->getDimensionIndexByName(dim.toStdString()));
  return this->getSlicePoint(index);
}

//------------------------------------------------------------------------------------
/** Set the color scale limits and log mode via a method call.
 *
 * @param min :: minimum value corresponding to the lowest color on the map
 * @param max :: maximum value corresponding to the highest color on the map
 * @param log :: true for a log color scale, false for linear
 * @throw std::invalid_argument if max < min or if the values are
 *        inconsistent with a log color scale
 */
void SliceViewer::setColorScale(double min, double max, bool log) {
  if (max <= min)
    throw std::invalid_argument("Color scale maximum must be > minimum.");
  if (log && ((min <= 0) || (max <= 0)))
    throw std::invalid_argument(
        "For logarithmic color scales, both minimum and maximum must be > 0.");
  m_colorBar->setViewRange(min, max);
  m_colorBar->setLog(log);
  this->colorRangeChanged();
}

//------------------------------------------------------------------------------------
/** Set the "background" color to use in the color map. Default is white.
 *
 * This is the color that is shown when:
 *
 *  - The coordinate is out of bounds of the workspace.
 *  - When a signal is NAN (not-a-number).
 *  - When the signal is Zero, if that option is selected using
 *setTransparentZeros()
 *
 * @param r :: red component, from 0-255
 * @param g :: green component, from 0-255
 * @param b :: blue component, from 0-255
 */
void SliceViewer::setColorMapBackground(int r, int g, int b) {
  m_colorBar->getColorMap().setNanColor(r, g, b);
  this->colorRangeChanged();
}

//------------------------------------------------------------------------------------
/** Set the minimum value corresponding to the lowest color on the map
 *
 * @param min :: minimum value corresponding to the lowest color on the map
 * @throw std::invalid_argument if max < min or if the values are
 *        inconsistent with a log color scale
 */
void SliceViewer::setColorScaleMin(double min) {
  this->setColorScale(min, this->getColorScaleMax(), this->getColorScaleLog());
}

//------------------------------------------------------------------------------------
/** Set the maximum value corresponding to the lowest color on the map
 *
 * @param max :: maximum value corresponding to the lowest color on the map
 * @throw std::invalid_argument if max < min or if the values are
 *        inconsistent with a log color scale
 */
void SliceViewer::setColorScaleMax(double max) {
  this->setColorScale(this->getColorScaleMin(), max, this->getColorScaleLog());
}

//------------------------------------------------------------------------------------
/** Set whether the color scale is logarithmic
 *
 * @param log :: true for a log color scale, false for linear
 * @throw std::invalid_argument if the min/max values are inconsistent
 *        with a log color scale
 */
void SliceViewer::setColorScaleLog(bool log) {
  this->setColorScale(this->getColorScaleMin(), this->getColorScaleMax(), log);
}

//------------------------------------------------------------------------------------
/** @return the value that corresponds to the lowest color on the color map */
double SliceViewer::getColorScaleMin() const {
  return m_colorBar->getMinimum();
}

/** @return the value that corresponds to the highest color on the color map */
double SliceViewer::getColorScaleMax() const {
  return m_colorBar->getMaximum();
}

/** @return True if the color scale is in logarithmic mode */
bool SliceViewer::getColorScaleLog() const { return m_colorBar->getLog(); }

//------------------------------------------------------------------------------------
/** Sets whether the image should be rendered in "fast" mode, where
 * the workspace's resolution is used to guess how many pixels to render.
 *
 * If false, each pixel on screen will be rendered. This is the most
 * accurate view but the slowest.
 *
 * This redraws the screen.
 *
 * @param fast :: true to use "fast" rendering mode.
 */
void SliceViewer::setFastRender(bool fast) {
  m_fastRender = fast;
  m_data->setFastMode(m_fastRender);
  this->updateDisplay();
}

/** Return true if the image is in "fast" rendering mode.
 *
 * In "fast" mode, the workspace's resolution is used to guess how many
 * pixels to render. If false, each pixel on screen will be rendered.
 * This is the most accurate view but the slowest.
 *
 * @return True if the image is in "fast" rendering mode.
 */
bool SliceViewer::getFastRender() const { return m_fastRender; }

//------------------------------------------------------------------------------------
/** Set the limits in X and Y to be shown in the plot.
 * The X and Y values are in the units of their respective dimensions.
 * You can change the mapping from X/Y in the plot to specific
 * dimensions in the displayed workspace using setXYDim().
 *
 * You can flip the direction of the scale if you specify,
 * e.g., xleft > xright.
 *
 * @param xleft   :: x-value on the left side of the graph
 * @param xright  :: x-value on the right side of the graph
 * @param ybottom :: y-value on the bottom of the graph
 * @param ytop    :: y-value on the top of the graph
 */
void SliceViewer::setXYLimits(double xleft, double xright, double ybottom,
                              double ytop) {
  // Set the limits in X and Y
  m_plot->setAxisScale(m_spect->xAxis(), xleft, xright);
  m_plot->setAxisScale(m_spect->yAxis(), ybottom, ytop);
  // Make sure the view updates
  m_plot->replot();
  updatePeaksOverlay();
}

//------------------------------------------------------------------------------------
/** @return Returns the [left, right] limits of the view in the X axis. */
QwtDoubleInterval SliceViewer::getXLimits() const {
  return m_plot->axisScaleDiv(m_spect->xAxis())->interval();
}

/** @return Returns the [bottom, top] limits of the view in the Y axis. */
QwtDoubleInterval SliceViewer::getYLimits() const {
  return m_plot->axisScaleDiv(m_spect->yAxis())->interval();
}

//------------------------------------------------------------------------------------
/** Opens a workspace and sets the view and slice points
 * given the XML from the MultiSlice view in XML format.
 *
 * @param xml :: string describing workspace, slice point, etc.
 * @throw std::runtime_error if error in parsing XML
 */
void SliceViewer::openFromXML(const QString &xml) {
  // Set up the DOM parser and parse xml file
  DOMParser pParser;
  Poco::AutoPtr<Poco::XML::Document> pDoc;
  try
  {
    pDoc = pParser.parseString(xml.toStdString());
  } catch (Poco::Exception &exc) {
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Unable to parse XML. " +
        std::string(exc.what()));
  } catch (...) {
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Unspecified error parsing XML. ");
  }

  // Get pointer to root element
  Poco::XML::Element *pRootElem = pDoc->documentElement();
  if (!pRootElem->hasChildNodes())
    throw std::runtime_error(
        "SliceViewer::openFromXML(): No root element in XML string.");

  // ------- Find the workspace ------------
  Poco::XML::Element *cur = NULL;
  cur = pRootElem->getChildElement("MDWorkspaceName");
  if (!cur)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): No MDWorkspaceName element.");
  std::string wsName = cur->innerText();

  if (wsName.empty())
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Empty MDWorkspaceName found!");

  // Look for the rebinned workspace with a custom name:
  std::string histoName = wsName + "_visual_md";
  // Use the rebinned workspace if available.
  if (AnalysisDataService::Instance().doesExist(histoName))
    this->setWorkspace(QString::fromStdString(histoName));
  else
    this->setWorkspace(QString::fromStdString(wsName));

  if (!m_ws)
    throw std::runtime_error("SliceViewer::openFromXML(): Workspace no found!");
  if ((m_ws->getNumDims() < 3) || (m_ws->getNumDims() > 4))
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Workspace should have 3 or 4 dimensions.");

  // Hard update to make sure axis reorientations are respected.
  this->updateDisplay(true);

  // ------- Read which are the X/Y dimensions ------------
  Poco::XML::Element *dims = pRootElem->getChildElement("DimensionSet");
  if (!dims)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): No DimensionSet element.");

  // Map: The index = dimension in ParaView; Value = dimension of the workspace.
  int dimMap[4];
  // For 4D workspace, the value of the "time"
  double TimeValue = 0.0;

  std::string dimChars = "XYZT";
  for (size_t ind = 0; ind < 4; ind++) {
    // X, Y, Z, or T
    std::string dimLetter = " ";
    dimLetter[0] = dimChars[ind];
    Poco::XML::Element *dim = dims->getChildElement(dimLetter + "Dimension");
    if (!dim)
      throw std::runtime_error("SliceViewer::openFromXML(): No " + dimLetter +
                               "Dimension element.");
    cur = dim->getChildElement("RefDimensionId");
    if (!cur)
      throw std::runtime_error(
          "SliceViewer::openFromXML(): No RefDimensionId in " + dimLetter +
          "Dimension element.");
    std::string dimName = cur->innerText();
    if (!dimName.empty())
      dimMap[ind] = int(m_ws->getDimensionIndexByName(dimName));
    else
      dimMap[ind] = -1;
    // Find the time value
    if (ind == 3) {
      cur = dim->getChildElement("Value");
      if (cur) {
        if (!Kernel::Strings::convert(cur->innerText(), TimeValue))
          throw std::runtime_error(
              "SliceViewer::openFromXML(): Could not cast Value '" +
              cur->innerText() + "' to double in TDimension element.");
      }
    }
  }
  // The index of the time dimensions
  int timeDim = dimMap[3];

  // ------- Read the plane function ------------
  Poco::XML::Element* func = pRootElem->getChildElement("Function");
  if (!func)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): No Function element.");
  Poco::XML::Element* paramlist = func->getChildElement("ParameterList");
  if (!paramlist)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): No ParameterList element.");

  Poco::AutoPtr<NodeList> params = paramlist->getElementsByTagName("Parameter");
  Poco::AutoPtr<NodeList> paramvals;
  Node * param;
  if (!params || params->length() < 2)
    throw std::runtime_error("SliceViewer::openFromXML(): Too few parameters.");

  param = params->item(0);
  paramvals = param->childNodes();
  if (!paramvals || paramvals->length() < 2)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Parameter has too few children");
  std::string normalStr = paramvals->item(1)->innerText();

  param = params->item(1);
  paramvals = param->childNodes();
  if (!paramvals || paramvals->length() < 2)
    throw std::runtime_error(
        "SliceViewer::openFromXML(): Parameter has too few children");
  std::string originStr = paramvals->item(1)->innerText();

  // ------- Apply the X/Y dimensions ------------
  V3D normal, origin;
  normal.fromString(normalStr);
  origin.fromString(originStr);
  coord_t planeOrigin = 0;
  int normalDim = -1;
  for (int i = 0; i < 3; i++)
    if (normal[i] > 0.99)
      normalDim = i;
  if (normal.norm() > 1.01 || normal.norm() < 0.99)
    throw std::runtime_error("Normal vector is not length 1.0!");
  if (normalDim < 0)
    throw std::runtime_error("Could not find the normal of the plane. Plane "
                             "must be along one of the axes!");

  // Get the plane origin and the dimension in the workspace dimensions
  planeOrigin = static_cast<coord_t>(origin[normalDim]);
  normalDim = dimMap[normalDim];

  VMD slicePoint(m_ws->getNumDims());
  slicePoint *= 0; // clearnormalDim
  // The plane origin in the 3D view
  slicePoint[normalDim] = planeOrigin;
  // The "time" of the paraview view
  if (dimMap[3] > 0)
    slicePoint[dimMap[3]] = static_cast<coord_t>(TimeValue);

  // Now find the first unused dimensions = that is the X view dimension
  int xdim = -1;
  for (int d = 0; d < int(m_ws->getNumDims()); d++)
    if ((d != normalDim) && (d != timeDim)) {
      xdim = d;
      break;
    }

  // Now find the second unused dimensions = that is the Y view dimension
  int ydim = -1;
  for (int d = 0; d < int(m_ws->getNumDims()); d++)
    if ((d != normalDim) && (d != timeDim) && (d != xdim)) {
      ydim = d;
      break;
    }

  if (xdim < 0 || ydim < 0)
    throw std::runtime_error("SliceViewer::openFromXML(): Could not find the X "
                             "or Y view dimension.");

  // Finally, set the view dimension and slice points
  this->setXYDim(xdim, ydim);
  for (int d = 0; d < int(m_ws->getNumDims()); d++)
    this->setSlicePoint(d, slicePoint[d]);
}

//------------------------------------------------------------------------------------
/** This slot is called when the dynamic rebinning parameters are changed.
 * It recalculates the dynamically rebinned workspace and plots it
 */
void SliceViewer::rebinParamsChanged() {
  if (!m_ws)
    return;

  // Cancel pre-existing algo.
  m_algoRunner->cancelRunningAlgorithm();

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("BinMD");
  alg->setProperty("InputWorkspace", m_ws);
  alg->setProperty("AxisAligned", false);

  std::vector<double> OutputExtents;
  std::vector<int> OutputBins;

  for (size_t d = 0; d < m_dimWidgets.size(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    MDHistoDimension_sptr dim = m_dimensions[d];

    // Build up the arguments to BinMD
    double min = 0;
    double max = 1;
    int numBins = 1;
    if (widget->getShownDim() < 0) {
      // Slice point. So integrate with a thickness
      min = widget->getSlicePoint() - widget->getThickness();
      max = widget->getSlicePoint() + widget->getThickness();
      // From min to max, with only 1 bin
    } else {
      // Shown dimension. Use the currently visible range.
      QwtDoubleInterval limits;
      if (widget->getShownDim() == 0)
        limits = this->getXLimits();
      else
        limits = this->getYLimits();
      min = limits.minValue();
      max = limits.maxValue();
      // And the user-entered number of bins
      numBins = widget->getNumBins();
    }

    OutputExtents.push_back(min);
    OutputExtents.push_back(max);
    OutputBins.push_back(numBins);

    // Set the BasisVector property...
    VMD basis(m_ws->getNumDims());
    basis[d] = 1.0;
    std::string prop = dim->getName() + "," + dim->getUnits().ascii() + "," +
                       basis.toString(",");
    alg->setPropertyValue("BasisVector" + Strings::toString(d), prop);
  }

  m_overlayWSName = m_ws->getName() + "_rebinned";

  // Set all the other properties
  alg->setProperty("OutputExtents", OutputExtents);
  alg->setProperty("OutputBins", OutputBins);
  alg->setPropertyValue("Translation", "");
  alg->setProperty("NormalizeBasisVectors", true);
  alg->setProperty("ForceOrthogonal", false);
  alg->setProperty("Parallel", true);
  alg->setPropertyValue("OutputWorkspace", m_overlayWSName);

  // Make the algorithm begin asynchronously
  m_algoRunner->startAlgorithm(alg);
  // The dynamicRebinComplete() slot is connected to the runner to plot when
  // complete.
}

//--------------------------------------------------------------------------------------
/** Slot called by the observer when the BinMD call has completed.
 * This returns the execution to the main GUI thread, and
 * so can update the GUI.
 * @param error :: true if the algorithm died with an error.
 */
void SliceViewer::dynamicRebinComplete(bool error) {
  // If there was an error, clear the workspace
  m_overlayWS.reset();
  if (!error) {
    if (AnalysisDataService::Instance().doesExist(m_overlayWSName))
      m_overlayWS = AnalysisDataService::Instance().retrieveWS<IMDWorkspace>(
          m_overlayWSName);
  }

  // Make it so we refresh the display, with this workspace on TOP
  m_data->setOverlayWorkspace(m_overlayWS);

  if (m_overlayWS) {
    // Position the outline according to the position of the workspace.
    double yMin = m_overlayWS->getDimension(m_dimY)->getMinimum();
    double yMax = m_overlayWS->getDimension(m_dimY)->getMaximum();
    double yMiddle = (yMin + yMax) / 2.0;
    QPointF pointA(m_overlayWS->getDimension(m_dimX)->getMinimum(), yMiddle);
    QPointF pointB(m_overlayWS->getDimension(m_dimX)->getMaximum(), yMiddle);
    m_overlayWSOutline->setPointA(pointA);
    m_overlayWSOutline->setPointB(pointB);
    m_overlayWSOutline->setWidth((yMax - yMin) / 2.0);
    m_overlayWSOutline->setCreationMode(false);
    m_overlayWSOutline->setShown(true);
  } else
    m_overlayWSOutline->setShown(false);
  this->updateDisplay();
}

/**
Event handler for plot panning.
*/
void SliceViewer::panned(int, int) {
  autoRebinIfRequired();

  this->updatePeaksOverlay();
}

/**
Event handler for changing magnification.
*/
void SliceViewer::magnifierRescaled(double) {
  autoRebinIfRequired();

  this->updatePeaksOverlay();
}

/**
Event handler for the auto rebin toggle event.
*/
void SliceViewer::autoRebin_toggled(bool checked) {
  if (checked) {
    // Generate the rebin overlay assuming it isn't up to date.
    this->rebinParamsChanged();
  }
}

/**
@return True only when Auto-Rebinning should be considered.
*/
bool SliceViewer::isAutoRebinSet() const {
  return ui.btnAutoRebin->isEnabled() && ui.btnAutoRebin->isChecked();
}

/**
Auto rebin the workspace according the the current-view + rebin parameters if
that option has been set.
*/
void SliceViewer::autoRebinIfRequired() {
  if (isAutoRebinSet()) {
    rebinParamsChanged();
  }
}

/**
 * Convenience function for removing all displayed peaks workspaces.
 */
void SliceViewer::clearPeaksWorkspaces() { this->disablePeakOverlays(); }

/**
 * Helper function to rest the SliceViewer into a no-peak overlay mode.
 */
void SliceViewer::disablePeakOverlays() {
  // Un-check the button for consistency.
  ui.btnPeakOverlay->setChecked(false);
  m_peaksPresenter->clear();
  emit showPeaksViewer(false);
  m_menuPeaks->setEnabled(false);
}

/**
 * Show a collection of peaks workspaces as overplots
 * @param list : List of peak workspace names to show.
 */
ProxyCompositePeaksPresenter *
SliceViewer::setPeaksWorkspaces(const QStringList &list) {

  if (m_ws->getNumDims() < 2) {
    throw std::invalid_argument("Cannot overplot a peaks workspace unless the "
                                "base workspace has two or more dimensions");
  }

  // Fetch the correct Peak Overlay Transform Factory;
  const std::string xDim =
      m_plot->axisTitle(QwtPlot::xBottom).text().toStdString();
  const std::string yDim =
      m_plot->axisTitle(QwtPlot::yLeft).text().toStdString();

  PeakTransformFactory_sptr transformFactory =
      m_peakTransformSelector.makeChoice(xDim, yDim);
  // Loop through each of those peaks workspaces and display them.
  for (int i = 0; i < list.size(); ++i) {
    const std::string workspaceName = list[i].toStdString();
    if (!AnalysisDataService::Instance().doesExist(workspaceName)) {
      throw std::invalid_argument(workspaceName + " Does not exist");
    }
    IPeaksWorkspace_sptr peaksWS =
        AnalysisDataService::Instance().retrieveWS<IPeaksWorkspace>(
            workspaceName);
    const size_t numberOfChildPresenters = m_peaksPresenter->size();

    PeakOverlayViewFactorySelector_sptr viewFactorySelector =
        boost::make_shared<PeakOverlayViewFactorySelector>();
    // Candidate for overplotting as spherical peaks
    viewFactorySelector->registerCandidate(
        boost::make_shared<PeakOverlayMultiSphereFactory>(
            peaksWS, m_plot, m_plot->canvas(), numberOfChildPresenters));
    // Candiate for plotting as a markers of peak positions
    viewFactorySelector->registerCandidate(
        boost::make_shared<PeakOverlayMultiCrossFactory>(
            m_ws, transformFactory->createDefaultTransform(), peaksWS, m_plot,
            m_plot->canvas(), numberOfChildPresenters));
    try {
      m_peaksPresenter->addPeaksPresenter(
          boost::make_shared<ConcretePeaksPresenter>(
              viewFactorySelector->makeSelection(), peaksWS, m_ws,
              transformFactory));
    } catch (std::invalid_argument &) {
      // Incompatible PeaksWorkspace.
      disablePeakOverlays();
      throw;
    }
  }
  updatePeakOverlaySliderWidget();
  emit showPeaksViewer(true);
  m_menuPeaks->setEnabled(true);
  // Depresses the button without raising the clicked event
  this->ui.btnPeakOverlay->setChecked(true);
  return m_proxyPeaksPresenter.get();
}


/**
Obtain the reference to a new PeakOverlay slider widget if necessary.
*/
void SliceViewer::updatePeakOverlaySliderWidget() {
  for (size_t d = 0; d < m_ws->getNumDims(); d++) {
    DimensionSliceWidget *widget = m_dimWidgets[d];
    if (widget->getShownDim() < 0) {
      if (m_peaksPresenter->isLabelOfFreeAxis(widget->getDimName())) {
        m_peaksSliderWidget = widget; // Cache the widget being used for this.
        auto xInterval = getXLimits();
        auto yInterval = getYLimits();
        PeakBoundingBox viewableRegion(
            Left(xInterval.minValue()), Right(xInterval.maxValue()),
            Top(yInterval.maxValue()), Bottom(yInterval.minValue()),
            SlicePoint(m_peaksSliderWidget->getSlicePoint()));

        updatePeaksOverlay(); // Ensure that the presenter is up-to-date with
                              // the change
      }
    }
  }
}

/**
 * Update the peaks presenter. Use the slice position as well as the plot region
 * to update the collection of peaks presetners.
 */
void SliceViewer::updatePeaksOverlay() {
  if (m_peaksSliderWidget != NULL) {
    auto xInterval = getXLimits();
    auto yInterval = getYLimits();
    PeakBoundingBox viewableRegion(
        Left(xInterval.minValue()), Right(xInterval.maxValue()),
        Top(yInterval.maxValue()), Bottom(yInterval.minValue()),
        SlicePoint(m_peaksSliderWidget->getSlicePoint()));
    m_peaksPresenter->updateWithSlicePoint(viewableRegion);
  }
}

/**
Decide whether to enable peak overlays, then reflect the ui controls to indicate
this.

1) Check the dimensionality of the workspace.
2) Check that the currently displayed plot x and y correspond to a valid peak
transform (H, K, L) etc.

*/
void SliceViewer::enablePeakOverlaysIfAppropriate() {
  bool enablePeakOverlays = false;
  if (m_ws->getNumDims() >= 2) {
    const std::string xDim =
        m_plot->axisTitle(QwtPlot::xBottom).text().toStdString();
    const std::string yDim =
        m_plot->axisTitle(QwtPlot::yLeft).text().toStdString();
    enablePeakOverlays =
        m_peakTransformSelector.hasFactoryForTransform(xDim, yDim);
  }

  if (!enablePeakOverlays) {
    ui.btnPeakOverlay->setChecked(false); // Don't leave the button depressed.
    m_peaksPresenter->clear();            // Reset the presenter
  }
}

/**
Get the peaks proxy presenter.
*/
boost::shared_ptr<ProxyCompositePeaksPresenter>
SliceViewer::getPeaksPresenter() const {
  return m_proxyPeaksPresenter;
}

/**
Zoom in upon a rectangle
@param boundingBox : The bounding rectangular box to zoom to.
*/
void SliceViewer::zoomToRectangle(const PeakBoundingBox &boundingBox) {
  // Set the limits in X and Y
  m_plot->setAxisScale(m_spect->xAxis(), boundingBox.left(),
                       boundingBox.right());
  m_plot->setAxisScale(m_spect->yAxis(), boundingBox.bottom(),
                       boundingBox.top());

  const QString dimensionName =
      QString::fromStdString(m_peaksSliderWidget->getDimName());
  this->setSlicePoint(dimensionName, boundingBox.slicePoint());

  // Make sure the view updates
  m_plot->replot();
}

/**
 * Reset the original view.
 */
void SliceViewer::resetView() { this->resetZoom(); }

/**
 * @brief Detach this sliceviewer from the peaksviewer
 */
void SliceViewer::detach()
{
    this->disablePeakOverlays();
}

void SliceViewer::peakWorkspaceChanged(
    const std::string &wsName,
    boost::shared_ptr<Mantid::API::IPeaksWorkspace> &changedPeaksWS) {
  // Tell the composite presenter about it
  m_peaksPresenter->notifyWorkspaceChanged(wsName, changedPeaksWS);
}

void SliceViewer::onPeaksViewerOverlayOptions() {
  PeaksViewerOverlayDialog dlg(this->m_peaksPresenter);
  dlg.exec();
}

void SliceViewer::dragEnterEvent(QDragEnterEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    e->accept();
  } else {
    e->ignore();
  }
}

void SliceViewer::dropEvent(QDropEvent *e) {
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace") {
    QString text = e->mimeData()->text();
    int endIndex = 0;
    QStringList wsNames;
    while (text.indexOf("[\"", endIndex) > -1) {
      int startIndex = text.indexOf("[\"", endIndex) + 2;
      endIndex = text.indexOf("\"]", startIndex);
      QString candidate = text.mid(startIndex, endIndex - startIndex);
      if(boost::dynamic_pointer_cast<IPeaksWorkspace>(AnalysisDataService::Instance().retrieve(candidate.toStdString())))
      {
          wsNames.append(candidate);
          e->accept();
      }
      else
      {
          e->ignore();
      }

    }
    if(!wsNames.empty()){
        // Show these peaks workspaces
        this->setPeaksWorkspaces(wsNames);
    }
}
}


} // namespace
}
