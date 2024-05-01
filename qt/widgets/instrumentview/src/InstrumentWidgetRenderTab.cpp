// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/InstrumentView/InstrumentWidgetRenderTab.h"
#include "MantidQtWidgets/InstrumentView/InstrumentRenderer.h"
#include "MantidQtWidgets/InstrumentView/Projection3D.h"
#include "MantidQtWidgets/InstrumentView/ProjectionSurface.h"
#include "MantidQtWidgets/InstrumentView/RotationSurface.h"
#include "MantidQtWidgets/InstrumentView/UCorrectionDialog.h"
#include "MantidQtWidgets/InstrumentView/UnwrappedSurface.h"

#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QToolTip>
#include <QVBoxLayout>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/InstrumentView/BinDialog.h"
#include "MantidQtWidgets/InstrumentView/InstrumentWidget.h"

#include <limits>
#include <utility>

namespace MantidQt::MantidWidgets {

Mantid::Kernel::Logger g_log("InstrumentWidgetRenderTab");

// QSettings entry names
const char *EntryManualUCorrection = "ManualUCorrection";
const char *EntryUCorrectionMin = "UCorrectionMin";
const char *EntryUCorrectionMax = "UCorrectionMax";

InstrumentWidgetRenderTab::InstrumentWidgetRenderTab(InstrumentWidget *instrWindow) : InstrumentWidgetTab(instrWindow) {
  QVBoxLayout *renderControlsLayout = new QVBoxLayout(this);

  connectInstrumentWidgetSignals();

  setupSurfaceTypeOptions();

  // Reset view button
  m_resetView = new QPushButton(tr("Reset View"));
  m_resetView->setToolTip("Reset the instrument view to default");
  connect(m_resetView, SIGNAL(clicked()), this, SLOT(resetView()));

  // Save image control
  mSaveImage = new QPushButton(tr("Save image"));
  mSaveImage->setToolTip("Save the instrument image to a file");
  connect(mSaveImage, SIGNAL(clicked()), this, SLOT(saveImage()));

  auto *displaySettings = setupDisplaySettings();

  QFrame *axisViewFrame = setupAxisFrame();

  setupColorMapWidget();

  auto *unwrappedControlsLayout = new QHBoxLayout;
  setupUnwrappedControls(unwrappedControlsLayout);

  m_autoscaling = new QCheckBox("Autoscaling", this);
  m_autoscaling->setChecked(true);
  connect(m_autoscaling, SIGNAL(toggled(bool)), this, SLOT(setColorMapAutoscaling(bool)));

  // layout
  renderControlsLayout->addWidget(m_surfaceTypeButton);
  renderControlsLayout->addLayout(unwrappedControlsLayout);
  renderControlsLayout->addWidget(axisViewFrame);
  renderControlsLayout->addWidget(m_resetView);
  renderControlsLayout->addWidget(displaySettings);
  renderControlsLayout->addWidget(mSaveImage);
  renderControlsLayout->addWidget(m_colorBarWidget);
  renderControlsLayout->addWidget(m_autoscaling);

  // Add GridBank Controls if Grid bank present
  setupGridBankMenu(renderControlsLayout);
}

InstrumentWidgetRenderTab::~InstrumentWidgetRenderTab() = default;

void InstrumentWidgetRenderTab::connectInstrumentWidgetSignals() const {
  // Connect to InstrumentWindow signals
  connect(m_instrWidget, SIGNAL(surfaceTypeChanged(int)), this, SLOT(surfaceTypeChanged(int)));
  connect(m_instrWidget, SIGNAL(maintainAspectRatioChanged(bool)), this, SLOT(maintainAspectRatioChanged(bool)));
  connect(m_instrWidget, SIGNAL(colorMapChanged()), this, SLOT(colorMapChanged()));
  connect(m_instrWidget, SIGNAL(colorMapMaxValueChanged(double)), this, SLOT(setMaxValue(double)));
  connect(m_instrWidget, SIGNAL(colorMapMinValueChanged(double)), this, SLOT(setMinValue(double)));
  connect(m_instrWidget, SIGNAL(colorMapRangeChanged(double, double)), this, SLOT(setRange(double, double)));
  connect(m_instrWidget, SIGNAL(scaleTypeChanged(int)), this, SLOT(scaleTypeChanged(int)));
  connect(m_instrWidget, SIGNAL(nthPowerChanged(double)), this, SLOT(nthPowerChanged(double)));
  connect(m_instrWidget, SIGNAL(glOptionChanged(bool)), this, SLOT(glOptionChanged(bool)));
}

void InstrumentWidgetRenderTab::setupSurfaceTypeOptions() {
  // Surface type controls
  m_surfaceTypeButton = new QPushButton("Render mode", this);
  m_surfaceTypeButton->setToolTip("Set render mode");

  QSignalMapper *signalMapper = new QSignalMapper(this);
  connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(setSurfaceType(int)));

  m_full3D = new QAction("Full 3D", this);
  m_full3D->setCheckable(true);
  connect(m_full3D, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_full3D, 0);
  m_cylindricalX = new QAction("Cylindrical X", this);
  m_cylindricalX->setCheckable(true);
  connect(m_cylindricalX, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_cylindricalX, 1);
  m_cylindricalY = new QAction("Cylindrical Y", this);
  m_cylindricalY->setCheckable(true);
  connect(m_cylindricalY, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_cylindricalY, 2);
  m_cylindricalZ = new QAction("Cylindrical Z", this);
  m_cylindricalZ->setCheckable(true);
  connect(m_cylindricalZ, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_cylindricalZ, 3);
  m_sphericalX = new QAction("Spherical X", this);
  m_sphericalX->setCheckable(true);
  connect(m_sphericalX, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_sphericalX, 4);
  m_sphericalY = new QAction("Spherical Y", this);
  m_sphericalY->setCheckable(true);
  connect(m_sphericalY, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_sphericalY, 5);
  m_sphericalZ = new QAction("Spherical Z", this);
  m_sphericalZ->setCheckable(true);
  connect(m_sphericalZ, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_sphericalZ, 6);
  m_sideBySide = new QAction("Side by Side", this);
  m_sideBySide->setCheckable(true);
  connect(m_sideBySide, SIGNAL(triggered()), signalMapper, SLOT(map()));
  signalMapper->setMapping(m_sideBySide, 7);

  m_surfaceTypeActionGroup = new QActionGroup(this);
  m_surfaceTypeActionGroup->setExclusive(true);
  m_surfaceTypeActionGroup->addAction(m_full3D);
  m_surfaceTypeActionGroup->addAction(m_cylindricalX);
  m_surfaceTypeActionGroup->addAction(m_cylindricalY);
  m_surfaceTypeActionGroup->addAction(m_cylindricalZ);
  m_surfaceTypeActionGroup->addAction(m_sphericalX);
  m_surfaceTypeActionGroup->addAction(m_sphericalY);
  m_surfaceTypeActionGroup->addAction(m_sphericalZ);
  m_surfaceTypeActionGroup->addAction(m_sideBySide);

  QMenu *renderModeMenu = new QMenu(this);
  renderModeMenu->addActions(m_surfaceTypeActionGroup->actions());
  connect(renderModeMenu, SIGNAL(hovered(QAction *)), this, SLOT(showMenuToolTip(QAction *)));

  m_surfaceTypeButton->setMenu(renderModeMenu);
}

QPushButton *InstrumentWidgetRenderTab::setupDisplaySettings() {
  // Setup Display Setting menu
  QPushButton *displaySettings = new QPushButton("Display Settings", this);
  QMenu *displaySettingsMenu = new QMenu(this);
  connect(displaySettingsMenu, SIGNAL(aboutToShow()), this, SLOT(displaySettingsAboutToshow()));
  m_colorMap = new QAction("Color Map", this);
  connect(m_colorMap, SIGNAL(triggered()), this, SLOT(changeColorMap()));
  m_backgroundColor = new QAction("Background Color", this);
  connect(m_backgroundColor, SIGNAL(triggered()), m_instrWidget, SLOT(pickBackgroundColor()));
  m_lighting = new QAction("Lighting", this);
  m_lighting->setCheckable(true);
  m_lighting->setChecked(false);
  connect(m_lighting, SIGNAL(toggled(bool)), m_instrWidget, SIGNAL(enableLighting(bool)));
  m_displayAxes = new QAction("Display Axes", this);
  m_displayAxes->setCheckable(true);
  m_displayAxes->setChecked(true);
  connect(m_displayAxes, SIGNAL(toggled(bool)), this, SLOT(showAxes(bool)));
  m_displayDetectorsOnly = new QAction("Display Detectors Only", this);
  m_displayDetectorsOnly->setCheckable(true);
  m_displayDetectorsOnly->setChecked(true);
  connect(m_displayDetectorsOnly, SIGNAL(toggled(bool)), this, SLOT(displayDetectorsOnly(bool)));
  m_wireframe = new QAction("Wireframe", this);
  m_wireframe->setCheckable(true);
  m_wireframe->setChecked(false);
  connect(m_wireframe, SIGNAL(toggled(bool)), m_instrWidget, SLOT(setWireframe(bool)));
  m_UCorrection = new QAction("U Correction", this);
  m_UCorrection->setToolTip("Manually set the limits on the horizontal axis.");
  connect(m_UCorrection, SIGNAL(triggered()), this, SLOT(setUCorrection()));
  m_maintainAspectRatio = new QAction("Maintain Aspect Ratio", this);
  m_maintainAspectRatio->setCheckable(true);
  m_maintainAspectRatio->setChecked(true);
  connect(m_maintainAspectRatio, SIGNAL(toggled(bool)), this, SLOT(setMaintainAspectRatio(bool)));
  m_tooltipInfo = new QAction("Tooltip", this);
  m_tooltipInfo->setToolTip("Show detector info in a tooltip when hovering.");
  m_tooltipInfo->setCheckable(true);
  m_tooltipInfo->setChecked(false);
  connect(m_tooltipInfo, SIGNAL(toggled(bool)), this, SLOT(toggleTooltip(bool)));

  // Create "Use OpenGL" action
  m_GLView = new QAction("Use OpenGL", this);
  m_GLView->setToolTip("Toggle use of OpenGL for unwrapped view. Default value "
                       "can be set in Preferences.");
  m_GLView->setCheckable(true);
  QString setting = QString::fromStdString(
                        Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InstrumentView.UseOpenGL"))
                        .toUpper();
  bool useOpenGL = setting == "ON";
  connect(m_GLView, SIGNAL(toggled(bool)), this, SLOT(enableGL(bool)));
  enableGL(useOpenGL);

  displaySettingsMenu->addAction(m_colorMap);
  displaySettingsMenu->addAction(m_backgroundColor);
  displaySettingsMenu->addSeparator();
  displaySettingsMenu->addAction(m_displayAxes);
  displaySettingsMenu->addAction(m_displayDetectorsOnly);
  displaySettingsMenu->addAction(m_wireframe);
  displaySettingsMenu->addAction(m_lighting);
  displaySettingsMenu->addAction(m_GLView);
  displaySettingsMenu->addAction(m_UCorrection);
  displaySettingsMenu->addAction(m_maintainAspectRatio);
  displaySettingsMenu->addAction(m_tooltipInfo);

  displaySettings->setMenu(displaySettingsMenu);
  connect(displaySettingsMenu, SIGNAL(hovered(QAction *)), this, SLOT(showMenuToolTip(QAction *)));

  return displaySettings;
}

void InstrumentWidgetRenderTab::setupColorMapWidget() {
  // Colormap widget
  m_colorBarWidget = new ColorBar(this);
  connect(m_colorBarWidget, SIGNAL(scaleTypeChanged(int)), m_instrWidget, SLOT(changeScaleType(int)));
  connect(m_colorBarWidget, SIGNAL(nthPowerChanged(double)), m_instrWidget, SLOT(changeNthPower(double)));
  connect(m_colorBarWidget, SIGNAL(minValueChanged(double)), m_instrWidget, SLOT(changeColorMapMinValue(double)));
  connect(m_colorBarWidget, SIGNAL(minValueEdited(double)), m_instrWidget, SLOT(disableColorMapAutoscaling()));
  connect(m_colorBarWidget, SIGNAL(maxValueChanged(double)), m_instrWidget, SLOT(changeColorMapMaxValue(double)));
  connect(m_colorBarWidget, SIGNAL(maxValueEdited(double)), m_instrWidget, SLOT(disableColorMapAutoscaling()));
}

void InstrumentWidgetRenderTab::setupUnwrappedControls(QHBoxLayout *parentLayout) {
  m_flipCheckBox = new QCheckBox("Flip view", this);
  m_flipCheckBox->setToolTip("Flip the instrument view horizontally");
  m_flipCheckBox->setChecked(false);
  m_flipCheckBox->hide();
  connect(m_flipCheckBox, SIGNAL(toggled(bool)), this, SLOT(flipUnwrappedView(bool)));

  m_peakOverlaysButton = new QPushButton("Peaks options", this);
  m_peakOverlaysButton->setToolTip("Set peak overlay options");
  m_peakOverlaysButton->hide();
  m_peakOverlaysButton->setMenu(createPeaksMenu());

  parentLayout->addWidget(m_flipCheckBox);
  parentLayout->addWidget(m_peakOverlaysButton);
}

void InstrumentWidgetRenderTab::setupGridBankMenu(QVBoxLayout *parentLayout) {
  const auto &actor = m_instrWidget->getInstrumentActor();

  if (!actor.isInitialized())
    return;

  if (!actor.hasGridBank())
    return;

  m_layerSlide = new QSlider(Qt::Orientation::Horizontal, this);
  m_layerCheck = new QCheckBox("Show Single Layer", this);
  m_layerDisplay = new QLabel("0", this);

  m_layerSlide->setRange(0, static_cast<int>(actor.getNumberOfGridLayers() - 1));
  m_layerSlide->setSingleStep(1);
  m_layerSlide->setPageStep(1);
  m_layerSlide->setSliderPosition(0);
  m_layerSlide->setEnabled(false);
  m_layerCheck->setChecked(false);

  connect(m_layerCheck, SIGNAL(toggled(bool)), this, SLOT(toggleLayerDisplay(bool)));
  connect(m_layerSlide, SIGNAL(valueChanged(int)), this, SLOT(setVisibleLayer(int)));
  connect(m_layerSlide, SIGNAL(valueChanged(int)), m_layerDisplay, SLOT(setNum(int)));
  auto *voxelControlsLayout = new QHBoxLayout();
  voxelControlsLayout->addWidget(m_layerCheck);
  voxelControlsLayout->addWidget(m_layerSlide);
  voxelControlsLayout->addWidget(m_layerDisplay);

  parentLayout->addLayout(voxelControlsLayout);
  m_usingLayerStore = false;
}

/** Sets up the controls and surrounding layout that allows uses to view the
 * instrument
 *  from an axis that they select
 *  @return the QFrame that will be inserted on the main instrument view form
 */
QFrame *InstrumentWidgetRenderTab::setupAxisFrame() {
  m_resetViewFrame = new QFrame();
  auto *axisViewLayout = new QHBoxLayout();
  axisViewLayout->addWidget(new QLabel("Axis View:"));

  mAxisCombo = new QComboBox();
  mAxisCombo->addItem("Z+");
  mAxisCombo->addItem("Z-");
  mAxisCombo->addItem("X+");
  mAxisCombo->addItem("X-");
  mAxisCombo->addItem("Y+");
  mAxisCombo->addItem("Y-");

  axisViewLayout->addWidget(mAxisCombo);

  axisViewLayout->addWidget(new QLabel("Freeze rotation"));
  m_freezeRotation = new QCheckBox();
  m_freezeRotation->setChecked(false);
  m_freezeRotation->setToolTip("Freeze the screen rotation.");
  axisViewLayout->addWidget(m_freezeRotation);

  m_resetViewFrame->setLayout(axisViewLayout);

  connect(mAxisCombo, SIGNAL(currentIndexChanged(const QString &)), m_instrWidget,
          SLOT(setViewDirection(const QString &)));

  connect(m_freezeRotation, SIGNAL(toggled(bool)), m_instrWidget, SLOT(freezeRotation(bool)));

  return m_resetViewFrame;
}

/**
 * Set checked n-th menu item in m_setPrecison menu.
 */
void InstrumentWidgetRenderTab::setPrecisionMenuItemChecked(int n) {
  for (int i = 0; i < m_precisionActions.size(); ++i) {
    QAction *prec = m_precisionActions[i];
    if (i == n - 1) {
      prec->setChecked(true);
      break;
    }
  }
}

/**
 * Enable/disable the Full 3D menu option
 * @param on :: True to enable.
 */
void InstrumentWidgetRenderTab::enable3DSurface(bool on) {
  m_full3D->setEnabled(on);
  if (on) {
    m_full3D->setToolTip("");
  } else {
    m_full3D->setToolTip("Disabled: check \"Use OpenGL\" option in Display Settings to enable");
  }
}

/// Force the rendering of layers for banks containing vocel/grid detectors,
/// only does this if not already in a forced state.
void InstrumentWidgetRenderTab::forceLayers(bool on) {
  auto &actor = m_instrWidget->getInstrumentActor();

  if (!actor.isInitialized()) {
    return;
  }

  if (!actor.hasGridBank())
    return;

  const auto &renderer = actor.getInstrumentRenderer();
  if (on) {
    // only force this state if not already enforced.
    if (!m_layerCheck->isChecked() || m_layerCheck->isEnabled()) {
      m_usingLayerStore = renderer.isUsingLayers();
      m_layerCheck->setChecked(on);
      toggleLayerDisplay(on);
    }
  } else {
    toggleLayerDisplay(m_usingLayerStore);
    m_layerCheck->setChecked(m_usingLayerStore);
  }

  // Checkbox disabled when forced so that all detectors are never drawn
  m_layerCheck->setDisabled(on);
}

/// Toggles the display of Grid bank layers or all detectors in the instrument
/// view.
void InstrumentWidgetRenderTab::toggleLayerDisplay(bool on) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  m_layerSlide->setEnabled(on);
  auto value = m_layerSlide->value();
  actor.setGridLayer(on, value);
  m_layerDisplay->setNum(value);
  emit rescaleColorMap();
}

/// Select the Grid bank layer which will be displayed in the instrument view.
void InstrumentWidgetRenderTab::setVisibleLayer(int layer) {
  const auto &actor = m_instrWidget->getInstrumentActor();
  actor.setGridLayer(true, layer);
  const auto &renderer = actor.getInstrumentRenderer();
  auto surfaceType = m_instrWidget->getSurfaceType();
  // If in an unwrapped view the surface needs to be redrawn
  if (renderer.isUsingLayers() && surfaceType != InstrumentWidget::SurfaceType::FULL3D)
    m_instrWidget->resetSurface();

  emit rescaleColorMap();
}

/**
 * Surface-specific adjustments.
 */
void InstrumentWidgetRenderTab::initSurface() {
  setAxis(QString::fromStdString(m_instrWidget->getInstrumentActor().getDefaultAxis()));
  auto surface = getSurface();

  // 3D axes switch needs to be shown for the 3D surface
  auto p3d = std::dynamic_pointer_cast<Projection3D>(surface);
  if (p3d) {
    p3d->set3DAxesState(areAxesOn());
  }

  bool detectorsOnly = !m_instrWidget->getInstrumentActor().areGuidesShown();
  m_displayDetectorsOnly->blockSignals(true);
  m_displayDetectorsOnly->setChecked(detectorsOnly);
  m_displayDetectorsOnly->blockSignals(false);
  setPrecisionMenuItemChecked(surface->getPeakLabelPrecision());

  // enable u-correction for surfaces of rotation. correction applied in the
  // last
  // session is loaded and re-applied in the new session
  auto rotSurface = std::dynamic_pointer_cast<RotationSurface>(surface);
  if (rotSurface) {
    m_UCorrection->setEnabled(true);
    QString groupName = m_instrWidget->getInstrumentSettingsGroupName();
    QSettings settings;
    settings.beginGroup(groupName);
    bool isManualUCorrection = settings.value(EntryManualUCorrection, false).toBool();
    if (isManualUCorrection) {
      double ucorrMin = settings.value(EntryUCorrectionMin, 0.0).toDouble();
      double ucorrMax = settings.value(EntryUCorrectionMax, 0.0).toDouble();
      rotSurface->setUCorrection(ucorrMin, ucorrMax);
    }
  } else {
    m_UCorrection->setEnabled(false);
  }
}

/**
 * Change color map button slot. This provides the file dialog box to select
 * colormap or sets it directly a string is provided
 */
void InstrumentWidgetRenderTab::changeColorMap(const QString &filename, const bool highlightZeroDets) {
  m_instrWidget->changeColormap(filename, highlightZeroDets);
}

void InstrumentWidgetRenderTab::loadSettings(const QSettings &settings) {
  int show3daxes = settings.value("3DAxesShown", 1).toInt();
  m_instrWidget->set3DAxesState(show3daxes != 0);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(show3daxes != 0);
  m_displayAxes->blockSignals(false);
}

void InstrumentWidgetRenderTab::saveSettings(QSettings &settings) const {
  int val = 0;
  if (m_displayAxes->isChecked())
    val = 1;
  settings.setValue("3DAxesShown", QVariant(val));
}

/**
 * Set minimum value on the colormap scale.
 * @param value :: New value to set.
 * @param apply ::
 */
void InstrumentWidgetRenderTab::setMinValue(double value, bool apply) {
  if (!apply)
    m_colorBarWidget->blockSignals(true);
  m_colorBarWidget->setMinValue(value);
  if (!apply)
    m_colorBarWidget->blockSignals(false);
}

/**
 * Set maximum value on the colormap scale.
 * @param value :: New value to set.
 * @param apply ::
 */
void InstrumentWidgetRenderTab::setMaxValue(double value, bool apply) {
  if (!apply)
    m_colorBarWidget->blockSignals(true);
  m_colorBarWidget->setMaxValue(value);
  if (!apply)
    m_colorBarWidget->blockSignals(false);
}

/**
 * Set minimum and maximum values on the colormap scale.
 * @param minValue :: New min value to set.
 * @param maxValue :: New max value to set.
 * @param apply ::
 */
void InstrumentWidgetRenderTab::setRange(double minValue, double maxValue, bool apply) {
  if (!apply)
    m_colorBarWidget->blockSignals(true);
  m_colorBarWidget->setMinValue(minValue);
  m_colorBarWidget->setMaxValue(maxValue);
  if (!apply)
    m_colorBarWidget->blockSignals(false);
}

ColorMap::ScaleType InstrumentWidgetRenderTab::getScaleType() const {
  return (ColorMap::ScaleType)m_colorBarWidget->getScaleType();
}

void InstrumentWidgetRenderTab::setScaleType(ColorMap::ScaleType type) {
  m_colorBarWidget->setScaleType(static_cast<int>(type));
}

void InstrumentWidgetRenderTab::setAxis(const QString &axisNameArg) {
  mAxisCombo->setCurrentIndex(mAxisCombo->findText("Z+"));
  QString axisName = axisNameArg.toUpper();
  int axisInd = mAxisCombo->findText(axisName.toUpper());
  if (axisInd < 0)
    axisInd = 0;
  mAxisCombo->setCurrentIndex(axisInd);
}

bool InstrumentWidgetRenderTab::areAxesOn() const { return m_displayAxes->isChecked(); }

/**
 * Change the type of the legend scale.
 * @param index :: Index selected in the color scale type combo box.
 */
void InstrumentWidgetRenderTab::setLegendScaleType(int index) {
  if ((int)m_colorBarWidget->getScaleType() != index) {
    m_colorBarWidget->setScaleType(index);
  }
}

/**
 * Show or hide boxes depending if the current render mode is Full3D view
 * @param iv Index of a render mode in RenderMode combo box. iv == 0 is 3D view
 */
void InstrumentWidgetRenderTab::showOrHideBoxes(int iv) {
  bool isFull3D = iv == 0;
  m_resetViewFrame->setVisible(isFull3D);
  m_flipCheckBox->setVisible(!isFull3D);
  m_peakOverlaysButton->setVisible(!isFull3D);

  if (isFull3D)
    m_freezeRotation->setChecked(false);
}

/**
 * Toggle display of 3D axes.
 *
 * @param on :: True of false for on and off.
 */
void InstrumentWidgetRenderTab::showAxes(bool on) {
  m_instrWidget->set3DAxesState(on);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(on);
  m_displayAxes->blockSignals(false);
}

/**
 * Toggle display of guide and other non-detector components.
 *
 * @param yes :: True of false for on and off.
 */
void InstrumentWidgetRenderTab::displayDetectorsOnly(bool yes) {
  m_instrWidget->getInstrumentActor().showGuides(!yes);
  m_instrWidget->updateInstrumentView();
  m_displayDetectorsOnly->blockSignals(true);
  m_displayDetectorsOnly->setChecked(yes);
  m_displayDetectorsOnly->blockSignals(false);
}

/**
 * Toggle use of OpenGL
 *
 * @param on :: True of false for on and off.
 */
void InstrumentWidgetRenderTab::enableGL(bool on) {
  m_instrWidget->enableGL(on);
  m_GLView->blockSignals(true);
  m_GLView->setChecked(m_instrWidget->isGLEnabled());
  m_GLView->blockSignals(false);
  enable3DSurface(on);
}

void InstrumentWidgetRenderTab::showEvent(QShowEvent * /*unused*/) {
  // check if the widget is fully initialized before continuing
  if (!m_instrWidget->isFinished()) {
    return;
  }

  auto surface = getSurface();
  if (surface) {
    surface->setInteractionMode(ProjectionSurface::MoveMode);
  }

  getSurface()->updateView();
  getSurface()->requestRedraw();
}

void InstrumentWidgetRenderTab::flipUnwrappedView(bool on) {
  auto surface = std::dynamic_pointer_cast<UnwrappedSurface>(m_instrWidget->getSurface());
  if (!surface)
    return;
  surface->setFlippedView(on);
  m_instrWidget->updateInstrumentView();
  // Sync checkbox
  m_flipCheckBox->blockSignals(true);
  m_flipCheckBox->setChecked(on);
  m_flipCheckBox->blockSignals(false);
}

/**
 * Resets the render tab view to its default position and zoom.
 */
void InstrumentWidgetRenderTab::resetView() {
  // just recreate the surface from scratch
  m_instrWidget->setSurfaceType(int(m_instrWidget->getSurfaceType()));
  m_instrWidget->getSurface()->setInteractionMode(ProjectionSurface::MoveMode);
  m_instrWidget->getSurface()->freezeRotation(m_freezeRotation->isChecked());
  toggleTooltip(m_tooltipInfo->isChecked());
  m_instrWidget->setWireframe(m_wireframe->isChecked());
}

/**
 * Saves the current image buffer to the given file. An empty string raises a
 * dialog
 * for finding the file
 * @param filename Optional full path of the saved image
 */
void InstrumentWidgetRenderTab::saveImage(const QString &filename) { m_instrWidget->saveImage(filename); }

/**
 * Reset the colorbar parameters.
 * @param cmap :: A new Mantid color map.
 * @param minValue :: A new minimum value.
 * @param maxValue :: A new maximum value.
 * @param minPositive :: A new minimum positive value for the log scale.
 * @param autoscaling :: Flag to set autoscaling of the color
 */
void InstrumentWidgetRenderTab::setupColorBar(const ColorMap &cmap, double minValue, double maxValue,
                                              double minPositive, bool autoscaling) {
  m_colorBarWidget->blockSignals(true);
  m_colorBarWidget->setClim(minValue, maxValue);
  m_colorBarWidget->blockSignals(false);
  m_colorBarWidget->setMinPositiveValue(minPositive);
  m_colorBarWidget->setupColorBarScaling(cmap);
  m_autoscaling->blockSignals(true);
  m_autoscaling->setChecked(autoscaling);
  m_autoscaling->blockSignals(false);
}

/**
 * Set on / off autoscaling of the color bar.
 */
void InstrumentWidgetRenderTab::setColorMapAutoscaling(bool on) { emit setAutoscaling(on); }

/**
 * Creates a menu for interaction with peak overlays
 */
QMenu *InstrumentWidgetRenderTab::createPeaksMenu() {
  QSettings settings;
  settings.beginGroup(m_instrWidget->getSettingsGroupName());
  QMenu *menu = new QMenu(this);

  // show/hide peak hkl labels
  QAction *showLabels = new QAction("Show labels", this);
  showLabels->setCheckable(true);
  showLabels->setChecked(settings.value("ShowPeakLabels", true).toBool());
  connect(showLabels, SIGNAL(toggled(bool)), m_instrWidget, SLOT(setShowPeakLabelsFlag(bool)));
  menu->addAction(showLabels);
  // show/hide peak table rows
  QAction *showRows = new QAction("Show rows", this);
  showRows->setCheckable(true);
  showRows->setChecked(settings.value("ShowPeakRows", true).toBool());
  connect(showRows, SIGNAL(toggled(bool)), m_instrWidget, SLOT(setShowPeakRowFlag(bool)));
  connect(showLabels, SIGNAL(toggled(bool)), showRows, SLOT(setEnabled(bool)));
  showRows->setEnabled(showLabels->isChecked());
  menu->addAction(showRows);
  // setting precision set of actions
  QMenu *setPrecision = new QMenu("Label precision", this);
  m_precisionActionGroup = new QActionGroup(this);
  QSignalMapper *signalMapper = new QSignalMapper(this);
  for (int i = 1; i < 10; ++i) {
    auto *prec = new QAction(QString::number(i), setPrecision);
    prec->setCheckable(true);
    setPrecision->addAction(prec);
    connect(prec, SIGNAL(triggered()), signalMapper, SLOT(map()));
    signalMapper->setMapping(prec, i);
    m_precisionActions.append(prec);
    m_precisionActionGroup->addAction(prec);
  }
  connect(signalMapper, SIGNAL(mapped(int)), m_instrWidget, SLOT(setPeakLabelPrecision(int)));
  menu->addMenu(setPrecision);

  QAction *showRelativeIntensity = new QAction("Indicate relative intensity", this);
  showRelativeIntensity->setCheckable(true);
  showRelativeIntensity->setChecked(settings.value("ShowPeakRelativeIntensities", false).toBool());
  connect(showRelativeIntensity, SIGNAL(toggled(bool)), m_instrWidget, SLOT(setShowPeakRelativeIntensity(bool)));
  menu->addAction(showRelativeIntensity);

  // Clear peaks action
  QAction *clearPeaks = new QAction("Clear peaks", this);
  connect(clearPeaks, SIGNAL(triggered()), m_instrWidget, SLOT(clearPeakOverlays()));
  menu->addAction(clearPeaks);
  return menu;
}

/**
 * Called before the display setting menu opens. Filters out menu options.
 */
void InstrumentWidgetRenderTab::displaySettingsAboutToshow() {
  if (m_instrWidget->getSurfaceType() == InstrumentWidget::FULL3D) {
    // in 3D mode use GL widget only and allow lighting
    m_GLView->setEnabled(false);
    m_maintainAspectRatio->setEnabled(false);
    m_lighting->setEnabled(true);
  } else {
    // in flat view mode allow changing to simple, non-GL viewer
    m_GLView->setEnabled(true);
    m_maintainAspectRatio->setEnabled(true);
    // allow lighting in GL viewer only
    if (!m_GLView->isChecked()) {
      m_lighting->setEnabled(false);
    } else {
      m_lighting->setEnabled(true);
    }
  }
}

/**
 * Change the type of the surface.
 * @param index :: Index selected in the surface type combo box.
 */
void InstrumentWidgetRenderTab::setSurfaceType(int index) {
  if ((int)m_instrWidget->getSurfaceType() != index) {
    m_instrWidget->setMaintainAspectRatio(m_maintainAspectRatio->isChecked());
    m_instrWidget->setSurfaceType(index);
    toggleTooltip(m_tooltipInfo->isChecked());
    m_instrWidget->setWireframe(m_wireframe->isChecked());
  }
}

/**
 * Respond to surface change from script.
 * @param index :: Index selected in the surface type combo box.
 */
void InstrumentWidgetRenderTab::surfaceTypeChanged(int index) {
  // display action's text on the render mode button
  QAction *action = m_surfaceTypeActionGroup->actions()[index];
  m_surfaceTypeButton->setText(action->text());

  // if action isn't checked then this method is called from script
  if (!action->isChecked()) {
    // checking action calls setSurfaceType slot
    action->setChecked(true);
  }
  showOrHideBoxes(index);
}

/**
 * Set to maintain aspect ratio.
 * @param on:: Boolean to turn maintain aspect ratio on or off.
 */
void InstrumentWidgetRenderTab::setMaintainAspectRatio(bool on) {
  if (m_instrWidget->getSurfaceType() == InstrumentWidget::FULL3D) {
    g_log.warning("Method to set maintain aspect ratio was ignored because the surface type is 'Full 3D'");
  } else {
    m_instrWidget->setMaintainAspectRatio(on);
  }
}

/**
 * Respond to change to maintain aspect ratio from script.
 * @param on :: Boolean to turn maintain aspect ratio on or off.
 */
void InstrumentWidgetRenderTab::maintainAspectRatioChanged(bool on) {
  if (m_maintainAspectRatio->isChecked() != on) {
    m_maintainAspectRatio->setChecked(on);
  }
}

/**
 * Respond to external change of the colormap.
 */
void InstrumentWidgetRenderTab::colorMapChanged() {
  const auto &instrumentActor = m_instrWidget->getInstrumentActor();
  if (!instrumentActor.isInitialized()) {
    return;
  }
  setupColorBar(instrumentActor.getColorMap(), instrumentActor.minValue(), instrumentActor.maxValue(),
                instrumentActor.minPositiveValue(), instrumentActor.autoscaling());
}

void InstrumentWidgetRenderTab::scaleTypeChanged(int type) { setScaleType(static_cast<ColorMap::ScaleType>(type)); }

void InstrumentWidgetRenderTab::nthPowerChanged(double nth_power) { m_colorBarWidget->setNthPower(nth_power); }

/**
 * Update the GUI element after the "Use OpenGL" option has been changed
 * programmatically.
 * @param on :: True for enabling OpenGL, false for disabling.
 */
void InstrumentWidgetRenderTab::glOptionChanged(bool on) {
  m_GLView->blockSignals(true);
  m_GLView->setChecked(on);
  m_GLView->blockSignals(false);
}

/**
 * Show the tooltip of an action which is attached to a menu.
 */
void InstrumentWidgetRenderTab::showMenuToolTip(QAction *action) {
  QToolTip::showText(QCursor::pos(), action->toolTip(), this);
}

/**
 * Set the offset in u-coordinate of a 2d (unwrapped) surface
 */
void InstrumentWidgetRenderTab::setUCorrection() {
  auto surface = getSurface();
  auto rotSurface = std::dynamic_pointer_cast<RotationSurface>(surface);
  if (rotSurface) {
    QPointF oldUCorr = rotSurface->getUCorrection();
    // ask the user to enter a number for the u-correction
    UCorrectionDialog dlg(this, oldUCorr, rotSurface->isManualUCorrection());
    if (dlg.exec() != QDialog::Accepted)
      return;

    QSettings settings;
    settings.beginGroup(m_instrWidget->getInstrumentSettingsGroupName());

    if (dlg.applyCorrection()) {
      QPointF ucorr = dlg.getValue();
      // update the surface only if the correction changes
      if (ucorr != oldUCorr) {
        rotSurface->setUCorrection(ucorr.x(),
                                   ucorr.y()); // manually set the correction
        rotSurface->requestRedraw();           // redraw the view
        settings.setValue(EntryManualUCorrection, true);
        settings.setValue(EntryUCorrectionMin, ucorr.x());
        settings.setValue(EntryUCorrectionMax, ucorr.y());
      }
    } else {
      rotSurface->setAutomaticUCorrection(); // switch to automatic correction
      rotSurface->requestRedraw();           // redraw the view
      settings.remove(EntryManualUCorrection);
      settings.remove(EntryUCorrectionMin);
      settings.remove(EntryUCorrectionMax);
    }
  }
}

/**
 * @brief InstrumentWidgetRenderTab::toggleToolTip
 * Change whether or not to display a tooltip with basic info when hovering over the instrument
 * @param activate : the new status
 */
void InstrumentWidgetRenderTab::toggleTooltip(bool activate) { getSurface()->toggleToolTip(activate); }

/**
 * Get current value for the u-correction for a RotationSurface.
 * Return 0 if it's not a RotationSurface.
 */
QPointF InstrumentWidgetRenderTab::getUCorrection() const {
  auto surface = getSurface();
  auto rotSurface = std::dynamic_pointer_cast<RotationSurface>(surface);
  if (rotSurface) {
    return rotSurface->getUCorrection();
  }
  return QPointF();
}

/**
 * Save widget render tab to a project file.
 * @return string representing the current state of the project file.
 */
std::string MantidQt::MantidWidgets::InstrumentWidgetRenderTab::saveToProject() const { return ""; }

/**
 * Load the state of the render tab from a project file.
 * @param lines :: lines defining the state of the render tab
 */
void InstrumentWidgetRenderTab::loadFromProject(const std::string &lines) {
  Q_UNUSED(lines);
  throw std::runtime_error("InstrumentActor::saveToProject() not implemented for Qt >= 5");
}

} // namespace MantidQt::MantidWidgets
