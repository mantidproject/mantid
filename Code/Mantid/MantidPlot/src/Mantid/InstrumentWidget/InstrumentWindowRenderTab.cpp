#include "InstrumentWindowRenderTab.h"
#include "ProjectionSurface.h"
#include "UnwrappedSurface.h"
#include "Projection3D.h"
#include "RotationSurface.h"
#include "UCorrectionDialog.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QAction>
#include <QActionGroup>
#include <QSignalMapper>
#include <QToolTip>

#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>

#include "MantidKernel/ConfigService.h"
#include "InstrumentWindow.h"
#include "BinDialog.h"
#include "ColorMapWidget.h"

#include <limits>

// QSettings entry names
const char *EntryManualUCorrection = "ManualUCorrection";
const char *EntryUCorrectionMin = "UCorrectionMin";
const char *EntryUCorrectionMax = "UCorrectionMax";

InstrumentWindowRenderTab::InstrumentWindowRenderTab(InstrumentWindow* instrWindow):
InstrumentWindowTab(instrWindow)
{
  QVBoxLayout* renderControlsLayout=new QVBoxLayout(this);

  // Connect to InstrumentWindow signals
  connect(m_instrWindow,SIGNAL(surfaceTypeChanged(int)),this,SLOT(surfaceTypeChanged(int)));
  connect(m_instrWindow,SIGNAL(colorMapChanged()),this,SLOT(colorMapChanged()));
  connect(m_instrWindow,SIGNAL(colorMapMaxValueChanged(double)),this,SLOT(setMaxValue(double)));
  connect(m_instrWindow,SIGNAL(colorMapMinValueChanged(double)),this,SLOT(setMinValue(double)));
  connect(m_instrWindow,SIGNAL(colorMapRangeChanged(double,double)),this,SLOT(setRange(double,double)));
  connect(m_instrWindow,SIGNAL(scaleTypeChanged(int)),this,SLOT(scaleTypeChanged(int)));
  connect(m_instrWindow,SIGNAL(glOptionChanged(bool)),this,SLOT(glOptionChanged(bool)));

  // Surface type controls
  m_surfaceTypeButton = new QPushButton("Render mode",this);
  m_surfaceTypeButton->setToolTip("Set render mode");

  QSignalMapper *signalMapper = new QSignalMapper(this);
  connect(signalMapper,SIGNAL(mapped(int)),this,SLOT(setSurfaceType(int)));

  m_full3D = new QAction("Full 3D",this);
  m_full3D->setCheckable(true);
  connect(m_full3D,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_full3D, 0);
  m_cylindricalX = new QAction("Cylindrical X",this);
  m_cylindricalX->setCheckable(true);
  connect(m_cylindricalX,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_cylindricalX, 1);
  m_cylindricalY = new QAction("Cylindrical Y",this);
  m_cylindricalY->setCheckable(true);
  connect(m_cylindricalY,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_cylindricalY, 2);
  m_cylindricalZ = new QAction("Cylindrical Z",this);
  m_cylindricalZ->setCheckable(true);
  connect(m_cylindricalZ,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_cylindricalZ, 3);
  m_sphericalX = new QAction("Spherical X",this);
  m_sphericalX->setCheckable(true);
  connect(m_sphericalX,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_sphericalX, 4);
  m_sphericalY = new QAction("Spherical Y",this);
  m_sphericalY->setCheckable(true);
  connect(m_sphericalY,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_sphericalY, 5);
  m_sphericalZ = new QAction("Spherical Z",this);
  m_sphericalZ->setCheckable(true);
  connect(m_sphericalZ,SIGNAL(triggered()),signalMapper,SLOT(map()));
  signalMapper->setMapping(m_sphericalZ, 6);
  m_sideBySide = new QAction("Side by Side",this);
  m_sideBySide->setCheckable(true);
  connect(m_sideBySide,SIGNAL(triggered()),signalMapper,SLOT(map()));
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
  connect(renderModeMenu,SIGNAL(hovered(QAction*)),this,SLOT(showMenuToolTip(QAction*)));

  m_surfaceTypeButton->setMenu( renderModeMenu );

  // Save image control
  mSaveImage = new QPushButton(tr("Save image"));
  mSaveImage->setToolTip("Save the instrument image to a file");
  connect(mSaveImage, SIGNAL(clicked()), this, SLOT(saveImage()));

  // Setup Display Setting menu
  QPushButton* displaySettings = new QPushButton("Display Settings",this);
  QMenu* displaySettingsMenu = new QMenu(this);
  connect(displaySettingsMenu, SIGNAL(aboutToShow()),this,SLOT(displaySettingsAboutToshow()));
  m_colorMap = new QAction("Color Map",this);
  connect(m_colorMap,SIGNAL(triggered()),this,SLOT(changeColorMap()));
  m_backgroundColor = new QAction("Background Color",this);
  connect(m_backgroundColor,SIGNAL(triggered()),m_instrWindow,SLOT(pickBackgroundColor()));
  m_lighting = new QAction("Lighting",this);
  m_lighting->setCheckable(true);
  m_lighting->setChecked(false);
  connect(m_lighting,SIGNAL(toggled(bool)),m_instrWindow,SIGNAL(enableLighting(bool)));
  m_displayAxes = new QAction("Display Axes",this);
  m_displayAxes->setCheckable(true);
  m_displayAxes->setChecked(true);
  connect(m_displayAxes, SIGNAL(toggled(bool)), this, SLOT(showAxes(bool)));
  m_displayDetectorsOnly = new QAction("Display Detectors Only",this);
  m_displayDetectorsOnly->setCheckable(true);
  m_displayDetectorsOnly->setChecked(true);
  connect(m_displayDetectorsOnly, SIGNAL(toggled(bool)), this, SLOT(displayDetectorsOnly(bool)));
  m_wireframe = new QAction("Wireframe",this);
  m_wireframe->setCheckable(true);
  m_wireframe->setChecked(false);
  connect(m_wireframe, SIGNAL(toggled(bool)), m_instrWindow, SLOT(setWireframe(bool)));
  m_UCorrection = new QAction("U Correction",this);
  m_UCorrection->setToolTip("Manually set the limits on the horizontal axis.");
  connect(m_UCorrection, SIGNAL(triggered()),this,SLOT(setUCorrection()));
  
  // Create "Use OpenGL" action
  m_GLView = new QAction("Use OpenGL",this);
  m_GLView->setToolTip("Toggle use of OpenGL for unwrapped view. Default value can be set in Preferences.");
  m_GLView->setCheckable(true);
  QString setting = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().
  getString("MantidOptions.InstrumentView.UseOpenGL")).toUpper();
  bool useOpenGL = setting == "ON";
  connect(m_GLView, SIGNAL( toggled(bool) ), this, SLOT( enableGL(bool) ));
  enableGL( useOpenGL );

  displaySettingsMenu->addAction(m_colorMap);
  displaySettingsMenu->addAction(m_backgroundColor);
  displaySettingsMenu->addSeparator();
  displaySettingsMenu->addAction(m_displayAxes);
  displaySettingsMenu->addAction(m_displayDetectorsOnly);
  displaySettingsMenu->addAction(m_wireframe);
  displaySettingsMenu->addAction(m_lighting);
  displaySettingsMenu->addAction(m_GLView);
  displaySettingsMenu->addAction(m_UCorrection);

  displaySettings->setMenu(displaySettingsMenu);
  connect(displaySettingsMenu,SIGNAL(hovered(QAction*)),this,SLOT(showMenuToolTip(QAction*)));

  QFrame * axisViewFrame = setupAxisFrame();

  // Colormap widget
  m_colorMapWidget = new ColorMapWidget(0,this);
  connect(m_colorMapWidget, SIGNAL(scaleTypeChanged(int)), m_instrWindow, SLOT(changeScaleType(int)));
  connect(m_colorMapWidget,SIGNAL(minValueChanged(double)),m_instrWindow, SLOT(changeColorMapMinValue(double)));
  connect(m_colorMapWidget,SIGNAL(maxValueChanged(double)),m_instrWindow, SLOT(changeColorMapMaxValue(double)));

  m_flipCheckBox = new QCheckBox("Flip view",this);
  m_flipCheckBox->setToolTip("Flip the instrument view horizontally");
  m_flipCheckBox->setChecked(false);
  m_flipCheckBox->hide();
  connect(m_flipCheckBox,SIGNAL(toggled(bool)),this,SLOT(flipUnwrappedView(bool)));

  m_peakOverlaysButton = new QPushButton("Peaks options",this);
  m_peakOverlaysButton->setToolTip("Set peak overlay options");
  m_peakOverlaysButton->hide();
  m_peakOverlaysButton->setMenu(createPeaksMenu());

  QHBoxLayout* unwrappedControlsLayout = new QHBoxLayout;
  unwrappedControlsLayout->addWidget(m_flipCheckBox);
  unwrappedControlsLayout->addWidget(m_peakOverlaysButton);

  m_autoscaling = new QCheckBox("Autoscaling",this);
  m_autoscaling->setChecked(true);
  connect(m_autoscaling,SIGNAL(toggled(bool)),this,SLOT(setColorMapAutoscaling(bool)));

  // layout
  renderControlsLayout->addWidget(m_surfaceTypeButton);
  renderControlsLayout->addLayout(unwrappedControlsLayout);
  renderControlsLayout->addWidget(axisViewFrame);
  renderControlsLayout->addWidget(displaySettings);
  renderControlsLayout->addWidget(mSaveImage);
  renderControlsLayout->addWidget(m_colorMapWidget);
  renderControlsLayout->addWidget(m_autoscaling);

}

InstrumentWindowRenderTab::~InstrumentWindowRenderTab()
{
}

/** Sets up the controls and surrounding layout that allows uses to view the instrument
*  from an axis that they select
*  @return the QFrame that will be inserted on the main instrument view form
*/
QFrame * InstrumentWindowRenderTab::setupAxisFrame()
{
  m_resetViewFrame = new QFrame();
  QHBoxLayout* axisViewLayout = new QHBoxLayout();
  axisViewLayout->addWidget(new QLabel("Axis View:"));

  mAxisCombo = new QComboBox();
  mAxisCombo->addItem("Z+");
  mAxisCombo->addItem("Z-");
  mAxisCombo->addItem("X+");
  mAxisCombo->addItem("X-");
  mAxisCombo->addItem("Y+");
  mAxisCombo->addItem("Y-");

  axisViewLayout->addWidget(mAxisCombo);
  m_resetViewFrame->setLayout(axisViewLayout);

  connect(mAxisCombo,SIGNAL(currentIndexChanged(const QString&)),m_instrWindow,SLOT(setViewDirection(const QString&)));

  return m_resetViewFrame;
}

/**
  * Set checked n-th menu item in m_setPrecison menu.
  */
void InstrumentWindowRenderTab::setPrecisionMenuItemChecked(int n)
{
    for(int i = 0; i < m_precisionActions.size(); ++i)
    {
        QAction *prec = m_precisionActions[i];
        if (i == n - 1)
        {
            prec->setChecked( true );
            break;
        }
    }
}

/**
 * Enable/disable the Full 3D menu option
 * @param on :: True to enable.
 */
void InstrumentWindowRenderTab::enable3DSurface(bool on)
{
    m_full3D->setEnabled( on );
    if ( on )
    {
        m_full3D->setToolTip("");
    }
    else
    {
        m_full3D->setToolTip("Disabled: check \"Use OpenGL\" option in Display Settings to enable");
    }
}

/**
  * Surface-specific adjustments. 
  */
void InstrumentWindowRenderTab::initSurface()
{
  setAxis(QString::fromStdString(m_instrWindow->getInstrumentActor()->getInstrument()->getDefaultAxis()));
  auto surface = getSurface();

  // 3D axes switch needs to be shown for the 3D surface
  auto p3d = boost::dynamic_pointer_cast<Projection3D>(surface);
  if ( p3d )
  {
      p3d->set3DAxesState(areAxesOn());
  }

  bool detectorsOnly = !m_instrWindow->getInstrumentActor()->areGuidesShown();
  m_displayDetectorsOnly->blockSignals(true);
  m_displayDetectorsOnly->setChecked(detectorsOnly);
  m_displayDetectorsOnly->blockSignals(false);
  setPrecisionMenuItemChecked(surface->getPeakLabelPrecision());
  
  // enable u-correction for surfaces of rotation. correction applied in the last
  // session is loaded and re-applied in the new session
  auto rotSurface = boost::dynamic_pointer_cast<RotationSurface>(surface);
  if ( rotSurface )
  {
    m_UCorrection->setEnabled(true);
    QString groupName = m_instrWindow->getInstrumentSettingsGroupName();
    QSettings settings;
    settings.beginGroup(  groupName );
    bool isManualUCorrection = settings.value(EntryManualUCorrection,false).asBool();
    if ( isManualUCorrection )
    {
      double ucorrMin = settings.value(EntryUCorrectionMin,0.0).asDouble();
      double ucorrMax = settings.value(EntryUCorrectionMax,0.0).asDouble();
      rotSurface->setUCorrection( ucorrMin, ucorrMax );
    }
  }
  else
  {
    m_UCorrection->setEnabled(false);
  }
}

/**
 *
 */
void InstrumentWindowRenderTab::setupColorBarScaling(const MantidColorMap& cmap,double minPositive)
{
  m_colorMapWidget->setMinPositiveValue(minPositive);
  m_colorMapWidget->setupColorBarScaling(cmap);
}

/**
 * Change color map button slot. This provides the file dialog box to select colormap or sets it directly a string is provided
 */
void InstrumentWindowRenderTab::changeColorMap(const QString &filename)
{
  m_instrWindow->changeColormap(filename);
}

void InstrumentWindowRenderTab::loadSettings(const QSettings& settings)
{
  int show3daxes = settings.value("3DAxesShown", 1 ).toInt();
  m_instrWindow->set3DAxesState(show3daxes != 0);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(show3daxes != 0);
  m_displayAxes->blockSignals(false);
}

void InstrumentWindowRenderTab::saveSettings(QSettings& settings) const
{
  int val = 0;  if (m_displayAxes->isChecked()) val = 1;
  settings.setValue("3DAxesShown", QVariant(val));
}

/**
  * Set minimum value on the colormap scale.
  * @param value :: New value to set.
  * @param apply ::
  */
void InstrumentWindowRenderTab::setMinValue(double value, bool apply)
{
  if (!apply) m_colorMapWidget->blockSignals(true);
  m_colorMapWidget->setMinValue(value);
  if (!apply) m_colorMapWidget->blockSignals(false);
}

/**
  * Set maximum value on the colormap scale.
  * @param value :: New value to set.
  * @param apply ::
  */
void InstrumentWindowRenderTab::setMaxValue(double value, bool apply)
{
  if (!apply) m_colorMapWidget->blockSignals(true);
  m_colorMapWidget->setMaxValue(value);
  if (!apply) m_colorMapWidget->blockSignals(false);
}

/**
  * Set minimum and maximum values on the colormap scale.
  * @param minValue :: New min value to set.
  * @param maxValue :: New max value to set.
  * @param apply ::
  */
void InstrumentWindowRenderTab::setRange(double minValue, double maxValue, bool apply)
{
    if (!apply) m_colorMapWidget->blockSignals(true);
    m_colorMapWidget->setMinValue(minValue);
    m_colorMapWidget->setMaxValue(maxValue);
    if (!apply) m_colorMapWidget->blockSignals(false);
}

GraphOptions::ScaleType InstrumentWindowRenderTab::getScaleType()const
{
  return (GraphOptions::ScaleType)m_colorMapWidget->getScaleType();
}

void InstrumentWindowRenderTab::setScaleType(GraphOptions::ScaleType type)
{
  m_colorMapWidget->setScaleType(type);
}

void InstrumentWindowRenderTab::setAxis(const QString& axisNameArg)
{
    QString axisName = axisNameArg.toUpper();
    int axisInd = mAxisCombo->findText(axisName.toUpper());
    if (axisInd < 0) axisInd = 0;
    mAxisCombo->setCurrentIndex(axisInd);
}

bool InstrumentWindowRenderTab::areAxesOn()const
{
  return m_displayAxes->isChecked();
}

/**
  * Show ResetView combo box only with 3D view
  * @param iv Index of a render mode in RenderMode combo box. iv == 0 is 3D view
  */
void InstrumentWindowRenderTab::showResetView(int iv)
{
  m_resetViewFrame->setVisible(iv == 0);
}

void InstrumentWindowRenderTab::showFlipControl(int iv)
{
  bool vis = iv != 0;
  m_flipCheckBox->setVisible(vis);
  m_peakOverlaysButton->setVisible(vis);
}

/**
 * Toggle display of 3D axes.
 * 
 * @param on :: True of false for on and off.
 */
void InstrumentWindowRenderTab::showAxes(bool on)
{
  m_instrWindow->set3DAxesState(on);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(on);
  m_displayAxes->blockSignals(false);
}

/**
 * Toggle display of guide and other non-detector components.
 *
 * @param yes :: True of false for on and off.
 */
void InstrumentWindowRenderTab::displayDetectorsOnly(bool yes)
{
  m_instrWindow->getInstrumentActor()->showGuides( !yes );
  m_instrWindow->updateInstrumentView();
  m_displayDetectorsOnly->blockSignals(true);
  m_displayDetectorsOnly->setChecked(yes);
  m_displayDetectorsOnly->blockSignals(false);
}

/**
 * Toggle use of OpenGL
 *
 * @param on :: True of false for on and off.
 */
void InstrumentWindowRenderTab::enableGL(bool on)
{
  m_instrWindow->enableGL(on);
  m_GLView->blockSignals(true);
  m_GLView->setChecked(m_instrWindow->isGLEnabled());
  m_GLView->blockSignals(false);
  enable3DSurface(on);
}


void InstrumentWindowRenderTab::showEvent (QShowEvent *)
{
  auto surface = getSurface();
  if (surface)
  {
    surface->setInteractionMode(ProjectionSurface::MoveMode);
  }
  InstrumentActor* actor = m_instrWindow->getInstrumentActor();
  if ( actor )
  {
    auto visitor = SetAllVisibleVisitor(actor->areGuidesShown());
    actor->accept( visitor );
    getSurface()->updateView();
    getSurface()->requestRedraw();
  }
}

void InstrumentWindowRenderTab::flipUnwrappedView(bool on)
{
  auto surface = boost::dynamic_pointer_cast<UnwrappedSurface>(m_instrWindow->getSurface());
  if (!surface) return;
  surface->setFlippedView(on);
  m_instrWindow->updateInstrumentView();
  // Sync checkbox
  m_flipCheckBox->blockSignals(true);
  m_flipCheckBox->setChecked(on);
  m_flipCheckBox->blockSignals(false);

}

/**
 * Saves the current image buffer to the given file. An empty string raises a dialog
 * for finding the file
 * @param filename Optional full path of the saved image
 */
void InstrumentWindowRenderTab::saveImage(QString filename)
{
  m_instrWindow->saveImage(filename);
}

/**
 * Reset the colorbar parameters.
 * @param cmap :: A new Mantid color map.
 * @param minValue :: A new minimum value.
 * @param maxValue :: A new maximum value.
 * @param minPositive :: A new minimum positive value for the log scale. 
 * @param autoscaling :: Flag to set autoscaling of the color
 */
void InstrumentWindowRenderTab::setupColorBar(const MantidColorMap& cmap,double minValue,double maxValue,double minPositive,bool autoscaling)
{
  setMinValue(minValue,false);
  setMaxValue(maxValue,false);
  m_colorMapWidget->setMinPositiveValue(minPositive);
  m_colorMapWidget->setupColorBarScaling(cmap);
  m_autoscaling->blockSignals(true);
  m_autoscaling->setChecked(autoscaling);
  m_autoscaling->blockSignals(false);
}

/**
 * Set on / off autoscaling of the color bar.
 */
void InstrumentWindowRenderTab::setColorMapAutoscaling(bool on)
{
  emit setAutoscaling(on);
}

/**
 * Creates a menu for interaction with peak overlays
 */
QMenu* InstrumentWindowRenderTab::createPeaksMenu()
{
  QSettings settings;
  settings.beginGroup( m_instrWindow->getSettingsGroupName() );
  QMenu* menu = new QMenu(this);

  // show/hide peak hkl labels
  QAction *showLabels = new QAction("Show labels",this);
  showLabels->setCheckable(true);
  showLabels->setChecked(settings.value("ShowPeakLabels",true).toBool());
  connect(showLabels,SIGNAL(toggled(bool)),m_instrWindow,SLOT(setShowPeakLabelsFlag(bool)));
  menu->addAction(showLabels);
  // show/hide peak table rows
  QAction *showRows = new QAction("Show rows",this);
  showRows->setCheckable(true);
  showRows->setChecked(settings.value("ShowPeakRows",true).toBool());
  connect(showRows,SIGNAL(toggled(bool)),m_instrWindow,SLOT(setShowPeakRowFlag(bool)));
  connect(showLabels,SIGNAL(toggled(bool)),showRows,SLOT(setEnabled(bool)));
  showRows->setEnabled( showLabels->isChecked() );
  menu->addAction(showRows);
  // setting precision set of actions
  QMenu *setPrecision = new QMenu("Label precision",this);
  m_precisionActionGroup = new QActionGroup(this);
  QSignalMapper *signalMapper = new QSignalMapper(this);
  for(int i = 1; i < 10; ++i)
  {
    QAction *prec = new QAction(QString::number(i),setPrecision);
    prec->setCheckable(true);
    setPrecision->addAction(prec);
    connect(prec,SIGNAL(triggered()),signalMapper,SLOT(map()));
    signalMapper->setMapping(prec,i);
    m_precisionActions.append(prec);
    m_precisionActionGroup->addAction(prec);
  }
  connect(signalMapper, SIGNAL(mapped(int)), m_instrWindow, SLOT(setPeakLabelPrecision(int)));
  menu->addMenu(setPrecision);

  // Clear peaks action
  QAction* clearPeaks = new QAction("Clear peaks",this);
  connect(clearPeaks,SIGNAL(triggered()),m_instrWindow, SLOT(clearPeakOverlays()));
  menu->addAction(clearPeaks);
  return menu;
}

/**
 * Called before the display setting menu opens. Filters out menu options.
 */
void InstrumentWindowRenderTab::displaySettingsAboutToshow()
{
  if ( m_instrWindow->getSurfaceType() == InstrumentWindow::FULL3D )
  {
    // in 3D mode use GL widget only and allow lighting
    m_GLView->setEnabled( false );
    m_lighting->setEnabled( true );
  }
  else
  {
    // in flat view mode allow changing to simple, non-GL viewer
    m_GLView->setEnabled( true );
    // allow lighting in GL viewer only
    if ( !m_GLView->isChecked() )
    {
      m_lighting->setEnabled( false );
    }
    else
    {
      m_lighting->setEnabled( true );
    }
  }
}

/**
 * Change the type of the surface.
 * @param index :: Index selected in the surface type combo box.
 */
void InstrumentWindowRenderTab::setSurfaceType(int index)
{
    if ( (int) m_instrWindow->getSurfaceType() != index )
    {
        m_instrWindow->setSurfaceType( index );
    }
}

/**
  * Respond to surface change from script.
  * @param index :: Index selected in the surface type combo box.
  */
void InstrumentWindowRenderTab::surfaceTypeChanged(int index)
{
    // display action's text on the render mode button
    QAction *action = m_surfaceTypeActionGroup->actions()[index];
    m_surfaceTypeButton->setText( action->text() );

    // if action isn't checked then this method is called from script
    if ( !action->isChecked() )
    {
        // checking action calls setSurfaceType slot
        action->setChecked(true);
    }
    showFlipControl( index );
    showResetView( index );
}

/**
  * Respond to external change of the colormap.
  */
void InstrumentWindowRenderTab::colorMapChanged()
{
    InstrumentActor *instrumentActor = m_instrWindow->getInstrumentActor();
    setupColorBar(
      instrumentActor->getColorMap(),
      instrumentActor->minValue(),
      instrumentActor->maxValue(),
      instrumentActor->minPositiveValue(),
      instrumentActor->autoscaling()
                );
}

void InstrumentWindowRenderTab::scaleTypeChanged(int type)
{
    setScaleType((GraphOptions::ScaleType)type);
}

/**
  * Update the GUI element after the "Use OpenGL" option has been changed
  * programmatically.
  * @param on :: True for enabling OpenGL, false for disabling.
  */
void InstrumentWindowRenderTab::glOptionChanged(bool on)
{
    m_GLView->blockSignals(true);
    m_GLView->setChecked(on);
    m_GLView->blockSignals(false);
}

/**
  * Show the tooltip of an action which is attached to a menu.
  */
void InstrumentWindowRenderTab::showMenuToolTip(QAction *action)
{
    QToolTip::showText(QCursor::pos(),action->toolTip(),this);
}

/**
  * Set the offset in u-coordinate of a 2d (unwrapped) surface
  */
void InstrumentWindowRenderTab::setUCorrection()
{
  auto surface = getSurface();
  auto rotSurface = boost::dynamic_pointer_cast<RotationSurface>(surface);
  if ( rotSurface )
  {
    QPointF oldUCorr = rotSurface->getUCorrection();
    // ask the user to enter a number for the u-correction
    UCorrectionDialog dlg(this, oldUCorr, rotSurface->isManualUCorrection());
    if ( dlg.exec() != QDialog::Accepted ) return;
    
    QSettings settings;
    settings.beginGroup( m_instrWindow->getInstrumentSettingsGroupName() );

    if ( dlg.applyCorrection() )
    {
      QPointF ucorr = dlg.getValue();
      // update the surface only if the correction changes
      if ( ucorr != oldUCorr )
      {
        rotSurface->setUCorrection( ucorr.x(), ucorr.y() ); // manually set the correction
        rotSurface->requestRedraw();         // redraw the view
        settings.setValue(EntryManualUCorrection,true);
        settings.setValue(EntryUCorrectionMin,ucorr.x());
        settings.setValue(EntryUCorrectionMax,ucorr.y());
      }
    }
    else
    {
      rotSurface->setAutomaticUCorrection(); // switch to automatic correction
      rotSurface->requestRedraw();         // redraw the view
      settings.removeEntry(EntryManualUCorrection);
      settings.removeEntry(EntryUCorrectionMin);
      settings.removeEntry(EntryUCorrectionMax);
    }
  }
}

/**
  * Get current value for the u-correction for a RotationSurface.
  * Return 0 if it's not a RotationSurface.
  */
QPointF InstrumentWindowRenderTab::getUCorrection() const
{
  auto surface = getSurface();
  auto rotSurface = boost::dynamic_pointer_cast<RotationSurface>(surface);
  if ( rotSurface )
  {
    return rotSurface->getUCorrection();
  }
  return QPointF();
}

