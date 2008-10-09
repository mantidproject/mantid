#include "InstrumentWindow.h"
#include "../MantidUI.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>	
#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QString>


/**
 * Contructor, creates the mdi subwindow within mantidplot
 */
InstrumentWindow::InstrumentWindow(const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ):MdiSubWindow(label,app,name,f)
{
	setFocusPolicy(Qt::StrongFocus);
	setFocus();
	QFrame *frame= new QFrame();
	QVBoxLayout* mainLayout = new QVBoxLayout(frame);
	QHBoxLayout* controlPanelLayout = new QHBoxLayout;

	mainLayout->addLayout(controlPanelLayout);
	
	mSelectButton = new QPushButton(tr("Pick"));
	mTimeBinSlider = new QSlider(Qt::Horizontal);
	mTimeBinSpinBox= new QSpinBox();
	mPlayButton = new QPushButton(tr("Play"));
	mPauseButton = new QPushButton(tr("Stop"));
	mSelectColormap = new QPushButton(tr("Pick ColorMap"));
	mInstrumentDisplay = new Instrument3DWidget();	

	controlPanelLayout->addWidget(mSelectButton);
	controlPanelLayout->addStretch();
	controlPanelLayout->addWidget(mTimeBinSpinBox);
	controlPanelLayout->addStretch();
	controlPanelLayout->addWidget(mTimeBinSlider);
	controlPanelLayout->addStretch();
	controlPanelLayout->addWidget(mPlayButton);
	controlPanelLayout->addStretch();
	controlPanelLayout->addWidget(mPauseButton);
	controlPanelLayout->addStretch();
	controlPanelLayout->addWidget(mSelectColormap);

	mainLayout->addWidget(mInstrumentDisplay);
	connect(mSelectButton, SIGNAL(clicked()), this,   SLOT(modeSelectButtonClicked()));
	connect(mPlayButton, SIGNAL(clicked()),   mInstrumentDisplay,   SLOT(startAnimation()));
	connect(mPauseButton, SIGNAL(clicked()),   mInstrumentDisplay,   SLOT(stopAnimation()));
	connect(mSelectColormap,SIGNAL(clicked()), this, SLOT(changeColormap()));
	connect(mInstrumentDisplay, SIGNAL(actionSpectraSelected(int)), this, SLOT(spectraInformation(int)));
	connect(mInstrumentDisplay, SIGNAL(actionDetectorSelected(int)), this, SLOT(detectorInformation(int)));

	connect(mTimeBinSpinBox,SIGNAL(valueChanged(int)),mInstrumentDisplay, SLOT(setTimeBin(int)));
	connect(mTimeBinSlider,SIGNAL(valueChanged(int)),mInstrumentDisplay, SLOT(setTimeBin(int)));
	connect(mTimeBinSpinBox,SIGNAL(valueChanged(int)),mTimeBinSlider, SLOT(setValue(int)));
	connect(mTimeBinSlider,SIGNAL(valueChanged(int)),mTimeBinSpinBox, SLOT(setValue(int)));

	setWidget(frame);
    mPopupContext = new QMenu(mInstrumentDisplay);
	QAction* infoAction = new QAction(tr("&Info"), this);
	connect(infoAction,SIGNAL(triggered()),this,SLOT(spectraInfoDialog()));
    mPopupContext->addAction(infoAction);

	QAction* plotAction = new QAction(tr("&Plot spectra"), this);
	connect(plotAction,SIGNAL(triggered()),this,SLOT(sendPlotSpectraSignal()));
    mPopupContext->addAction(plotAction);

    mAnimationTimer = new QTimer(this);
	connect(mAnimationTimer, SIGNAL(timeout()), this, SLOT(updateTimeBin()));
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
	}
	else
	{
		mSelectButton->setText("Pick");
		mInstrumentDisplay->setInteractionModeNormal();
	}

}

/**
 * Change color map button slot. This provides the file dialog box to select colormap.
 */
void InstrumentWindow::changeColormap()
{
	QString file=QFileDialog::getOpenFileName(this, tr("Pick a Colormap"), ".",tr("Colormaps (*.map *.MAP)"));
	mInstrumentDisplay->setColorMapName(std::string(file.ascii()));

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
	int count=output->getNumberHistograms();
	mTimeBinSpinBox->setRange(0,count);
	mTimeBinSlider->setRange(0,count);
}

/**
 * This method is to start the animation. Steping throught the timebins.
 * It uses QTimer to loop through the animation. the animation start from begining
 * when the last time bin is displayed.
 * WARNING: Currently the time speed is set to 1 sec.
 */
void InstrumentWindow::startAnimation()
{
	mAnimationTimer->start(1000);
}

/**
 * This method is to stop the animation.
 */
void InstrumentWindow::stopAnimation()
{
	mAnimationTimer->stop();
}

/**
 * This method is slot method for the update of the time bin and increment to the next step. if reaches the last time bin
 * then restarts from first bin.
 */
void InstrumentWindow::updateTimeBin()
{
    Workspace_sptr output = AnalysisDataService::Instance().retrieve(mInstrumentDisplay->getWorkspaceName());
	int count=output->getNumberHistograms();
	int iTimeBin=mTimeBinSpinBox->value();
	if(iTimeBin>count)iTimeBin=0;
	iTimeBin++;
	mTimeBinSpinBox->setValue(iTimeBin);
	mInstrumentDisplay->setTimeBin(iTimeBin);
}