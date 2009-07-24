#include "InstrumentWindow.h"
#include "../MantidUI.h"
#include "MantidKernel/System.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "Poco/Path.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QSplitter>
#include <QDoubleValidator>
#include <QRadioButton>
#include <QGroupBox>
#include <QGridLayout>
#include <QComboBox>
#include <QSettings>
#include <QFileInfo>
#include <QColorDialog>
#include <QLineEdit>
#include <QCheckBox>
#include "qwt_scale_widget.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

using namespace Mantid::API;

/**
 * Constructor.
 */
InstrumentWindow::InstrumentWindow(const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ): 
  MdiSubWindow(label, app, name, f)
{
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	QFrame *frame = new QFrame();
	QVBoxLayout* mainLayout = new QVBoxLayout;
	QSplitter* controlPanelLayout = new QSplitter(Qt::Horizontal);

	//Add Tab control panel and Render window
	mControlsTab = new QTabWidget(0,0);
	controlPanelLayout->addWidget(mControlsTab);
	controlPanelLayout->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
	QFrame* renderControls=new QFrame(mControlsTab);
	QFrame* instrumentTree=new QFrame(mControlsTab);
	mControlsTab->addTab( renderControls, QString("Render Controls"));
	mControlsTab->addTab( instrumentTree, QString("Instrument Tree"));
	mInstrumentDisplay = new Instrument3DWidget();
	controlPanelLayout->addWidget(mInstrumentDisplay);
	mainLayout->addWidget(controlPanelLayout);
	QVBoxLayout* renderControlsLayout=new QVBoxLayout(renderControls);
	QVBoxLayout* instrumentTreeLayout=new QVBoxLayout(instrumentTree);
	//Tree Controls
	mInstrumentTree = new InstrumentTreeWidget(0);
	instrumentTreeLayout->addWidget(mInstrumentTree);
	//Render Controls
	mSelectButton = new QPushButton(tr("Pick"));
	mSelectColormap = new QPushButton(tr("Select ColorMap"));
	QPushButton* mSelectBin = new QPushButton(tr("Select X Range"));
	mBinMapDialog = new BinDialog(this);
	mColorMapWidget = new QwtScaleWidget(QwtScaleDraw::RightScale);

	mMinValueBox = new QLineEdit();
	mMaxValueBox = new QLineEdit();
	mMinValueBox->setMinimumWidth(40);
	mMaxValueBox->setMinimumWidth(40);
	mMinValueBox->setMaximumWidth(60);
	mMaxValueBox->setMaximumWidth(60);
	mMinValueBox->setValidator(new QDoubleValidator(mMinValueBox));
	mMaxValueBox->setValidator(new QDoubleValidator(mMaxValueBox));

	//Axis view buttons
	QFrame* axisViewFrame = new QFrame();
	QHBoxLayout* axisViewLayout = new QHBoxLayout();
	axisViewLayout->addWidget(new QLabel("Axis View:"));
	QComboBox* axisCombo=new QComboBox();
	axisCombo->addItem("Z+");
	axisCombo->addItem("Z-");
	axisCombo->addItem("X+");
	axisCombo->addItem("X-");
	axisCombo->addItem("Y+");
	axisCombo->addItem("Y-");
	axisViewLayout->addWidget(axisCombo);
	axisViewFrame->setLayout(axisViewLayout);

	//Colormap Frame widget
	QFrame* lColormapFrame = new QFrame();

	QVBoxLayout* lColormapLayout = new QVBoxLayout;
	lColormapLayout->addWidget(mMaxValueBox);
	lColormapLayout->addWidget(mColorMapWidget);
	lColormapLayout->addWidget(mMinValueBox);
	mColorMapWidget->setColorBarEnabled(true);
	mColorMapWidget->setColorBarWidth(20);
	mColorMapWidget->setAlignment(QwtScaleDraw::RightScale);
	mColorMapWidget->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter);

	mScaleOptions = new QComboBox;
	mScaleOptions->addItem("Log10", QVariant(MantidColorMap::Log10));
	mScaleOptions->addItem("Linear", QVariant(MantidColorMap::Linear));
	connect(mScaleOptions, SIGNAL(currentIndexChanged(int)), this, SLOT(scaleTypeChanged(int)));

	QVBoxLayout* options_layout = new QVBoxLayout;
	options_layout->addStretch();
	options_layout->addWidget(mScaleOptions);

	QHBoxLayout *colourmap_layout = new QHBoxLayout;
	colourmap_layout->addLayout(lColormapLayout);
	colourmap_layout->addLayout(options_layout);
	lColormapFrame->setLayout(colourmap_layout);


	//Pick background color
	QPushButton *btnBackgroundColor=new QPushButton("Pick Background");

	//Check box to toggle high resolution lighting
	mLightingToggle = new QCheckBox("High resolution &lighting", this);
	mLightingToggle->setToolTip("Toggle the use of a high resolution lighting and shading model.\n"
				    "This option is best used with a high-end graphics card."
				    "Note: Shading will alter the colours and hence accuracy of the image.");
	mLightingToggle->setCheckState(Qt::Unchecked);
	connect(mLightingToggle, SIGNAL(stateChanged(int)), mInstrumentDisplay, SLOT(setLightingState(int)));

	renderControlsLayout->addWidget(mSelectButton);
	renderControlsLayout->addWidget(mSelectBin);
	renderControlsLayout->addWidget(mSelectColormap);
	renderControlsLayout->addWidget(axisViewFrame);
	renderControlsLayout->addWidget(btnBackgroundColor);
	renderControlsLayout->addWidget(lColormapFrame);
	renderControlsLayout->addWidget(mLightingToggle);

	//Set the main frame to the window
	frame->setLayout(mainLayout);
	setWidget(frame);

	//Set the mouse/keyboard operation info
	mInteractionInfo=new QLabel(tr("Mouse Button: Left -- Rotation, Middle -- Zoom, Right -- Translate\nKeyboard: NumKeys -- Rotation, PageUp/Down -- Zoom, ArrowKeys -- Translate"));
	mInteractionInfo->setMaximumHeight(30);
	mainLayout->addWidget(mInteractionInfo);
	connect(mSelectButton, SIGNAL(clicked()), this,   SLOT(modeSelectButtonClicked()));
	connect(mSelectColormap,SIGNAL(clicked()), this, SLOT(changeColormap()));
	connect(mMinValueBox,SIGNAL(editingFinished()),this, SLOT(minValueChanged()));
	connect(mMaxValueBox,SIGNAL(editingFinished()),this, SLOT(maxValueChanged()));
	connect(mInstrumentDisplay, SIGNAL(actionSpectraSelected(int)), this, SLOT(spectraInformation(int)));
	connect(mInstrumentDisplay, SIGNAL(actionDetectorSelected(int)), this, SLOT(detectorInformation(int)));
	connect(mInstrumentDisplay, SIGNAL(actionDetectorHighlighted(int,int,int)),this,SLOT(detectorHighlighted(int,int,int)));
	connect(mInstrumentDisplay, SIGNAL(actionSpectraSelectedList(std::vector<int>)), this, SLOT(spectraListInformation(std::vector<int>)));
	connect(mInstrumentDisplay, SIGNAL(actionDetectorSelectedList(std::vector<int>)), this, SLOT(detectorListInformation(std::vector<int>)));
	connect(mSelectBin, SIGNAL(clicked()), this, SLOT(selectBinButtonClicked()));
	connect(mBinMapDialog,SIGNAL(IntegralMinMax(double,double)), mInstrumentDisplay, SLOT(setDataMappingIntegral(double,double)));
	connect(axisCombo,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(setViewDirection(const QString&)));
	connect(btnBackgroundColor,SIGNAL(clicked()),this,SLOT(pickBackgroundColor()));
	mPopupContext = new QMenu(mInstrumentDisplay);
	QAction* infoAction = new QAction(tr("&Info"), this);
	connect(infoAction,SIGNAL(triggered()),this,SLOT(spectraInfoDialog()));
	mPopupContext->addAction(infoAction);
	
	QAction* plotAction = new QAction(tr("&Plot spectra"), this);
	connect(plotAction,SIGNAL(triggered()),this,SLOT(sendPlotSpectraSignal()));
	mPopupContext->addAction(plotAction);
	
	mDetectorGroupPopupContext = new QMenu(mInstrumentDisplay);
	QAction* infoGroupAction = new QAction(tr("&Info"), this);
	connect(infoGroupAction,SIGNAL(triggered()),this,SLOT(spectraGroupInfoDialog()));
	mDetectorGroupPopupContext->addAction(infoGroupAction);
	QAction* plotGroupAction = new QAction(tr("&Plot spectra"), this);
	connect(plotGroupAction,SIGNAL(triggered()),this,SLOT(sendPlotSpectraGroupSignal()));
	mDetectorGroupPopupContext->addAction(plotGroupAction);
    
	// Load settings
	loadSettings();
    
	askOnCloseEvent(false);
	setAttribute(Qt::WA_DeleteOnClose);
}

/**
 * Mode select button slot.
 */
void InstrumentWindow::modeSelectButtonClicked()
{
	if(mSelectButton->text()=="Pick")
	{
		mSelectButton->setText("Normal");
		mInstrumentDisplay->setInteractionModePick();
		mInteractionInfo->setText(tr("Use Mouse Left Button to Pick an detector\n Click on 'Normal' button to get into interactive mode"));
	}
	else
	{
		mSelectButton->setText("Pick");
		mInstrumentDisplay->setInteractionModeNormal();
		mInteractionInfo->setText(tr("Mouse Button: Left -- Rotation, Middle -- Zoom, Right -- Translate\nKeyboard: NumKeys -- Rotation, PageUp/Down -- Zoom, ArrowKeys -- Translate"));
	}

}

void InstrumentWindow::selectBinButtonClicked()
{
  mBinMapDialog->setIntegralMinMax(mInstrumentDisplay->getBinMinValue(), mInstrumentDisplay->getBinMaxValue());
  mBinMapDialog->exec();
}

/**
 * Change color map button slot. This provides the file dialog box to select colormap or sets it directly a string is provided
 */
void InstrumentWindow::changeColormap(const QString &filename)
{
  QString fileselection;
  //Use a file dialog if no parameter is passed
  if( filename.isEmpty() )
  {
    fileselection = QFileDialog::getOpenFileName(this, tr("Pick a Colormap"), 
						 QFileInfo(mCurrentColorMap).absoluteFilePath(),
						 tr("Colormaps (*.map *.MAP)"));
    // User cancelled if filename is still empty
    if( fileselection.isEmpty() ) return;
  }
  else
  {
    fileselection = QFileInfo(filename).absoluteFilePath();
    if( !QFileInfo(fileselection).exists() ) return;
  }
  
  if( fileselection == mCurrentColorMap ) return;

  mCurrentColorMap = fileselection;
  mInstrumentDisplay->mutableColorMap().loadMap(mCurrentColorMap);
  setupColorBarScaling();
  mInstrumentDisplay->updateColorsForNewMap();
}

/**
 * This is the spectra information slot executed when a detector is picked/selected.
 */
void InstrumentWindow::spectraInformation(int value)
{
	mPopupContext->popup(QCursor::pos());
	mSpectraIDSelected=value;
}

/**
 * This is the detector information slot executed when a detector is picked in graphics widget.
 */
void InstrumentWindow::detectorInformation(int value)
{
	mDetectorIDSelected=value;
}
/**
 * This method is a slot for the collection of the spectra index list that was selected
 */
void InstrumentWindow::spectraListInformation(const std::vector<int>& result)
{
	mDetectorGroupPopupContext->popup(QCursor::pos());
	mSpectraIDSelectedList=result;
}
/**
 * This method is a slot for the collection of the detector list that was selected
 */
void InstrumentWindow::detectorListInformation(const std::vector<int>& result)
{
	mDetectorGroupPopupContext->popup(QCursor::pos());
	mDetectorIDSelectedList=result;
}

/**
 * This is the detector information slot executed when a detector is highlighted by moving mouse in graphics widget.
 */
void InstrumentWindow::detectorHighlighted(int detectorId,int spectraId,int count)
{
  QString txt("Detector ID: ");
  if(detectorId != -1) txt += QString::number(detectorId);
  txt+="\nSpectra ID: ";
  if(spectraId != -1) txt += QString::number(spectraId);
  txt += "  Count: ";
  if(detectorId != -1) txt += QString::number(count);
  mInteractionInfo->setText(txt);
}
/**
 * This is slot for the dialog to appear when a detector is picked and the info menu is selected
 */
void InstrumentWindow::spectraInfoDialog()
{
  QString info("Workspace Index: ");
  info += QString::number(mSpectraIDSelected);
  info +="\nDetector ID: ";
  info += QString::number(mDetectorIDSelected);
  QMessageBox::information(this,tr("Detector/Spectrum Information"), info, QMessageBox::Ok|QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
}

/**
 * Shows dialog with group of detectors information
 */
void InstrumentWindow::spectraGroupInfoDialog()
{
  QString info("Index list size: ");
  info += QString::number(mSpectraIDSelectedList.size());
  info +="\nDetector list size: ";
  info += QString::number(mDetectorIDSelectedList.size());
  QMessageBox::information(this,tr("Detector/Spectrum Information"), info, QMessageBox::Ok|QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);

}

/**
 *   Sends a signal to plot the selected spectrum.
 */
void InstrumentWindow::sendPlotSpectraSignal()
{
    emit plotSpectra( QString::fromStdString(mInstrumentDisplay->getWorkspaceName()), mSpectraIDSelected );
}

/**
 *   Sends a signal to plot the selected spectrum.
 */
void InstrumentWindow::sendPlotSpectraGroupSignal()
{
    emit plotSpectraList( QString::fromStdString(mInstrumentDisplay->getWorkspaceName()), mSpectraIDSelectedList );
}
/**
 * Destructor
 */
InstrumentWindow::~InstrumentWindow()
{
  saveSettings();
  delete mInstrumentDisplay;
}

/**
 * This method sets the workspace name for the Instrument
 */
void InstrumentWindow::setWorkspaceName(std::string wsName)
{
  mWorkspaceName = wsName;
}

void InstrumentWindow::showWindow()
{
    try
    {
        updateWindow();
        show();
    }
    catch(std::exception& e)
    {
        QMessageBox::critical(this,"MantidPlot - Error","Instrument Window failed to initialize due to the error:\n\n"+
            QString::fromStdString(e.what()));
    }
}

void InstrumentWindow::updateWindow()
{
  if( mWorkspaceName.empty() ) return;

  bool resultError=false;

  MatrixWorkspace_sptr workspace = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(mWorkspaceName));
  if( !workspace.get() ) return;

  try
  {
    renderInstrument(workspace.get());
  }
  catch(...)
  {
    mInstrumentDisplay->resetWidget();
    mInstrumentDisplay->setSlowRendering();
    resultError = true;
  }
  if(resultError)
  {
    QMessageBox::critical(this,"Mantid -- Error","Trying Slow Rendering");
    try
    {
      renderInstrument(workspace.get());
    }
    catch(std::bad_alloc &e)
    {
      QMessageBox::critical(this,"Mantid -- Error","not enough memory to display this instrument");
      mInstrumentDisplay->resetWidget();
    }
  }

  connect(mInstrumentTree->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
	  this, SLOT(componentSelected(const QItemSelection&, const QItemSelection&)));
}

void InstrumentWindow::renderInstrument(Mantid::API::MatrixWorkspace* workspace)
{
  mInstrumentDisplay->setWorkspace(mWorkspaceName);

  double minValue = mInstrumentDisplay->getDataMinValue();
  double maxValue = mInstrumentDisplay->getDataMaxValue();
  mMinValueBox->setText(QString::number(minValue));
  mMaxValueBox->setText(QString::number(maxValue));

  // Setup the colour map details
  MantidColorMap::ScaleType type = (MantidColorMap::ScaleType)mScaleOptions->itemData(mScaleOptions->currentIndex()).toUInt();
  mInstrumentDisplay->mutableColorMap().changeScaleType(type);
  setupColorBarScaling();
  
  // Ensure the 3D display is up-to-date
  mInstrumentDisplay->update();
  // Populate the instrument tree
  mInstrumentTree->setInstrument(workspace->getInstrument());
}

/// Set a maximum and minimum for the colour map range
void InstrumentWindow::setColorMapRange(double minValue, double maxValue)
{
  setColorMapMinValue(minValue);
  setColorMapMaxValue(maxValue);
}

/// Set the minimum value of the colour map
void InstrumentWindow::setColorMapMinValue(double minValue)
{
  mMinValueBox->setText(QString::number(minValue));
  minValueChanged();
}

/// Set the maximumu value of the colour map
void InstrumentWindow::setColorMapMaxValue(double maxValue)
{
  mMaxValueBox->setText(QString::number(maxValue));
  maxValueChanged();
}

/**
 *
 */
void InstrumentWindow::setupColorBarScaling()
{
  double minValue = mMinValueBox->displayText().toDouble();
  double maxValue = mMaxValueBox->displayText().toDouble();

  MantidColorMap::ScaleType type = (MantidColorMap::ScaleType)mScaleOptions->itemData(mScaleOptions->currentIndex()).toUInt();
  if( type == MantidColorMap::Linear )
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

void InstrumentWindow::setDataMappingIntegral(double minValue,double maxValue)
{
  mInstrumentDisplay->setDataMappingIntegral(minValue, maxValue);
}

/**
 *
 */
void InstrumentWindow::minValueChanged()
{
  double value = mMinValueBox->displayText().toDouble();
  if( this->isVisible() )
  { 
    setupColorBarScaling();
    mInstrumentDisplay->updateForNewMinData(value);
  }
}

/**
 *
 */
void InstrumentWindow::maxValueChanged()
{
  double value = mMaxValueBox->displayText().toDouble();
  if( this->isVisible() )
  { 
    setupColorBarScaling();
    mInstrumentDisplay->updateForNewMaxData(value);
  }
}



/**
 * This is the callback for the combo box that selects the view direction
 */
void InstrumentWindow::setViewDirection(const QString& input)
{
	if(input.compare("X+")==0)
	{
		mInstrumentDisplay->setViewDirectionXPositive();
	}
	else if(input.compare("X-")==0)
	{
		mInstrumentDisplay->setViewDirectionXNegative();
	}
	else if(input.compare("Y+")==0)
	{
		mInstrumentDisplay->setViewDirectionYPositive();
	}
	else if(input.compare("Y-")==0)
	{
		mInstrumentDisplay->setViewDirectionYNegative();
	}
	else if(input.compare("Z+")==0)
	{
		mInstrumentDisplay->setViewDirectionZPositive();
	}
	else if(input.compare("Z-")==0)
	{
		mInstrumentDisplay->setViewDirectionZNegative();
	}
}

/**
 * For the scripting API
 */
void InstrumentWindow::selectComponent(const QString & name)
{
  QModelIndex component = mInstrumentTree->findComponentByName(name);
  if( !component.isValid() ) return;

  mInstrumentTree->scrollTo(component, QAbstractItemView::EnsureVisible );
  mInstrumentTree->selectionModel()->select(component, QItemSelectionModel::Select);
}

/// A slot for the mouse selection
void InstrumentWindow::componentSelected(const QItemSelection & selected, const QItemSelection &)
{
  QModelIndexList items = selected.indexes();
  if( items.isEmpty() ) return;

  double xmax(0.), xmin(0.), ymax(0.), ymin(0.), zmax(0.), zmin(0.);
  mInstrumentTree->getSelectedBoundingBox(items.first(), xmax, ymax, zmax, xmin, ymin, zmin);
  Mantid::Geometry::V3D pos = mInstrumentTree->getSamplePos();
  mInstrumentDisplay->setView(pos, xmax, ymax, zmax, xmin, ymin, zmin);
}

/**
 * This method picks the background color
 */
void InstrumentWindow::pickBackgroundColor()
{
	QColor color = QColorDialog::getColor(Qt::green,this);
	mInstrumentDisplay->setBackgroundColor(color);
}

/**
 * A slot called when the scale type combo box's selection changes
 */
void InstrumentWindow::scaleTypeChanged(int index)
{
  MantidColorMap::ScaleType type = (MantidColorMap::ScaleType)mScaleOptions->itemData(index).toUInt();
  mInstrumentDisplay->mutableColorMap().changeScaleType(type);
  setupColorBarScaling();
  mInstrumentDisplay->recount();
}

/**
 * This method loads the setting from QSettings
 */
void InstrumentWindow::loadSettings()
{
  //Load Color
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  
  // Background colour
  mInstrumentDisplay->setBackgroundColor(settings.value("BackgroundColor",QColor(0,0,0,1.0)).value<QColor>());
  
  //Load Colormap. If the file is invalid the default stored colour map is used
  mCurrentColorMap = settings.value("ColormapFile", "").toString();
  // Set values from settings
  mInstrumentDisplay->mutableColorMap().loadMap(mCurrentColorMap);
  
  MantidColorMap::ScaleType type = (MantidColorMap::ScaleType)settings.value("ScaleType", MantidColorMap::Log10).toUInt();
  // Block signal emission temporarily since we have not fully initialized the window
  mScaleOptions->blockSignals(true);
  mScaleOptions->setCurrentIndex(mScaleOptions->findData(type));
  mScaleOptions->blockSignals(false);
  mInstrumentDisplay->mutableColorMap().changeScaleType(type);
  
  settings.endGroup();
}

/**
 * Save properties of the window a persistent store
 */
void InstrumentWindow::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  settings.setValue("BackgroundColor", mInstrumentDisplay->currentBackgroundColor());
  settings.setValue("ColormapFile", mCurrentColorMap);
  settings.setValue("ScaleType", mInstrumentDisplay->getColorMap().getScaleType());
  settings.endGroup();
}
