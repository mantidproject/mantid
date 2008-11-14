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
	connect(mSelectBin, SIGNAL(clicked()), mBinMapDialog,SLOT(exec()));
	connect(mBinMapDialog,SIGNAL(SingleBinNumber(int)), mInstrumentDisplay, SLOT(setDataMappingSingleBin(int)));
	connect(mBinMapDialog,SIGNAL(IntegralMinMax(int,int)), mInstrumentDisplay, SLOT(setDataMappingIntegral(int,int)));

    mPopupContext = new QMenu(mInstrumentDisplay);
	QAction* infoAction = new QAction(tr("&Info"), this);
	connect(infoAction,SIGNAL(triggered()),this,SLOT(spectraInfoDialog()));
    mPopupContext->addAction(infoAction);

	QAction* plotAction = new QAction(tr("&Plot spectra"), this);
	connect(plotAction,SIGNAL(triggered()),this,SLOT(sendPlotSpectraSignal()));
    mPopupContext->addAction(plotAction);
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
 * This is slot for the dialog to appear when a detector is picked and the info menu is selected
 */
void InstrumentWindow::spectraInfoDialog()
{
	QString info;
	info+=" The Spectra Index Id: ";
	info+= QString::number(mSpectraIDSelected);
	info+=" \n The Detector Id: ";
	info+= QString::number(mDetectorIDSelected);
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
}