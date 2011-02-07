#include "InstrumentWindow.h"
#include "InstrumentWindowRenderTab.h"
#include "InstrumentWindowPickTab.h"
#include "../MantidUI.h"
#include "MantidKernel/ConfigService.h"
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
#include <QImageWriter>

#include <numeric>

using namespace Mantid::API;
using namespace Mantid::Geometry;

/**
 * Constructor.
 */
InstrumentWindow::InstrumentWindow(const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ): 
  MdiSubWindow(label, app, name, f), WorkspaceObserver(), mViewChanged(false)
{
  m_savedialog_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  setFocusPolicy(Qt::StrongFocus);
  setFocus();
  QFrame *frame = new QFrame();
  QVBoxLayout* mainLayout = new QVBoxLayout;
  QSplitter* controlPanelLayout = new QSplitter(Qt::Horizontal);

  //Add Tab control panel and Render window
  mControlsTab = new QTabWidget(0,0);
  controlPanelLayout->addWidget(mControlsTab);
  controlPanelLayout->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  // Create the display widget
  mInstrumentDisplay = new Instrument3DWidget();
  controlPanelLayout->addWidget(mInstrumentDisplay);
  mainLayout->addWidget(controlPanelLayout);

  // Load settings is called after mInstrumentDisplay is created but before m_renderTab
  loadSettings();

  //Render Controls
  m_renderTab = new InstrumentWindowRenderTab(this);
  mControlsTab->addTab( m_renderTab, QString("Render"));
  
  // Pick controls
  m_pickTab = new InstrumentWindowPickTab(this);
  mControlsTab->addTab( m_pickTab, QString("Pick"));

  // Instrument tree controls
  QFrame* instrumentTree=createInstrumentTreeTab(mControlsTab);
  mControlsTab->addTab( instrumentTree, QString("Instrument Tree"));

  connect(mControlsTab,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

  //Set the main frame to the window
  frame->setLayout(mainLayout);
  setWidget(frame);

  //Set the mouse/keyboard operation info
  mInteractionInfo = new QLabel();
  mainLayout->addWidget(mInteractionInfo);
  updateInteractionInfoText();  
  connect(mInstrumentDisplay, SIGNAL(actionDetectorHighlighted(const Instrument3DWidget::DetInfo &)),this,SLOT(detectorHighlighted(const Instrument3DWidget::DetInfo &)));
  connect(mInstrumentDisplay, SIGNAL(detectorsSelected()), this, SLOT(showPickOptions()));


  // Init actions
  mInfoAction = new QAction(tr("&Details"), this);
  connect(mInfoAction,SIGNAL(triggered()),this,SLOT(spectraInfoDialog()));

  mPlotAction = new QAction(tr("&Plot Spectra"), this);
  connect(mPlotAction,SIGNAL(triggered()),this,SLOT(plotSelectedSpectra()));

  mDetTableAction = new QAction(tr("&Extract Data"), this);
  connect(mDetTableAction, SIGNAL(triggered()), this, SLOT(showDetectorTable()));

  mGroupDetsAction = new QAction(tr("&Group"), this);
  connect(mGroupDetsAction, SIGNAL(triggered()), this, SLOT(groupDetectors()));

  mMaskDetsAction = new QAction(tr("&Mask"), this);
  connect(mMaskDetsAction, SIGNAL(triggered()), this, SLOT(maskDetectors()));

  askOnCloseEvent(app->confirmCloseInstrWindow);

  setAttribute(Qt::WA_DeleteOnClose);

  // Watch for the deletion of the associated workspace
  observeDelete();
  observeAfterReplace();
  observeADSClear();

  resize(600,650);
}

/**
  * Connected to QTabWidget::currentChanged signal
  */
void InstrumentWindow::tabChanged(int i)
{
  if (i == 1) 
  {
    mInstrumentDisplay->setInteractionModePick();
  }
  else
  {
    mInstrumentDisplay->setInteractionModeNormal();
  }

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
  if( this->isVisible() )
  {
    m_renderTab->setupColorBarScaling();
    mInstrumentDisplay->updateColorsForNewMap();
  }
}

void InstrumentWindow::showPickOptions()
{
  QMenu context(mInstrumentDisplay);
  
  context.addAction(mInfoAction);
  context.addAction(mPlotAction);
  context.addAction(mDetTableAction);

  if( mInstrumentDisplay->getSelectedWorkspaceIndices().size() > 1 )
  {
    context.insertSeparator();
    context.addAction(mGroupDetsAction);
    context.addAction(mMaskDetsAction);
  }

  context.exec(QCursor::pos());
  mInstrumentDisplay->hidePickBox();
}

/**
 * This is the detector information slot executed when a detector is highlighted by moving mouse in graphics widget.
 * @param information :: about the detector that is at the location of the users mouse pointer
 */
void InstrumentWindow::detectorHighlighted(const Instrument3DWidget::DetInfo & cursorPos)
{
  mInteractionInfo->setText(cursorPos.display());
  m_pickTab->updatePick(cursorPos);
}
/**
 * This is slot for the dialog to appear when a detector is picked and the info menu is selected
 */
void InstrumentWindow::spectraInfoDialog()
{
  QString info;
  const std::vector<int> & det_ids = mInstrumentDisplay->getSelectedDetectorIDs();
  const std::vector<int> & wksp_indices = mInstrumentDisplay->getSelectedWorkspaceIndices();
  const int ndets = det_ids.size();
  if( ndets == 1 )
  {
    info = QString("Workspace index: %1\nDetector ID: %2").arg(QString::number(wksp_indices.front()),QString::number(det_ids.front()));
  }
  else
  {
    info = QString("Index list size: %1\nDetector list size: %2").arg(QString::number(wksp_indices.size()), QString::number(ndets));
  }
  QMessageBox::information(this,tr("Detector/Spectrum Information"), info, 
			   QMessageBox::Ok|QMessageBox::Default, QMessageBox::NoButton, QMessageBox::NoButton);
}

/**
 *   Sends a signal to plot the selected spectrum.
 */
void InstrumentWindow::plotSelectedSpectra()
{
  std::set<int> indices(mInstrumentDisplay->getSelectedWorkspaceIndices().begin(), mInstrumentDisplay->getSelectedWorkspaceIndices().end());
  if (indices.count(-1) > 0)
  {
    indices.erase(-1);
  }
  emit plotSpectra( mInstrumentDisplay->getWorkspaceName(), indices);
}

/**
 * Show detector table
 */
void InstrumentWindow::showDetectorTable()
{
  emit createDetectorTable(mInstrumentDisplay->getWorkspaceName(), mInstrumentDisplay->getSelectedWorkspaceIndices(), true);
}

QString InstrumentWindow::confirmDetectorOperation(const QString & opName, const QString & inputWS, int ndets)
{
  QString message("This operation will affect %1 detectors.\nSelect output workspace option:");
  QMessageBox prompt(this);
  prompt.setWindowTitle("MantidPlot");
  prompt.setText(message.arg(QString::number(ndets)));
  QPushButton *replace = prompt.addButton("Replace", QMessageBox::ActionRole);
  QPushButton *create = prompt.addButton("New", QMessageBox::ActionRole);
  prompt.addButton("Cancel", QMessageBox::ActionRole);
  prompt.exec();
  QString outputWS;
  if( prompt.clickedButton() == replace )
  {
    outputWS = inputWS;
  }
  else if( prompt.clickedButton() == create )
  {
    outputWS = inputWS + "_" + opName;
  }
  else
  {
    outputWS = "";
  }
  return outputWS;
}
/**
 * Group selected detectors
 */
void InstrumentWindow::groupDetectors()
{
  const std::vector<int> & wksp_indices = mInstrumentDisplay->getSelectedWorkspaceIndices();
  const std::vector<int> & det_ids = mInstrumentDisplay->getSelectedDetectorIDs();
  QString inputWS = mInstrumentDisplay->getWorkspaceName();
  QString outputWS = confirmDetectorOperation("grouped", inputWS, static_cast<int>(det_ids.size()));
  if( outputWS.isEmpty() ) return;
  QString param_list = "InputWorkspace=%1;OutputWorkspace=%2;WorkspaceIndexList=%3;KeepUngroupedSpectra=1";
  emit execMantidAlgorithm("GroupDetectors",
			   param_list.arg(inputWS, outputWS, asString(wksp_indices))
			   );
}

/**
 * Mask selected detectors
 */
void InstrumentWindow::maskDetectors()
{
  const std::vector<int> & wksp_indices = mInstrumentDisplay->getSelectedWorkspaceIndices();

  //Following variable is unused:
  //const std::vector<int> & det_ids = mInstrumentDisplay->getSelectedDetectorIDs();

  QString inputWS = mInstrumentDisplay->getWorkspaceName();
  // Masking can only replace the input workspace so no need to ask for confirmation
  QString param_list = "Workspace=%1;WorkspaceIndexList=%2";
  QString indices = asString(mInstrumentDisplay->getSelectedWorkspaceIndices());
  emit execMantidAlgorithm("MaskDetectors",param_list.arg(inputWS, asString(wksp_indices)));
}

/**
 * Convert a list of integers to a comma separated string of numbers 
 */
QString InstrumentWindow::asString(const std::vector<int>& numbers) const
{
  QString num_str;
  std::vector<int>::const_iterator iend = numbers.end();
  for( std::vector<int>::const_iterator itr = numbers.begin(); itr < iend; ++itr )
  {
    num_str += QString::number(*itr) + ",";
  }
  //Remove trailing comma
  num_str.chop(1);
  return num_str;
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
    catch(std::bad_alloc &)
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
  mInstrumentDisplay->setWorkspace(QString::fromStdString(mWorkspaceName));
  // Need to check if the values have already been set for the range
  if( !mInstrumentDisplay->dataMinValueEdited() )
  {
    m_renderTab->setMinValue(mInstrumentDisplay->getDataMinValue(),false);
  }
  if( !mInstrumentDisplay->dataMaxValueEdited() )
  {
    m_renderTab->setMaxValue(mInstrumentDisplay->getDataMaxValue(),false);
  }
  
  // Setup the colour map details
  GraphOptions::ScaleType type = m_renderTab->getScaleType();
  mInstrumentDisplay->mutableColorMap().changeScaleType(type);
  m_renderTab->setupColorBarScaling();
  
  mInstrumentDisplay->resetUnwrappedViews();
  // Ensure the 3D display is up-to-date
  mInstrumentDisplay->update();
  // Populate the instrument tree
  mInstrumentTree->setInstrument(workspace->getInstrument());

  if ( ! mViewChanged )
  {
    // set the default view, the axis that the instrument is be viewed from initially can be set in the instrument definition and there is always a value in the Instrument
    QString axisName = QString::fromStdString(
    workspace->getInstrument()->getDefaultAxis());
    m_renderTab->setAxis(axisName);
    // this was an automatic view change, only flag that the view changed if the user initiated the change
    mViewChanged = false;
  }

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
  m_renderTab->setMinValue(minValue);
}

/// Set the maximumu value of the colour map
void InstrumentWindow::setColorMapMaxValue(double maxValue)
{
  m_renderTab->setMaxValue(maxValue);
}

void InstrumentWindow::setDataMappingIntegral(double minValue,double maxValue,bool entireRange)
{
  mInstrumentDisplay->setDataMappingIntegral(minValue, maxValue, entireRange);
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

  mViewChanged = true;
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

/**
 * Set the scale type programmatically
 * @param type :: The scale choice
 */
void InstrumentWindow::setScaleType(GraphOptions::ScaleType type)
{
  m_renderTab->setScaleType(type);
}

/// A slot for the mouse selection
void InstrumentWindow::componentSelected(const QItemSelection & selected, const QItemSelection &)
{
  QModelIndexList items = selected.indexes();
  if( items.isEmpty() ) return;

  if (mInstrumentDisplay->getRenderMode() == GL3DWidget::FULL3D)
  {
    double xmax(0.), xmin(0.), ymax(0.), ymin(0.), zmax(0.), zmin(0.);
    mInstrumentTree->getSelectedBoundingBox(items.first(), xmax, ymax, zmax, xmin, ymin, zmin);
    V3D pos = mInstrumentTree->getSamplePos();
    mInstrumentDisplay->setView(pos, xmax, ymax, zmax, xmin, ymin, zmin);
  }
  else
  {
    mInstrumentTree->sendComponentSelectedSignal(items.first());
  }

}

/**
 * This method opens a color dialog to pick the background color,
 * and then sets it.
 */
void InstrumentWindow::pickBackgroundColor()
{
	QColor color = QColorDialog::getColor(Qt::green,this);
	mInstrumentDisplay->setBackgroundColor(color);
}

void InstrumentWindow::saveImage()
{
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  QListIterator<QByteArray> itr(formats);
  QString filter("");
  while( itr.hasNext() )
  {
    filter += "*." + itr.next();
    if( itr.hasNext() )
    {
      filter += ";;";
    }
  }
  QString selectedFilter = "*.png";
  QString filename = QFileDialog::getSaveFileName(this, "Save image ...", m_savedialog_dir, filter, &selectedFilter);

  // If its empty, they cancelled the dialog
  if( filename.isEmpty() ) return;
  
  //Save the directory used
  QFileInfo finfo(filename);
  m_savedialog_dir = finfo.dir().path();

  QString ext = finfo.completeSuffix();
  if( ext.isEmpty() )
  {
    filename += selectedFilter.section("*", 1);
    ext = QFileInfo(filename).completeSuffix();
  }
  else
  {
    QStringList extlist = filter.split(";;");
    if( !extlist.contains("*." + ext) )
    {
      QMessageBox::warning(this, "MantidPlot", "Unsupported file extension, please use one from the supported list.");
      return;
    }
  }
  
  mInstrumentDisplay->saveToFile(filename);
}

/**
 * Update the text display that informs the user of the current mode and details about it
 */
void InstrumentWindow::updateInteractionInfoText()
{
  QString text;  
  if(mInstrumentDisplay->getInteractionMode() == Instrument3DWidget::PickMode)
	{
    text = tr("Mouse Button: Left -- Rotation, Middle -- Zoom, Right -- Translate\nKeyboard: NumKeys -- Rotation, PageUp/Down -- Zoom, ArrowKeys -- Translate");
    if( m_renderTab->areAxesOn() )
    {
      text += "\nAxes: X = Red; Y = Green; Z = Blue";
    }
  }
  else
  {
   text = tr("Use Mouse Left Button to Pick an detector\n Click on 'Normal' button to get into interactive mode");
  }
  mInteractionInfo->setText(text);
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
  
  GraphOptions::ScaleType type = (GraphOptions::ScaleType)settings.value("ScaleType", GraphOptions::Log10).toUInt();
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
  settings.setValue("ColormapFile", mCurrentColorMap);
  settings.endGroup();
}

/// Closes the window if the associated workspace is deleted
void InstrumentWindow::deleteHandle(const std::string & ws_name, boost::shared_ptr<Mantid::API::Workspace>)
{
  if (ws_name == mWorkspaceName)
  {
    askOnCloseEvent(false);
    close();
  }
}

void InstrumentWindow::afterReplaceHandle(const std::string& wsName,
					  const boost::shared_ptr<Mantid::API::Workspace>)
{
  //Replace current workspace
  if (wsName == mWorkspaceName)
  {
    updateWindow();
  }
}

void InstrumentWindow::clearADSHandle()
{
  askOnCloseEvent(false);
  close();
}


/**
 * This method saves the workspace name associated with the instrument window 
 * and geometry to a string.This is useful for loading/saving the project.
 */
QString InstrumentWindow::saveToString(const QString& geometry, bool saveAsTemplate)
{
  (void) saveAsTemplate;
	QString s="<instrumentwindow>\n";
	s+="WorkspaceName\t"+QString::fromStdString(mWorkspaceName)+"\n";
	s+=geometry;
	s+="</instrumentwindow>\n";
	return s;

}

/** 
 * Called just before a show event
 * @param A :: pointer to the QShowEvent object
 */
void InstrumentWindow::showEvent(QShowEvent* event)
{
  updateWindow();
}



QFrame * InstrumentWindow::createInstrumentTreeTab(QTabWidget* ControlsTab)
{
  QFrame* instrumentTree=new QFrame(ControlsTab);
  QVBoxLayout* instrumentTreeLayout=new QVBoxLayout(instrumentTree);
  //Tree Controls
  mInstrumentTree = new InstrumentTreeWidget(0);
  instrumentTreeLayout->addWidget(mInstrumentTree);
  connect(mInstrumentTree,SIGNAL(componentSelected(Mantid::Geometry::ComponentID)),
          mInstrumentDisplay,SLOT(componentSelected(Mantid::Geometry::ComponentID)));
  return instrumentTree;
}

