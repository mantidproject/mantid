#include "InstrumentWindow.h"
#include "InstrumentWindowRenderTab.h"
#include "BinDialog.h"
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

  QComboBox* renderMode = new QComboBox(this);
  renderMode->setToolTip("Set render mode");
  QStringList modeList;
  modeList << "Full 3D" << "Cylindrical Y" << "Cylindrical Z" << "Cylindrical X" << "Spherical Y" << "Spherical Z" << "Spherical X";
  renderMode->insertItems(0,modeList);
  connect(renderMode,SIGNAL(currentIndexChanged(int)),m_instrWindow->getInstrumentDisplay(),SLOT(setRenderMode(int)));
  connect(renderMode, SIGNAL(currentIndexChanged(int)), this, SLOT(showResetView(int)));

  QPushButton* mSelectBin = new QPushButton(tr("Select X Range"));
  connect(mSelectBin, SIGNAL(clicked()), this, SLOT(selectBinButtonClicked()));

  mBinDialog = new BinDialog(this);
  connect(mBinDialog,SIGNAL(IntegralMinMax(double,double,bool)), m_instrWindow->getInstrumentDisplay(), SLOT(setDataMappingIntegral(double,double,bool)));

  mColorMapWidget = new QwtScaleWidget(QwtScaleDraw::RightScale);
  mColorMapWidget->setColorBarEnabled(true);
  mColorMapWidget->setColorBarWidth(20);
  mColorMapWidget->setAlignment(QwtScaleDraw::RightScale);
  mColorMapWidget->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter);

  //Save image control
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
  connect(m_displayAxes, SIGNAL(toggled(bool)), m_instrWindow->getInstrumentDisplay(), SLOT(set3DAxesState(bool)));
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

  mMinValueBox = new QLineEdit();
  mMaxValueBox = new QLineEdit();
  mMinValueBox->setMinimumWidth(40);
  mMaxValueBox->setMinimumWidth(40);
  mMinValueBox->setMaximumWidth(60);
  mMaxValueBox->setMaximumWidth(60);
  mMinValueBox->setValidator(new QDoubleValidator(mMinValueBox));
  mMaxValueBox->setValidator(new QDoubleValidator(mMaxValueBox));
  //Ensure the boxes start empty, this is important for checking if values have been set from the scripting side
  mMinValueBox->setText("");
  mMaxValueBox->setText("");

  QFrame * axisViewFrame = setupAxisFrame();

  //Colormap Frame widget
  QFrame* lColormapFrame = new QFrame();

  QVBoxLayout* lColormapLayout = new QVBoxLayout;
  lColormapLayout->addWidget(mMaxValueBox);
  lColormapLayout->addWidget(mColorMapWidget);
  lColormapLayout->addWidget(mMinValueBox);

  mScaleOptions = new QComboBox;
  mScaleOptions->addItem("Log10", QVariant(GraphOptions::Log10));
  mScaleOptions->addItem("Linear", QVariant(GraphOptions::Linear));
  mScaleOptions->setCurrentIndex(mScaleOptions->findData(
      mInstrumentDisplay->mutableColorMap().getScaleType()
    ));
  connect(mScaleOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleTypeChanged(int)));

  QVBoxLayout* options_layout = new QVBoxLayout;
  options_layout->addStretch();
  options_layout->addWidget(mScaleOptions);

  QHBoxLayout *colourmap_layout = new QHBoxLayout;
  colourmap_layout->addLayout(lColormapLayout);
  colourmap_layout->addLayout(options_layout);
  lColormapFrame->setLayout(colourmap_layout);

  //Check box to toggle orientation axes
  //m3DAxesToggle = new QCheckBox("Show 3D &Axes", this);
  //m3DAxesToggle->setToolTip("Toggle the display of 3D axes (X=Red; Y=Green; Z=Blue).");
  //m3DAxesToggle->setCheckState(Qt::Checked);
  //connect(m3DAxesToggle, SIGNAL(stateChanged(int)), m_instrWindow->getInstrumentDisplay(), SLOT(set3DAxesState(int)));
  //connect(m3DAxesToggle, SIGNAL(stateChanged(int)), m_instrWindow, SLOT(updateInteractionInfoText()));

  //Check box to toggle polygon mode
  //QCheckBox* poligonMOdeToggle = new QCheckBox("Show wireframe", this);
  //poligonMOdeToggle->setToolTip("Toggle the wireframe polygon mode.");
  //poligonMOdeToggle->setCheckState(Qt::Unchecked);
  //connect(poligonMOdeToggle, SIGNAL(clicked(bool)), m_instrWindow->getInstrumentDisplay(), SLOT(setWireframe(bool)));
  
  renderControlsLayout->addWidget(renderMode);
  renderControlsLayout->addWidget(axisViewFrame);
  renderControlsLayout->addWidget(displaySettings);

  renderControlsLayout->addWidget(mSelectBin);
  renderControlsLayout->addWidget(mSaveImage);
  renderControlsLayout->addWidget(lColormapFrame);
  //renderControlsLayout->addWidget(m3DAxesToggle);
  //renderControlsLayout->addWidget(poligonMOdeToggle);


  connect(mMinValueBox,SIGNAL(editingFinished()),this, SLOT(minValueChanged()));
  connect(mMaxValueBox,SIGNAL(editingFinished()),this, SLOT(maxValueChanged()));

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
    GraphOptions::ScaleType type = (GraphOptions::ScaleType)mScaleOptions->itemData(index).toUInt();
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
  double minValue = mMinValueBox->displayText().toDouble();
  double maxValue = mMaxValueBox->displayText().toDouble();

  GraphOptions::ScaleType type = (GraphOptions::ScaleType)mScaleOptions->itemData(mScaleOptions->currentIndex()).toUInt();
  if( type == GraphOptions::Linear )
  {
    QwtLinearScaleEngine linScaler;
    mColorMapWidget->setScaleDiv(linScaler.transformation(), linScaler.divideScale(minValue, maxValue,  20, 5));
    mColorMapWidget->setColorMap(QwtDoubleInterval(minValue, maxValue),mInstrumentDisplay->getColorMap());
  }
  else
 {
    QwtLog10ScaleEngine logScaler;    
    double logmin(minValue);
    if( logmin < 1.0 )
    {
      logmin = 1.0;
    }
    mColorMapWidget->setScaleDiv(logScaler.transformation(), logScaler.divideScale(logmin, maxValue, 20, 5));
    mColorMapWidget->setColorMap(QwtDoubleInterval(minValue, maxValue), mInstrumentDisplay->getColorMap());
  }
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
void InstrumentWindowRenderTab::minValueChanged()
{
  double updated_value = mMinValueBox->displayText().toDouble();
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
    mMinValueBox->setText(QString::number(old_value));
  }
}

/**
 *
 */
void InstrumentWindowRenderTab::maxValueChanged()
{
  double updated_value = mMaxValueBox->displayText().toDouble();
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
    mMaxValueBox->setText(QString::number(old_value));
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
  mMinValueBox->setText(QString::number(value));
  if (apply) minValueChanged();
}

void InstrumentWindowRenderTab::setMaxValue(double value, bool apply)
{
  mMaxValueBox->setText(QString::number(value));
  if (apply) maxValueChanged();
}

GraphOptions::ScaleType InstrumentWindowRenderTab::getScaleType()const
{
  return (GraphOptions::ScaleType)mScaleOptions->itemData(mScaleOptions->currentIndex()).toUInt();
}

void InstrumentWindowRenderTab::setScaleType(GraphOptions::ScaleType type)
{
  mScaleOptions->setCurrentIndex(mScaleOptions->findData(type));
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
