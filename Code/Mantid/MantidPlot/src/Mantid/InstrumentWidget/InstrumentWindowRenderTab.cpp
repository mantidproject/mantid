#include "InstrumentWindowRenderTab.h"
#include "ProjectionSurface.h"
#include "UnwrappedSurface.h"

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
#include <QSignalMapper>
#include <QMessageBox>

#include <qwt_scale_widget.h>
#include <qwt_scale_engine.h>

#include "MantidKernel/ConfigService.h"
#include "InstrumentWindow.h"
#include "BinDialog.h"
#include "ColorMapWidget.h"


InstrumentWindowRenderTab::InstrumentWindowRenderTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),m_instrWindow(instrWindow)
{
  m_InstrumentDisplay = m_instrWindow->getInstrumentDisplay();
  QVBoxLayout* renderControlsLayout=new QVBoxLayout(this);

  // Render Mode control
  m_renderMode = new QComboBox(this);
  m_renderMode->setToolTip("Set render mode");
  QStringList modeList;
  modeList << "Full 3D" << "Cylindrical X"  << "Cylindrical Y" << "Cylindrical Z" << "Spherical X" << "Spherical Y" << "Spherical Z";
  m_renderMode->insertItems(0,modeList);
  connect(m_renderMode,SIGNAL(currentIndexChanged(int)), this, SLOT(setSurfaceType(int)));

  // Save image control
  mSaveImage = new QPushButton(tr("Save image"));
  connect(mSaveImage, SIGNAL(clicked()), m_instrWindow, SLOT(saveImage()));

  // Setup Display Setting menu
  QPushButton* displaySettings = new QPushButton("Display Settings",this);
  QMenu* displaySettingsMenu = new QMenu(this);
  connect(displaySettingsMenu, SIGNAL(aboutToShow()),this,SLOT(displaySettingsAboutToshow()));
  m_colorMap = new QAction("Color Map",this);
  connect(m_colorMap,SIGNAL(triggered()),this,SLOT(changeColormap()));
  m_backgroundColor = new QAction("Background Color",this);
  connect(m_backgroundColor,SIGNAL(triggered()),m_instrWindow,SLOT(pickBackgroundColor()));
  m_lighting = new QAction("Lighting",this);
  m_lighting->setCheckable(true);
  m_lighting->setChecked(false);
  connect(m_lighting,SIGNAL(toggled(bool)),m_instrWindow->getInstrumentDisplay(),SLOT(enableLighting(bool)));
  m_displayAxes = new QAction("Display Axes",this);
  m_displayAxes->setCheckable(true);
  m_displayAxes->setChecked(true);
  connect(m_displayAxes, SIGNAL(toggled(bool)), this, SLOT(showAxes(bool)));
  m_wireframe = new QAction("Wireframe",this);
  m_wireframe->setCheckable(true);
  m_wireframe->setChecked(false);
  connect(m_wireframe, SIGNAL(toggled(bool)), m_instrWindow, SLOT(setWireframe(bool)));
  m_GLView = new QAction("Use OpenGL",this);
  m_GLView->setCheckable(true);
  m_GLView->setChecked( m_instrWindow->isGLEnabled() );
  connect(m_GLView, SIGNAL( toggled(bool) ), m_instrWindow, SLOT( enableGL(bool) ));
  displaySettingsMenu->addAction(m_colorMap);
  displaySettingsMenu->addAction(m_backgroundColor);
  displaySettingsMenu->addSeparator();
  displaySettingsMenu->addAction(m_displayAxes);
  displaySettingsMenu->addAction(m_wireframe);
  displaySettingsMenu->addAction(m_lighting);
  displaySettingsMenu->addAction(m_GLView);
  displaySettings->setMenu(displaySettingsMenu);

  QFrame * axisViewFrame = setupAxisFrame();

  // Colormap widget
  m_colorMapWidget = new ColorMapWidget(0,this);
  connect(m_colorMapWidget, SIGNAL(scaleTypeChanged(int)), m_instrWindow, SLOT(changeScaleType(int)));
  connect(m_colorMapWidget,SIGNAL(minValueChanged(double)),m_instrWindow, SLOT(changeColorMapMinValue(double)));
  connect(m_colorMapWidget,SIGNAL(maxValueChanged(double)),m_instrWindow, SLOT(changeColorMapMaxValue(double)));

  m_flipCheckBox = new QCheckBox("Flip view",this);
  m_flipCheckBox->setChecked(false);
  m_flipCheckBox->hide();
  connect(m_flipCheckBox,SIGNAL(toggled(bool)),this,SLOT(flipUnwrappedView(bool)));

  m_peakOverlaysButton = new QPushButton("Peaks options",this);
  m_peakOverlaysButton->hide();
  m_peakOverlaysButton->setMenu(createPeaksMenu());

  QHBoxLayout* unwrappedControlsLayout = new QHBoxLayout;
  unwrappedControlsLayout->addWidget(m_flipCheckBox);
  unwrappedControlsLayout->addWidget(m_peakOverlaysButton);

  m_autoscaling = new QCheckBox("Autoscaling",this);
  m_autoscaling->setChecked(true);
  connect(m_autoscaling,SIGNAL(toggled(bool)),this,SLOT(setColorMapAutoscaling(bool)));

  // layout
  renderControlsLayout->addWidget(m_renderMode);
  renderControlsLayout->addLayout(unwrappedControlsLayout);
  renderControlsLayout->addWidget(axisViewFrame);
  renderControlsLayout->addWidget(displaySettings);
  renderControlsLayout->addWidget(mSaveImage);
  renderControlsLayout->addWidget(m_colorMapWidget);
  renderControlsLayout->addWidget(m_autoscaling);

}

InstrumentWindowRenderTab::~InstrumentWindowRenderTab()
{
  saveSettings("Mantid/InstrumentWindow");
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

  loadSettings("Mantid/InstrumentWindow");
  return m_resetViewFrame;
}
void InstrumentWindowRenderTab::init()
{
  setAxis(QString::fromStdString(m_instrWindow->getInstrumentActor()->getInstrument()->getDefaultAxis()));
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
void InstrumentWindowRenderTab::changeColormap(const QString &filename)
{
  m_instrWindow->changeColormap(filename);
}

void InstrumentWindowRenderTab::loadSettings(const QString& section)
{
  QSettings settings;
  settings.beginGroup(section);
  int show3daxes = settings.value("3DAxesShown", 1 ).toInt();
  m_instrWindow->set3DAxesState(show3daxes != 0);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(show3daxes != 0);
  m_displayAxes->blockSignals(false);
  settings.endGroup();
}

void InstrumentWindowRenderTab::saveSettings(const QString& section)
{
  QSettings settings;
  settings.beginGroup(section);
  int val = 0;  if (m_displayAxes->isChecked()) val = 1;
  settings.setValue("3DAxesShown", QVariant(val));
  settings.endGroup();
}

void InstrumentWindowRenderTab::setMinValue(double value, bool apply)
{
  if (!apply) m_colorMapWidget->blockSignals(true);
  m_colorMapWidget->setMinValue(value);
  if (!apply) m_colorMapWidget->blockSignals(false);
}

void InstrumentWindowRenderTab::setMaxValue(double value, bool apply)
{
  if (!apply) m_colorMapWidget->blockSignals(true);
  m_colorMapWidget->setMaxValue(value);
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

void InstrumentWindowRenderTab::showAxes(bool on)
{
  m_instrWindow->set3DAxesState(on);
  m_displayAxes->blockSignals(true);
  m_displayAxes->setChecked(on);
  m_displayAxes->blockSignals(false);
}

void InstrumentWindowRenderTab::showEvent (QShowEvent *)
{
  ProjectionSurface* surface = m_InstrumentDisplay->getSurface();
  if (surface)
  {
    surface->setInteractionModeMove();
  }
}

/**
 * Update the surface type control to show type without emiting the signal.
 * @param type :: InstrumentWindow::SurfaceType.
 */
void InstrumentWindowRenderTab::updateSurfaceTypeControl(int type)
{
  m_renderMode->blockSignals(true);
  m_renderMode->setCurrentIndex(type);
  m_renderMode->blockSignals(false);
}

void InstrumentWindowRenderTab::flipUnwrappedView(bool on)
{
  UnwrappedSurface* surface = dynamic_cast<UnwrappedSurface*>(m_instrWindow->getSurface());
  if (!surface) return;
  surface->setFlippedView(on);
  m_instrWindow->updateWindow();
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
  settings.beginGroup("Mantid/InstrumentWindow");
  QMenu* menu = new QMenu(this);

  QAction* showRows = new QAction("Show rows",this);
  showRows->setCheckable(true);
  showRows->setChecked(settings.value("ShowPeakRows",true).toBool());
  connect(showRows,SIGNAL(toggled(bool)),m_instrWindow,SLOT(setShowPeakRowFlag(bool)));
  menu->addAction(showRows);
  // setting precision set of actions
  QMenu *setPrecision = new QMenu("Label precision",this);
  QSignalMapper *signalMapper = new QSignalMapper(this);
  for(int i = 1; i < 10; ++i)
  {
    QAction *prec = new QAction(QString::number(i),setPrecision);
    setPrecision->addAction(prec);
    connect(prec,SIGNAL(triggered()),signalMapper,SLOT(map()));
    signalMapper->setMapping(prec,i);
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
  // don't allow the simple viewer with 3D mode
  if ( index == InstrumentWindow::FULL3D && !m_instrWindow->isGLEnabled() )
  {
    m_renderMode->blockSignals( true );
    m_renderMode->setCurrentIndex( m_instrWindow->getSurfaceType() );
    m_renderMode->blockSignals( false );
    QMessageBox::warning(this,"MantidPlot - Warning","OpenGL must be enabled to view the instrument in 3D.\n"
      "Check \"Use OpenGL\" in Display Settings.");
    return;
  }
  m_instrWindow->setSurfaceType( index );
  showResetView( index );
  showFlipControl( index );
}
