#include "InstrumentWindow.h"
#include "InstrumentWindowRenderTab.h"
#include "BinDialog.h"
#include "ColorMapWidget.h"
#include "MantidKernel/ConfigService.h"

#include "qwt_scale_widget.h"
#include "qwt_scale_engine.h"
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

InstrumentWindowRenderTab::InstrumentWindowRenderTab(InstrumentWindow* instrWindow):
QFrame(instrWindow),m_instrWindow(instrWindow)
{
  mInstrumentDisplay = m_instrWindow->getInstrumentDisplay();
  QVBoxLayout* renderControlsLayout=new QVBoxLayout(this);

  // Render Mode control
  QComboBox* renderMode = new QComboBox(this);
  renderMode->setToolTip("Set render mode");
  QStringList modeList;
  modeList << "Full 3D" << "Cylindrical Y" << "Cylindrical Z" << "Cylindrical X" << "Spherical Y" << "Spherical Z" << "Spherical X";
  renderMode->insertItems(0,modeList);
  connect(renderMode,SIGNAL(currentIndexChanged(int)),m_instrWindow,SLOT(setRenderMode(int)));
  connect(renderMode, SIGNAL(currentIndexChanged(int)), this, SLOT(showResetView(int)));

  // X selection control
  QPushButton* mSelectBin = new QPushButton(tr("Select X Range"));
  connect(mSelectBin, SIGNAL(clicked()), this, SLOT(selectBinButtonClicked()));
  mBinDialog = new BinDialog(this);
  connect(mBinDialog,SIGNAL(IntegralMinMax(double,double,bool)), m_instrWindow->getInstrumentDisplay(), SLOT(setDataMappingIntegral(double,double,bool)));

  // Save image control
  mSaveImage = new QPushButton(tr("Save image"));
  connect(mSaveImage, SIGNAL(clicked()), m_instrWindow, SLOT(saveImage()));

  // Setup Display Setting menu
  QPushButton* displaySettings = new QPushButton("Display Settings",this);
  QMenu* displaySettingsMenu = new QMenu(this);
  m_colorMap = new QAction("Color Map",this);
  connect(m_colorMap,SIGNAL(triggered()),this,SLOT(changeColormap()));
  m_backgroundColor = new QAction("Background Colour",this);
  connect(m_backgroundColor,SIGNAL(triggered()),m_instrWindow,SLOT(pickBackgroundColor()));
  m_lighting = new QAction("Lighting",this);
  m_lighting->setCheckable(true);
  m_lighting->setChecked(false);
  connect(m_lighting,SIGNAL(toggled(bool)),mInstrumentDisplay,SLOT(enableLighting(bool)));
  m_displayAxes = new QAction("Display Axes",this);
  m_displayAxes->setCheckable(true);
  m_displayAxes->setChecked(true);
  connect(m_displayAxes, SIGNAL(toggled(bool)), m_instrWindow, SLOT(set3DAxesState(bool)));
  connect(m_displayAxes, SIGNAL(toggled(bool)), m_instrWindow, SLOT(updateInteractionInfoText()));
  m_wireframe = new QAction("Wireframe",this);
  m_wireframe->setCheckable(true);
  m_wireframe->setChecked(false);
  connect(m_wireframe, SIGNAL(toggled(bool)), m_instrWindow->getInstrumentDisplay(), SLOT(setWireframe(bool)));
  displaySettingsMenu->addAction(m_colorMap);
  displaySettingsMenu->addAction(m_backgroundColor);
  displaySettingsMenu->addSeparator();
  displaySettingsMenu->addAction(m_displayAxes);
  displaySettingsMenu->addAction(m_wireframe);
  //displaySettingsMenu->addAction(m_lighting); // enable for testing
  displaySettings->setMenu(displaySettingsMenu);

  QFrame * axisViewFrame = setupAxisFrame();

  // Colormap widget
  m_colorMapWidget = new ColorMapWidget(mInstrumentDisplay->getColorMap().getScaleType(),this);
  connect(m_colorMapWidget, SIGNAL(scaleTypeChanged(int)), this, SLOT(scaleTypeChanged(int)));
  connect(m_colorMapWidget,SIGNAL(minValueChanged(double)),this, SLOT(minValueChanged(double)));
  connect(m_colorMapWidget,SIGNAL(maxValueChanged(double)),this, SLOT(maxValueChanged(double)));

  // layout
  renderControlsLayout->addWidget(renderMode);
  renderControlsLayout->addWidget(axisViewFrame);
  renderControlsLayout->addWidget(displaySettings);
  renderControlsLayout->addWidget(mSelectBin);
  renderControlsLayout->addWidget(mSaveImage);
  renderControlsLayout->addWidget(m_colorMapWidget);


  loadSettings("Mantid/InstrumentWindow");
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

  return m_resetViewFrame;
}

/**
 * A slot called when the scale type combo box's selection changes
 */
void InstrumentWindowRenderTab::scaleTypeChanged(int index)
{
  if( m_instrWindow->isVisible() )
  {
    GraphOptions::ScaleType type = (GraphOptions::ScaleType)index;
    mInstrumentDisplay->mutableColorMap().changeScaleType(type);
    setupColorBarScaling();
    mInstrumentDisplay->recount();
  }
}

/**
 *
 */
void InstrumentWindowRenderTab::setupColorBarScaling()
{

  m_colorMapWidget->setupColorBarScaling(mInstrumentDisplay->getColorMap());
}

/**
 * Change color map button slot. This provides the file dialog box to select colormap or sets it directly a string is provided
 */
void InstrumentWindowRenderTab::changeColormap(const QString &filename)
{
  m_instrWindow->changeColormap(filename);
  setupColorBarScaling();
}

/**
 *
 */
void InstrumentWindowRenderTab::minValueChanged(double value)
{
  double updated_value = value;
  double old_value = mInstrumentDisplay->getDataMinValue();
  // If the new value is the same
  if( std::abs( (updated_value - old_value) / old_value) < 1e-08 ) return;
  //Check it is less than the max
  if( updated_value < mInstrumentDisplay->getDataMaxValue() )
  {
    mInstrumentDisplay->setMinData(updated_value);

    if( this->isVisible() )
    { 
      setupColorBarScaling();
      mInstrumentDisplay->recount();
    }
  }
  else
  {
    // Invalid. Reset value.
    m_colorMapWidget->blockSignals(true);
    m_colorMapWidget->setMinValue(old_value);
    m_colorMapWidget->blockSignals(false);
  }
}

/**
 *
 */
void InstrumentWindowRenderTab::maxValueChanged(double value)
{
  double updated_value = value;
  double old_value = mInstrumentDisplay->getDataMaxValue();
  // If the new value is the same
  if( std::abs( (updated_value - old_value) / old_value) < 1e-08 ) return;
  // Check that it is valid
  if( updated_value > mInstrumentDisplay->getDataMinValue() )
  {
    mInstrumentDisplay->setMaxData(updated_value);

    if( this->isVisible() )
    { 
      setupColorBarScaling();
      mInstrumentDisplay->recount();
    }
  }
  else
  {
    // Invalid. Reset
    m_colorMapWidget->blockSignals(true);
    m_colorMapWidget->setMaxValue(old_value);
    m_colorMapWidget->blockSignals(false);
  }
}

void InstrumentWindowRenderTab::selectBinButtonClicked()
{
  //At this point (only) do we calculate the bin ranges.
  mInstrumentDisplay->calculateBinRange();
  //Set the values found + the bool for entire range
  mBinDialog->setIntegralMinMax(mInstrumentDisplay->getBinMinValue(), mInstrumentDisplay->getBinMaxValue(), mInstrumentDisplay->getBinEntireRange());
  //Show the dialog
  mBinDialog->exec();
}

void InstrumentWindowRenderTab::loadSettings(const QString& section)
{
  QSettings settings;
  settings.beginGroup(section);
  int show3daxes = settings.value("3DAxesShown", 1 ).toInt();
  m_displayAxes->setChecked(show3daxes != 0);
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
