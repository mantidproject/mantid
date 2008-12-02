#include "InstrumentWindow.h"
#include "../MantidUI.h"
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
#include "GLColorMapQwt.h"
#include "qwt_scale_widget.h"
#include "qwt_scale_div.h"
#include "qwt_scale_engine.h"

/**
 * Contructor, creates the mdi subwindow within mantidplot
 */
InstrumentWindow::InstrumentWindow(const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ):MdiSubWindow(label,app,name,f)
{
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	QFrame *frame= new QFrame();
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
	QPushButton* mSelectBin = new QPushButton(tr("Select Bin"));
	mBinMapDialog = new BinDialog(this);
	mColorMapWidget = new QwtScaleWidget(QwtScaleDraw::RightScale);
	mMinValueBox    = new QLineEdit();
	mMaxValueBox    = new QLineEdit();
	mMinValueBox->setValidator(new QDoubleValidator(mMinValueBox));
	mMaxValueBox->setValidator(new QDoubleValidator(mMaxValueBox));
	mMinValueBox->setMaximumWidth(40);
	mMaxValueBox->setMaximumWidth(40);
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
	//QFrame* axisViewFrame = new QFrame();
	//QGridLayout* axisViewLayout =new QGridLayout;
	//QPushButton* xPosView=new QPushButton(tr("X+"));
	//QPushButton* xNegView=new QPushButton(tr("X-"));
	//QPushButton* yPosView=new QPushButton(tr("Y+"));
	//QPushButton* yNegView=new QPushButton(tr("Y-"));
	//QPushButton* zPosView=new QPushButton(tr("Z+"));
	//QPushButton* zNegView=new QPushButton(tr("Z-"));
	//axisViewLayout->addWidget(xPosView,0,0);
	//axisViewLayout->addWidget(xNegView,0,1);
	//axisViewLayout->addWidget(yPosView,1,0);
	//axisViewLayout->addWidget(yNegView,1,1);
	//axisViewLayout->addWidget(zPosView,2,0);
	//axisViewLayout->addWidget(zNegView,2,1);
	//axisViewFrame->setLayout(axisViewLayout);

	//Colormap Frame widget
	QFrame* lColormapFrame=new QFrame();
	QVBoxLayout* lColormapLayout=new QVBoxLayout(lColormapFrame);
	lColormapLayout->addWidget(mMaxValueBox);
	lColormapLayout->addWidget(mColorMapWidget);
	lColormapLayout->addWidget(mMinValueBox);
	mColorMapWidget->setColorMap(QwtDoubleInterval(0,1),mInstrumentDisplay->getColorMap());
	mColorMapWidget->setColorBarEnabled(true);
	mColorMapWidget->setColorBarWidth(20);
	mColorMapWidget->setAlignment(QwtScaleDraw::RightScale);
	mColorMapWidget->setLabelAlignment( Qt::AlignRight | Qt::AlignVCenter);
	QwtLinearScaleEngine* lse=new QwtLinearScaleEngine();	
	mColorMapWidget->setScaleDiv(lse->transformation(),lse->divideScale(0,1,5,5));
	renderControlsLayout->addWidget(mSelectButton);
	renderControlsLayout->addWidget(mSelectBin);
	renderControlsLayout->addWidget(mSelectColormap);
	renderControlsLayout->addWidget(axisViewFrame);
	renderControlsLayout->addWidget(lColormapFrame);

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
	connect(mSelectBin, SIGNAL(clicked()), mBinMapDialog,SLOT(exec()));
	connect(mBinMapDialog,SIGNAL(SingleBinNumber(int)), mInstrumentDisplay, SLOT(setDataMappingSingleBin(int)));
	connect(mBinMapDialog,SIGNAL(IntegralMinMax(int,int)), mInstrumentDisplay, SLOT(setDataMappingIntegral(int,int)));
	connect(axisCombo,SIGNAL(currentIndexChanged(const QString&)),this,SLOT(setViewDirection(const QString&)));
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

	askOnCloseEvent(false);
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

/**
 * Change color map button slot. This provides the file dialog box to select colormap.
 */
void InstrumentWindow::changeColormap()
{
	QString file=QFileDialog::getOpenFileName(this, tr("Pick a Colormap"), ".",tr("Colormaps (*.map *.MAP)"));
	mInstrumentDisplay->setColorMapName(std::string(file.ascii()));
	updateColorMapWidget();
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
void InstrumentWindow::spectraListInformation(std::vector<int> result)
{
	mDetectorGroupPopupContext->popup(QCursor::pos());
	mSpectraIDSelectedList=result;
}
/**
 * This method is a slot for the collection of the detector list that was selected
 */
void InstrumentWindow::detectorListInformation(std::vector<int> result)
{
	mDetectorGroupPopupContext->popup(QCursor::pos());
	mDetectorIDSelectedList=result;
}

/**
 * This is the detector information slot executed when a detector is highlighted by moving mouse in graphics widget.
 */
void InstrumentWindow::detectorHighlighted(int detectorId,int spectraId,int count)
{
	QString txt;
	QString number;
	txt+="Detector Id: ";
	if(detectorId!=-1)
		txt+=number.setNum(detectorId);
	txt+="\nSpectra Id: ";
	if(detectorId!=-1)
		txt+=number.setNum(spectraId);
	txt+="  Count: ";
	if(detectorId!=-1)
		txt+=number.setNum(count);
	mInteractionInfo->setText(txt);
}
/**
 * This is slot for the dialog to appear when a detector is picked and the info menu is selected
 */
void InstrumentWindow::spectraInfoDialog()
{
	QString info;
	info+=" The Spectra Index Id: ";
	info+= QString::number(mSpectraIDSelected);
	info+=" \nThe Detector Id: ";
	info+= QString::number(mDetectorIDSelected);
	QMessageBox::information(this,tr("Detector/Spectrum Information"), info, QMessageBox::Ok|QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
}

/**
 * Shows dialog with group of detectors information
 */
void InstrumentWindow::spectraGroupInfoDialog()
{
	QString info;
	info+=" The Spectra Index Number: ";
	info+= QString::number(mSpectraIDSelectedList.size());
	info+=" \nThe Detector Id Numbers: ";
	info+= QString::number(mDetectorIDSelectedList.size());
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
	delete mInstrumentDisplay;
}

/**
 * This method sets the workspace name for the Instrument
 */
void InstrumentWindow::setWorkspaceName(std::string wsName)
{
	mInstrumentDisplay->setWorkspace(wsName);
	Workspace_sptr output = AnalysisDataService::Instance().retrieve(mInstrumentDisplay->getWorkspaceName());
	int count=output->blocksize();
	double minValue=mInstrumentDisplay->getDataMinValue();
	double maxValue=mInstrumentDisplay->getDataMaxValue();
	QString text;
	mMinValueBox->setText(text.setNum(minValue));
	mMaxValueBox->setText(text.setNum(maxValue));
	updateColorMapWidget();
	mInstrumentTree->setInstrument(output->getInstrument().get());
}

/**
 *
 */
void InstrumentWindow::minValueChanged()
{
	QString value=mMinValueBox->displayText();
	mInstrumentDisplay->setColorMapMinValue(value.toDouble());
	updateColorMapWidget();
}

/**
 *
 */
void InstrumentWindow::maxValueChanged()
{
	QString value=mMaxValueBox->displayText();
	mInstrumentDisplay->setColorMapMaxValue(value.toDouble());
	updateColorMapWidget();
}

/**
 *
 */
void InstrumentWindow::updateColorMapWidget()
{
	QwtLinearScaleEngine lse;
	double minValue=mMinValueBox->displayText().toDouble();
	double maxValue=mMaxValueBox->displayText().toDouble();
	mColorMapWidget->setScaleDiv(lse.transformation(),lse.divideScale(minValue,maxValue,20,5));
	mColorMapWidget->setColorMap(QwtDoubleInterval(minValue,maxValue),mInstrumentDisplay->getColorMap());
	mInstrumentDisplay->update();
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
