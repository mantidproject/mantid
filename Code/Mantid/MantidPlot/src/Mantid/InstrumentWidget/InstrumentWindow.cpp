#include "InstrumentWindow.h"
#include "InstrumentWindowRenderTab.h"
#include "InstrumentWindowPickTab.h"
#include "InstrumentWindowMaskTab.h"
#include "InstrumentWindowTreeTab.h"
#include "XIntegrationControl.h"
#include "InstrumentActor.h"
#include "UnwrappedCylinder.h"
#include "UnwrappedSphere.h"
#include "Projection3D.h"
#include "SimpleWidget.h"
#include "../MantidUI.h"
#include "../AlgorithmMonitor.h"

#include "MantidKernel/ConfigService.h"
#include "MantidAPI/IPeaksWorkspace.h"

#include <Poco/Path.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
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
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QStackedLayout>
#include <QKeyEvent>

#include "MantidQtAPI/FileDialogHandler.h"

#include <numeric>
#include <fstream>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::API;

/**
 * Constructor.
 */
InstrumentWindow::InstrumentWindow(const QString& wsName, const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ):
  MdiSubWindow(app, label, name, f), WorkspaceObserver(),
  m_InstrumentDisplay(NULL),
  m_simpleDisplay(NULL),
  m_workspaceName(wsName),
  m_instrumentActor(NULL),
  mViewChanged(false), 
  m_blocked(false),
  m_instrumentDisplayContextMenuOn(false)
{
  m_surfaceType = FULL3D;
  m_savedialog_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  setFocusPolicy(Qt::StrongFocus);
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  QSplitter* controlPanelLayout = new QSplitter(Qt::Horizontal);

  //Add Tab control panel
  mControlsTab = new QTabWidget(this,0);
  controlPanelLayout->addWidget(mControlsTab);
  controlPanelLayout->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  // Create the display widget
  m_InstrumentDisplay = new MantidGLWidget(this);
  m_InstrumentDisplay->installEventFilter(this);
  connect(this,SIGNAL(enableLighting(bool)),m_InstrumentDisplay,SLOT(enableLighting(bool)));

  // Create simple display widget
  m_simpleDisplay = new SimpleWidget(this);
  m_simpleDisplay->installEventFilter(this);

  QWidget* aWidget = new QWidget(this);
  m_instrumentDisplayLayout = new QStackedLayout(aWidget);
  m_instrumentDisplayLayout->addWidget(m_InstrumentDisplay);
  m_instrumentDisplayLayout->addWidget(m_simpleDisplay);

  controlPanelLayout->addWidget(aWidget);

  mainLayout->addWidget(controlPanelLayout);

  m_xIntegration = new XIntegrationControl(this);
  mainLayout->addWidget(m_xIntegration);
  connect(m_xIntegration,SIGNAL(changed(double,double)),this,SLOT(setIntegrationRange(double,double)));

  //Set the mouse/keyboard operation info and help button
  QHBoxLayout* infoLayout = new QHBoxLayout();
  mInteractionInfo = new QLabel();
  infoLayout->addWidget(mInteractionInfo);
  QPushButton* helpButton = new QPushButton("?");
  helpButton->setMaximumWidth(25);
  connect(helpButton,SIGNAL(clicked()),this,SLOT(helpClicked()));
  infoLayout->addWidget(helpButton);
  infoLayout->setStretchFactor(mInteractionInfo,1);
  infoLayout->setStretchFactor(helpButton,0);
  mainLayout->addLayout(infoLayout);

  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  
  // Background colour
  setBackgroundColor(settings.value("BackgroundColor",QColor(0,0,0,1.0)).value<QColor>());

  // Create the b=tabs
  createTabs(settings);
  
  settings.endGroup();

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

  m_ExtractDetsToWorkspaceAction = new QAction("Extract to new workspace",this);
  connect(m_ExtractDetsToWorkspaceAction,SIGNAL(activated()),this,SLOT(extractDetsToWorkspace()));

  m_SumDetsToWorkspaceAction = new QAction("Sum to new workspace",this);
  connect(m_SumDetsToWorkspaceAction,SIGNAL(activated()),this,SLOT(sumDetsToWorkspace()));

  m_createIncludeGroupingFileAction = new QAction("Include",this);
  connect(m_createIncludeGroupingFileAction,SIGNAL(activated()),this,SLOT(createIncludeGroupingFile()));

  m_createExcludeGroupingFileAction = new QAction("Exclude",this);
  connect(m_createExcludeGroupingFileAction,SIGNAL(activated()),this,SLOT(createExcludeGroupingFile()));

  m_clearPeakOverlays = new QAction("Clear peaks",this);
  connect(m_clearPeakOverlays,SIGNAL(activated()),this,SLOT(clearPeakOverlays()));

  confirmClose(app->confirmCloseInstrWindow);

  setAttribute(Qt::WA_DeleteOnClose);

  // Watch for the deletion of the associated workspace
  observePreDelete();
  observeAfterReplace();
  observeADSClear();

  connect(app->mantidUI->getAlgMonitor(),SIGNAL(algorithmStarted(void*)),this,SLOT(block()));
  connect(app->mantidUI->getAlgMonitor(),SIGNAL(allAlgorithmsStopped()),this,SLOT(unblock()));

  const int windowWidth = 800;
  const int tabsSize = windowWidth / 4;
  QList<int> sizes;
  sizes << tabsSize << windowWidth - tabsSize;
  controlPanelLayout->setSizes(sizes);
  controlPanelLayout->setStretchFactor(0,0);
  controlPanelLayout->setStretchFactor(1,1);
  
  resize(windowWidth,650);

  tabChanged(0);

  connect(this,SIGNAL(needSetIntegrationRange(double,double)),this,SLOT(setIntegrationRange(double,double)),Qt::QueuedConnection);
  setAcceptDrops(true);

  setWindowTitle(QString("Instrument - ") + m_workspaceName);
}

/**
 * Destructor
 */
InstrumentWindow::~InstrumentWindow()
{
  if (m_instrumentActor)
  {
    saveSettings();
    delete m_instrumentActor;
  }
}

/**
 * Init the geometry and colour map outside constructor to prevent creating a broken MdiSubwindow.
 * Must be called straight after constructor.
 * @param resetGeometry :: Set true for resetting the view's geometry: the bounding box and rotation. Default is true.
 * @param autoscaling :: True to start with autoscaling option on.
 * @param scaleMin :: Minimum value of the colormap scale. Ignored if autoscaling == true.
 * @param scaleMax :: Maximum value of the colormap scale. Ignored if autoscaling == true.
 * @param setDefaultView :: Set the default surface type
 */
void InstrumentWindow::init(bool resetGeometry, bool autoscaling, double scaleMin, double scaleMax, bool setDefaultView)
{
  // Previously in (now removed) setWorkspaceName method
  m_instrumentActor = new InstrumentActor(m_workspaceName, autoscaling, scaleMin, scaleMax);
  m_xIntegration->setTotalRange(m_instrumentActor->minBinValue(),m_instrumentActor->maxBinValue());
  m_xIntegration->setUnits(QString::fromStdString(m_instrumentActor->getWorkspace()->getAxis(0)->unit()->caption()));
  auto surface = getSurface();
  if ( resetGeometry || !surface )
  {
    if ( setDefaultView )
    {
      // set the view type to the instrument's default view
      QString defaultView = QString::fromStdString(m_instrumentActor->getInstrument()->getDefaultView());
      if ( defaultView == "3D" && Mantid::Kernel::ConfigService::Instance().getString("MantidOptions.InstrumentView.UseOpenGL") != "On" )
      {
        // if OpenGL is switched off don't open the 3D view at start up
        defaultView = "CYLINDRICAL_Y";
      }
      setSurfaceType( defaultView );
    }
    else
    {
      setSurfaceType(m_surfaceType); // This call must come after the InstrumentActor is created
    }
    setupColorMap();
  }
  else
  {
    surface->resetInstrumentActor( m_instrumentActor );
    updateInfoText();
  }

}

/**
 * Select the tab to be displayed
 */
void InstrumentWindow::selectTab(int tab)
{
  mControlsTab->setCurrentIndex(tab);
}

/**
 * Return the currently displayed tab.
 */
InstrumentWindowTab *InstrumentWindow::getTab()const
{
    return static_cast<InstrumentWindowTab*>(mControlsTab->currentWidget());
}

/**
  * Update the info text displayed at the bottom of the window.
  */
void InstrumentWindow::updateInfoText()
{
    setInfoText( getSurfaceInfoText() );
}

void InstrumentWindow::setSurfaceType(int type)
{
  if (type < RENDERMODE_SIZE)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    m_surfaceType = SurfaceType(type);
    if (!m_instrumentActor) return;
    Mantid::Geometry::Instrument_const_sptr instr = m_instrumentActor->getInstrument();
    Mantid::Geometry::IObjComponent_const_sptr sample = instr->getSample();
    Mantid::Kernel::V3D sample_pos = sample->getPos();
    Mantid::Kernel::V3D axis;
    if (m_surfaceType == SPHERICAL_Y || m_surfaceType == CYLINDRICAL_Y)
    {
      axis = Mantid::Kernel::V3D(0,1,0);
    }
    else if (m_surfaceType == SPHERICAL_Z || m_surfaceType == CYLINDRICAL_Z)
    {
      axis = Mantid::Kernel::V3D(0,0,1);
    }
    else // SPHERICAL_X || CYLINDRICAL_X
    {
      axis = Mantid::Kernel::V3D(1,0,0);
    }

    ProjectionSurface* surface = getSurface().get();
    int peakLabelPrecision = 6;
    bool showPeakRow = true;
    if ( surface )
    {
      peakLabelPrecision = surface->getPeakLabelPrecision();
      showPeakRow = surface->getShowPeakRowFlag();
    }
    else
    {
      QSettings settings;
      peakLabelPrecision = settings.value("Mantid/InstrumentWindow/PeakLabelPrecision",6).toInt();
      showPeakRow = settings.value("Mantid/InstrumentWindow/ShowPeakRows",true).toBool();
    }

    // which display to use?
    bool useOpenGL = isGLEnabled();
    if (m_surfaceType == FULL3D)
    {
      Projection3D* p3d = new Projection3D(m_instrumentActor,getInstrumentDisplayWidth(),getInstrumentDisplayHeight());
      surface = p3d;
      // always OpenGL in 3D
      useOpenGL = true;
    }
    else if (m_surfaceType <= CYLINDRICAL_Z)
    {
      surface = new UnwrappedCylinder(m_instrumentActor,sample_pos,axis);
    }
    else // SPHERICAL
    {
      surface = new UnwrappedSphere(m_instrumentActor,sample_pos,axis);
    }
    surface->setPeakLabelPrecision(peakLabelPrecision);
    surface->setShowPeakRowFlag(showPeakRow);
    // set new surface
    setSurface(surface);
    // make sure to switch to the right instrument display
    selectOpenGLDisplay( useOpenGL );

    // init tabs with new surface
    foreach (InstrumentWindowTab* tab, m_tabs)
    {
        tab->initSurface();
    }

    connect(surface,SIGNAL(multipleDetectorsSelected(QList<int>&)),this,SLOT(multipleDetectorsSelected(QList<int>&)));
    connect(surface,SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),this,SIGNAL(execMantidAlgorithm(Mantid::API::IAlgorithm_sptr)));
    connect(surface,SIGNAL(updateInfoText()),this,SLOT(updateInfoText()),Qt::QueuedConnection);
    QApplication::restoreOverrideCursor();
  }
  updateInfoText();
  update();
}

/**
 * Set the surface type from a string.
 * @param typeStr :: Symbolic name of the surface type: same as the names in SurfaceType enum. Caseless.
 */
void InstrumentWindow::setSurfaceType(const QString& typeStr)
{
  int typeIndex = 0;
  QString upperCaseStr = typeStr.toUpper();
  if ( upperCaseStr == "FULL3D" || upperCaseStr == "3D" )
  {
    typeIndex = 0;
  }
  else if ( upperCaseStr == "CYLINDRICAL_X" )
  {
    typeIndex = 1;
  }
  else if ( upperCaseStr == "CYLINDRICAL_Y" )
  {
    typeIndex = 2;
  }
  else if ( upperCaseStr == "CYLINDRICAL_Z" )
  {
    typeIndex = 3;
  }
  else if ( upperCaseStr == "SPHERICAL_X" )
  {
    typeIndex = 4;
  }
  else if ( upperCaseStr == "SPHERICAL_Y" )
  {
    typeIndex = 5;
  }
  else if ( upperCaseStr == "SPHERICAL_Z" )
  {
    typeIndex = 6;
  }
  setSurfaceType( typeIndex );
  emit surfaceTypeChanged( typeIndex );
}

/**
 * Update the colormap on the render tab.
 */
void InstrumentWindow::setupColorMap()
{
    emit colorMapChanged();
}

/**
  * Connected to QTabWidget::currentChanged signal
  */
void InstrumentWindow::tabChanged(int)
{
    updateInfoText();
}

/**
 * Change color map button slot. This provides the file dialog box to select colormap or sets it directly a string is provided
 */
void InstrumentWindow::changeColormap(const QString &filename)
{
  if (!m_instrumentActor) return;
  QString fileselection;
  //Use a file dialog if no parameter is passed
  if( filename.isEmpty() )
  {
    fileselection = MantidColorMap::loadMapDialog(m_instrumentActor->getCurrentColorMap(), this);
    if( fileselection.isEmpty() ) return;
  }
  else
  {
    fileselection = QFileInfo(filename).absoluteFilePath();
    if( !QFileInfo(fileselection).exists() ) return;
  }
  
  if( !m_instrumentActor->getCurrentColorMap().isEmpty() && (fileselection == m_instrumentActor->getCurrentColorMap())) return;

  m_instrumentActor->loadColorMap(fileselection);
  if( this->isVisible() )
  {
    setupColorMap();
    updateInstrumentView();
  }
}

void InstrumentWindow::showPickOptions()
{
  if (/*m_pickTab->canUpdateTouchedDetector() &&*/ !m_selectedDetectors.empty())
  {
    QMenu context(m_InstrumentDisplay);

    context.addAction(mInfoAction);
    context.addAction(mPlotAction);
    context.addAction(mDetTableAction);

    context.insertSeparator();
    context.addAction(mGroupDetsAction);
    context.addAction(mMaskDetsAction);
    context.addAction(m_ExtractDetsToWorkspaceAction);
    context.addAction(m_SumDetsToWorkspaceAction);
    QMenu *gfileMenu = context.addMenu("Create grouping file");
    gfileMenu->addAction(m_createIncludeGroupingFileAction);
    gfileMenu->addAction(m_createExcludeGroupingFileAction);

    context.exec(QCursor::pos());
  }
}

/**
 * This is slot for the dialog to appear when a detector is picked and the info menu is selected
 */
void InstrumentWindow::spectraInfoDialog()
{
  QString info;
  const int ndets = static_cast<int>(m_selectedDetectors.size());
  if( ndets == 1 )
  {
    QString wsIndex;
    try {
      wsIndex = QString::number(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors.front()));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      // Detector doesn't have a workspace index relating to it
      wsIndex = "None";
    }
    info = QString("Workspace index: %1\nDetector ID: %2").arg(wsIndex,
                                               QString::number(m_selectedDetectors.front()));
  }
  else
  {
    std::vector<size_t> wksp_indices;
    for(int i = 0; i < m_selectedDetectors.size(); ++i)
    {
      try {
        wksp_indices.push_back(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors[i]));
      } catch (Mantid::Kernel::Exception::NotFoundError &) {
        continue; // Detector doesn't have a workspace index relating to it
      }
    }
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
  if (m_selectedDetectors.empty()) return;
  std::set<int> indices;
  for(int i = 0; i < m_selectedDetectors.size(); ++i)
  {
    try {
      indices.insert(int(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors[i])));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      continue; // Detector doesn't have a workspace index relating to it
    }
  }
  emit plotSpectra(m_workspaceName, indices);
}

/**
 * Show detector table
 */
void InstrumentWindow::showDetectorTable()
{
  if (m_selectedDetectors.empty()) return;
  std::vector<int> indexes;
  for(int i = 0; i < m_selectedDetectors.size(); ++i)
  {
    try {
      indexes.push_back(int(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors[i])));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      continue; // Detector doesn't have a workspace index relating to it
    }
  }
  emit createDetectorTable(m_workspaceName, indexes, true);
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
  if (m_selectedDetectors.empty()) return;
  std::vector<int> wksp_indices;
  for(int i = 0; i < m_selectedDetectors.size(); ++i)
  {
    try {
      wksp_indices.push_back(int(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors[i])));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      continue; // Detector doesn't have a workspace index relating to it
    }
  }

  QString inputWS = m_workspaceName;
  QString outputWS = confirmDetectorOperation("grouped", inputWS, static_cast<int>(m_selectedDetectors.size()));
  if( outputWS.isEmpty() ) return;
  QString param_list = "InputWorkspace=%1;OutputWorkspace=%2;WorkspaceIndexList=%3;KeepUngroupedSpectra=1";
  emit execMantidAlgorithm("GroupDetectors",
			   param_list.arg(inputWS, outputWS, asString(wksp_indices)),
         this
			   );
}

/**
 * Mask selected detectors
 */
void InstrumentWindow::maskDetectors()
{
  if (m_selectedDetectors.empty()) return;
  std::vector<int> wksp_indices;
  for(int i = 0; i < m_selectedDetectors.size(); ++i)
  {
    try {
      wksp_indices.push_back(int(m_instrumentActor->getWorkspaceIndex(m_selectedDetectors[i])));
    } catch (Mantid::Kernel::Exception::NotFoundError &) {
      continue; // Detector doesn't have a workspace index relating to it
    }
  }

  QString inputWS = m_workspaceName;
  // Masking can only replace the input workspace so no need to ask for confirmation
  QString param_list = "Workspace=%1;WorkspaceIndexList=%2";
  emit execMantidAlgorithm("MaskDetectors",param_list.arg(inputWS, asString(wksp_indices)),this);
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

/// Set a maximum and minimum for the colour map range
void InstrumentWindow::setColorMapRange(double minValue, double maxValue)
{
  emit colorMapRangeChanged(minValue, maxValue);
  update();
}

/// Set the minimum value of the colour map
void InstrumentWindow::setColorMapMinValue(double minValue)
{
  emit colorMapMinValueChanged(minValue);
  update();
}

/// Set the maximumu value of the colour map
void InstrumentWindow::setColorMapMaxValue(double maxValue)
{
  emit colorMapMaxValueChanged(maxValue);
  update();
}

/**
 * This is the callback for the combo box that selects the view direction
 */
void InstrumentWindow::setViewDirection(const QString& input)
{
  auto p3d = boost::dynamic_pointer_cast<Projection3D>( getSurface() );
  if (p3d)
  {
    p3d->setViewDirection(input);
  }
  updateInstrumentView();
  repaint();
}

/** For the scripting API. Selects a component in the tree and zooms to it.
 *  @param name The name of the component
 *  @throw std::invalid_argument If the component name given does not exist in the tree
 */
void InstrumentWindow::selectComponent(const QString & name)
{
    emit requestSelectComponent(name);
}

/**
 * Set the scale type programmatically
 * @param type :: The scale choice
 */
void InstrumentWindow::setScaleType(GraphOptions::ScaleType type)
{
    emit scaleTypeChanged(type);
}

/**
 * This method opens a color dialog to pick the background color,
 * and then sets it.
 */
void InstrumentWindow::pickBackgroundColor()
{
	QColor color = QColorDialog::getColor(Qt::green,this);
	setBackgroundColor(color);
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
  QString filename = MantidQt::API::FileDialogHandler::getSaveFileName(this, "Save image ...", m_savedialog_dir, filter, &selectedFilter);

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
  
  if ( m_InstrumentDisplay )
    m_InstrumentDisplay->saveToFile(filename);
}

/**
 * Use the file dialog to select a filename to save grouping.
 */
QString InstrumentWindow::getSaveGroupingFilename()
{
  QString filename = MantidQt::API::FileDialogHandler::getSaveFileName(this, "Save grouping file", m_savedialog_dir, "Grouping (*.xml);;All files (*.*)");

  // If its empty, they cancelled the dialog
  if( !filename.isEmpty() )
  {
    //Save the directory used
    QFileInfo finfo(filename);
    m_savedialog_dir = finfo.dir().path();
  }

  return filename;
}

///**
// * Update the text display that informs the user of the current mode and details about it
// */
void InstrumentWindow::setInfoText(const QString& text)
{
  mInteractionInfo->setText(text);
}

/**
 * Save properties of the window a persistent store
 */
void InstrumentWindow::saveSettings()
{
  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  if ( m_InstrumentDisplay )
    settings.setValue("BackgroundColor", m_InstrumentDisplay->currentBackgroundColor());
  settings.setValue("PeakLabelPrecision",getSurface()->getPeakLabelPrecision());
  settings.setValue("ShowPeakRows",getSurface()->getShowPeakRowFlag());
  foreach(InstrumentWindowTab* tab, m_tabs)
  {
      tab->saveSettings(settings);
  }
  settings.endGroup();
}

/** 
 * Closes the window if the associated workspace is deleted.
 * @param ws_name :: Name of the deleted workspace.
 * @param workspace_ptr :: Pointer to the workspace to be deleted
 */
void InstrumentWindow::preDeleteHandle(const std::string & ws_name, const boost::shared_ptr<Workspace> workspace_ptr)
{
  if (ws_name == m_workspaceName.toStdString())
  {
    confirmClose(false);
    close();
    return;
  }
  Mantid::API::IPeaksWorkspace_sptr pws = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(workspace_ptr);
  if (pws)
  {
    getSurface()->peaksWorkspaceDeleted(pws);
    updateInstrumentView();
    return;
  }
}

void InstrumentWindow::afterReplaceHandle(const std::string& wsName,
            const boost::shared_ptr<Workspace> workspace)
{
  //Replace current workspace
  if (wsName == m_workspaceName.toStdString())
  {
    bool resetGeometry = true;
    bool autoscaling = true;
    double scaleMin = 0.0;
    double scaleMax = 0.0;
    if (m_instrumentActor)
    {
      // try to detect if the instrument changes with the workspace
      auto matrixWS = boost::dynamic_pointer_cast<const MatrixWorkspace>( workspace );
      resetGeometry = matrixWS->getInstrument()->getNumberDetectors() != m_instrumentActor->ndetectors();

      // if instrument doesn't change keep the scaling
      if ( !resetGeometry )
      {
        autoscaling = m_instrumentActor->autoscaling();
        scaleMin = m_instrumentActor->minValue();
        scaleMax = m_instrumentActor->maxValue();
      }

      delete m_instrumentActor;
      m_instrumentActor = NULL;
    }

    init( resetGeometry, autoscaling, scaleMin, scaleMax, false );
    updateInstrumentDetectors();
  }
}

void InstrumentWindow::clearADSHandle()
{
  confirmClose(false);
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
	s+="WorkspaceName\t"+m_workspaceName+"\n";
	s+=geometry;
	s+="</instrumentwindow>\n";
	return s;

}

/** 
 * Called just before a show event
 */
void InstrumentWindow::showEvent(QShowEvent* e)
{
  MdiSubWindow::showEvent(e);
  //updateWindow();
}

void InstrumentWindow::block()
{
  m_blocked = true;
}

void InstrumentWindow::unblock()
{
    m_blocked = false;
}

void InstrumentWindow::helpClicked()
{
    QDesktopServices::openUrl(QUrl("http://www.mantidproject.org/MantidPlot:_Instrument_View"));
}

void InstrumentWindow::set3DAxesState(bool on)
{
  auto p3d = boost::dynamic_pointer_cast<Projection3D>( getSurface() );
  if (p3d)
  {
    p3d->set3DAxesState(on);
    updateInstrumentView();
  }
}

void InstrumentWindow::finishHandle(const Mantid::API::IAlgorithm* alg)
{
  UNUSED_ARG(alg);
  emit needSetIntegrationRange(m_instrumentActor->minBinValue(),m_instrumentActor->maxBinValue());
  //m_instrumentActor->update();
  //m_InstrumentDisplay->refreshView();
}

void InstrumentWindow::changeScaleType(int type)
{
  m_instrumentActor->changeScaleType(type);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWindow::changeColorMapMinValue(double minValue)
{
  m_instrumentActor->setAutoscaling(false);
  m_instrumentActor->setMinValue(minValue);
  setupColorMap();
  updateInstrumentView();
}

/// Set the maximumu value of the colour map
void InstrumentWindow::changeColorMapMaxValue(double maxValue)
{
  m_instrumentActor->setAutoscaling(false);
  m_instrumentActor->setMaxValue(maxValue);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWindow::changeColorMapRange(double minValue, double maxValue)
{
  m_instrumentActor->setMinMaxRange(minValue,maxValue);
  setupColorMap();
  updateInstrumentView();
}

void InstrumentWindow::setWireframe(bool on)
{
  auto p3d = boost::dynamic_pointer_cast<Projection3D>( getSurface() );
  if (p3d)
  {
    p3d->setWireframe(on);
  }
  updateInstrumentView();
}

/**
 * Set new integration range but don't update XIntegrationControl (because the control calls this slot)
 */
void InstrumentWindow::setIntegrationRange(double xmin,double xmax)
{
  m_instrumentActor->setIntegrationRange(xmin,xmax);
  setupColorMap();
  updateInstrumentDetectors();
  emit integrationRangeChanged(xmin,xmax);
}

/**
 * Set new integration range and update XIntegrationControl. To be called from python.
 */
void InstrumentWindow::setBinRange(double xmin,double xmax)
{
  m_xIntegration->setRange(xmin,xmax);
}

void InstrumentWindow::multipleDetectorsSelected(QList<int>& detlist)
{
  m_selectedDetectors = detlist;
  showPickOptions();
}

/**
  * Update the display to view a selected component. The selected component
  * is visible the rest of the instrument is hidden.
  * @param id :: The component id.
  */
void InstrumentWindow::componentSelected(ComponentID id)
{
    auto surface = getSurface();
    if (surface)
    {
      surface->componentSelected(id);
      surface->updateView();
      updateInstrumentView();
    }
}

/** A class for creating grouping xml files
  */
class DetXMLFile
{
public:
  enum Option {List,Sum};
  /// Create a grouping file to extract all detectors in detector_list excluding those in dets
  DetXMLFile(const std::string& instrName, const std::vector<int>& detector_list, const QList<int>& dets, const QString& fname)
  {
    m_instrName = instrName;
    m_fileName = fname;
    m_delete = false;
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping instrument=\"" << m_instrName << "\"> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    std::vector<int>::const_iterator idet = detector_list.begin();
    bool first = true;
    for(; idet != detector_list.end(); ++idet)
    {
      if (!dets.contains(*idet))
      {
        if ( !first ) out << ',';
        out <<  *idet ;
        first = false;
      }
    }
    out << "\"/> </group> \n</detector-grouping>\n";
  }

  /// Create a grouping file to extract detectors in dets. Option List - one group - one detector,
  /// Option Sum - one group which is a sum of the detectors
  /// If fname is empty create a temporary file
  DetXMLFile(const std::string& instrName, const QList<int>& dets, Option opt = List, const QString& fname = "")
  {
    if (dets.empty())
    {
      m_fileName = "";
      return;
    }

    m_instrName = instrName;
    if (fname.isEmpty())
    {
      QTemporaryFile mapFile;
      mapFile.open();
      m_fileName = mapFile.fileName() + ".xml";
      mapFile.close();
      m_delete = true;
    }
    else
    {
      m_fileName = fname;
      m_delete = false;
    }

    switch(opt)
    {
    case Sum: makeSumFile(dets); break;
    case List: makeListFile(dets); break;
    }

  }

  /// Make grouping file where each detector is put into its own group
  void makeListFile(const QList<int>& dets)
  {
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping instrument=\"" << m_instrName << "\"> \n";
    foreach(int det,dets)
    {
      out << "<group name=\"" << det << "\"> <detids val=\"" << det << "\"/> </group> \n";
    }
    out << "</detector-grouping>\n";
  }

  /// Make grouping file for putting the detectors into one group (summing the detectors)
  void makeSumFile(const QList<int>& dets)
  {
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping instrument=\"" << m_instrName << "\"> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    int first_det = dets[0];
    foreach(int det,dets)
    {
      if ( det != first_det ) out << ',';
      out << det;
    }
    out << "\"/> </group> \n</detector-grouping>\n";
  }

  ~DetXMLFile()
  {
    if (m_delete)
    {
      QDir dir;
      dir.remove(m_fileName);
    }
  }

  /// Return the name of the created grouping file
  const std::string operator()()const{return m_fileName.toStdString();}

private:
  std::string m_instrName; ///< the instrument name
  QString m_fileName;  ///< holds the grouping file name
  bool m_delete;       ///< if true delete the file on destruction
};

/**
  * Extract selected detectors to a new workspace
  */
void InstrumentWindow::extractDetsToWorkspace()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  DetXMLFile mapFile(m_instrumentActor->getInstrument()->getName(), m_selectedDetectors);
  std::string fname = mapFile();

  if (!fname.empty())
  {
    Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
    alg->setPropertyValue("InputWorkspace",m_workspaceName.toStdString());
    alg->setPropertyValue("MapFile",fname);
    alg->setPropertyValue("OutputWorkspace",m_workspaceName.toStdString()+"_selection");
    alg->execute();
  }

  QApplication::restoreOverrideCursor();
}

/**
  * Sum selected detectors to a new workspace
  */
void InstrumentWindow::sumDetsToWorkspace()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  DetXMLFile mapFile(m_instrumentActor->getInstrument()->getName(), m_selectedDetectors,DetXMLFile::Sum);
  std::string fname = mapFile();

  if (!fname.empty())
  {
    Mantid::API::IAlgorithm* alg = Mantid::API::FrameworkManager::Instance().createAlgorithm("GroupDetectors");
    alg->setPropertyValue("InputWorkspace",m_workspaceName.toStdString());
    alg->setPropertyValue("MapFile",fname);
    alg->setPropertyValue("OutputWorkspace",m_workspaceName.toStdString()+"_sum");
    alg->execute();
  }

  QApplication::restoreOverrideCursor();
}

void InstrumentWindow::createIncludeGroupingFile()
{
  QString fname = getSaveGroupingFilename();
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(m_instrumentActor->getInstrument()->getName(), m_selectedDetectors,DetXMLFile::Sum,fname);
  }

}

void InstrumentWindow::createExcludeGroupingFile()
{
  QString fname = getSaveGroupingFilename();
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(m_instrumentActor->getInstrument()->getName(), m_instrumentActor->getAllDetIDs(),m_selectedDetectors,fname);
  }
}

void InstrumentWindow::executeAlgorithm(const QString& alg_name, const QString& param_list)
{
    emit execMantidAlgorithm(alg_name,param_list,this);
}

void InstrumentWindow::executeAlgorithm(Mantid::API::IAlgorithm_sptr alg)
{
    emit execMantidAlgorithm( alg );
}

/**
 * Set the type of the view (SurfaceType).
 * @param type :: String code for the type. One of: 
 * FULL3D, CYLINDRICAL_X, CYLINDRICAL_Y, CYLINDRICAL_Z, SPHERICAL_X, SPHERICAL_Y, SPHERICAL_Z
 */
void InstrumentWindow::setViewType(const QString& type)
{
  QString type_upper = type.toUpper();
  SurfaceType itype = FULL3D;
  if (type_upper == "FULL3D")
  {
    itype = FULL3D;
  }
  else if (type_upper == "CYLINDRICAL_X")
  {
    itype = CYLINDRICAL_X;
  }
  else if (type_upper == "CYLINDRICAL_Y")
  {
    itype = CYLINDRICAL_Y;
  }
  else if (type_upper == "CYLINDRICAL_Z")
  {
    itype = CYLINDRICAL_Z;
  }
  else if (type_upper == "SPHERICAL_X")
  {
    itype = SPHERICAL_X;
  }
  else if (type_upper == "SPHERICAL_Y")
  {
    itype = SPHERICAL_Y;
  }
  else if (type_upper == "SPHERICAL_Z")
  {
    itype = SPHERICAL_Z;
  }
  setSurfaceType(itype);
}

void InstrumentWindow::dragEnterEvent( QDragEnterEvent* e )
{
  QString text = e->mimeData()->text();
  if (text.startsWith("Workspace::"))
  {
    e->accept();
  }
  else
  {
    e->ignore();
  }
}

void InstrumentWindow::dropEvent( QDropEvent* e )
{
  QString text = e->mimeData()->text();
  if (text.startsWith("Workspace::"))
  {
    QStringList wsName = text.split("::");
    Mantid::API::IPeaksWorkspace_sptr pws = boost::dynamic_pointer_cast<Mantid::API::IPeaksWorkspace>(
      Mantid::API::AnalysisDataService::Instance().retrieve(wsName[1].toStdString()));
    auto surface = boost::dynamic_pointer_cast<UnwrappedSurface>( getSurface() );
    if (pws && surface)
    {
      surface->setPeaksWorkspace(pws);
      updateInstrumentView();
      e->accept();
      return;
    }
    else if (pws && !surface)
    {
      QMessageBox::warning(this,"MantidPlot - Warning","Please change to an unwrapped view to see peak labels.");
    }
  }
  e->ignore();
}

/**
 * Filter events directed to m_InstrumentDisplay and ContextMenuEvent in particular.
 * @param obj :: Object which events will be filtered.
 * @param ev :: An ingoing event.
 */
bool InstrumentWindow::eventFilter(QObject *obj, QEvent *ev)
{
  if (ev->type() == QEvent::ContextMenu &&
       (dynamic_cast<MantidGLWidget*>(obj) == m_InstrumentDisplay ||
        dynamic_cast<SimpleWidget*>(obj) == m_simpleDisplay) &&
        getSurface() && getSurface()->canShowContextMenu())
  {
    // an ugly way of preventing the curve in the pick tab's miniplot disappearing when
    // cursor enters the context menu
    m_instrumentDisplayContextMenuOn = true;
    QMenu context(this);
    // add tab specific actions
    InstrumentWindowTab *tab = getTab();
    tab->addToDisplayContextMenu(context);
    if ( getSurface()->hasPeakOverlays() )
    {
      context.addSeparator();
      context.addAction(m_clearPeakOverlays);
    }
    if ( !context.isEmpty() )
    {
      context.exec(QCursor::pos());
    }
    m_instrumentDisplayContextMenuOn = false;
    return true;
  }
  return MdiSubWindow::eventFilter(obj,ev);
}

/**
 * Set on / off autoscaling of the color map on the render tab.
 * @param on :: On or Off.
 */
void InstrumentWindow::setColorMapAutoscaling(bool on)
{
  m_instrumentActor->setAutoscaling(on);
  setupColorMap();
  updateInstrumentView();
}

/**
 * Remove all peak overlays from the instrument display.
 */
void InstrumentWindow::clearPeakOverlays()
{
  getSurface()->clearPeakOverlays();
  updateInstrumentView();
}

/**
 * Set the precision (significant digits) with which the HKL peak labels are displayed.
 * @param n :: Precision, > 0
 */
void InstrumentWindow::setPeakLabelPrecision(int n)
{
  getSurface()->setPeakLabelPrecision(n);
  updateInstrumentView();
}

/**
 * Enable or disable the show peak row flag
 */
void InstrumentWindow::setShowPeakRowFlag(bool on)
{
  getSurface()->setShowPeakRowFlag(on);
  updateInstrumentView();
}

/**
 * Set background color of the instrument display
 * @param color :: New background colour.
 */
void InstrumentWindow::setBackgroundColor(const QColor& color)
{
  if ( m_InstrumentDisplay )
    m_InstrumentDisplay->setBackgroundColor(color);
}

/**
 * Get the surface info string
 */
QString InstrumentWindow::getSurfaceInfoText() const
{
  ProjectionSurface* surface = getSurface().get();
  return surface ? surface->getInfoText() : "";
}

/**
 * Get pointer to the projection surface
 */
ProjectionSurface_sptr InstrumentWindow::getSurface() const
{
  if ( m_InstrumentDisplay )
  {
    return m_InstrumentDisplay->getSurface();
  }
  else if ( m_simpleDisplay )
  {
    return m_simpleDisplay->getSurface();
  }
  return ProjectionSurface_sptr();
}

/**
 * Set newly created projection surface
 * @param surface :: Pointer to the new surace.
 */
void InstrumentWindow::setSurface(ProjectionSurface* surface)
{
  ProjectionSurface_sptr sharedSurface( surface );
  if ( m_InstrumentDisplay )
  {
    m_InstrumentDisplay->setSurface(sharedSurface);
    m_InstrumentDisplay->update();
  }
  if ( m_simpleDisplay )
  {
    m_simpleDisplay->setSurface(sharedSurface);
    m_simpleDisplay->update();
  }
}

/// Return the width of the instrunemt display
int InstrumentWindow::getInstrumentDisplayWidth() const
{
  if ( m_InstrumentDisplay )
  {
    return m_InstrumentDisplay->width();
  }
  else if ( m_simpleDisplay )
  {
    return m_simpleDisplay->width();
  }
  return 0;
}

/// Return the height of the instrunemt display
int InstrumentWindow::getInstrumentDisplayHeight() const
{
  if ( m_InstrumentDisplay )
  {
    return m_InstrumentDisplay->height();
  }
  else if ( m_simpleDisplay )
  {
    return m_simpleDisplay->height();
  }
  return 0;
}

/// Redraw the instrument view
/// @param picking :: Set to true to update the picking image regardless the interaction
///   mode of the surface.
void InstrumentWindow::updateInstrumentView(bool picking)
{
  if ( m_InstrumentDisplay && m_instrumentDisplayLayout->currentWidget() == dynamic_cast<QWidget*>(m_InstrumentDisplay) )
  {
    m_InstrumentDisplay->updateView(picking);
  }
  else
  {
    m_simpleDisplay->updateView(picking);
  }
}

/// Recalculate the colours and redraw the instrument view
void InstrumentWindow::updateInstrumentDetectors()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  if ( m_InstrumentDisplay && m_instrumentDisplayLayout->currentWidget() == dynamic_cast<QWidget*>(m_InstrumentDisplay) )
  {
    m_InstrumentDisplay->updateDetectors();
  }
  else
  {
    m_simpleDisplay->updateDetectors();
  }
  QApplication::restoreOverrideCursor();
}

/**
 * Choose which widget to use.
 * @param yes :: True to use the OpenGL one or false to use the Simple
 */
void InstrumentWindow::selectOpenGLDisplay(bool yes)
{
  int widgetIndex = yes ? 0 : 1;
  const int oldIndex = m_instrumentDisplayLayout->currentIndex();
  if ( oldIndex == widgetIndex ) return;
  m_instrumentDisplayLayout->setCurrentIndex( widgetIndex );
  auto surface = getSurface();
  if ( surface )
  {
    surface->updateView();
  }
}

/// Public slot to toggle between the GL and simple instrument display widgets
void InstrumentWindow::enableOpenGL( bool on )
{
    enableGL(on);
    emit glOptionChanged(on);
}

/// Private slot to toggle between the GL and simple instrument display widgets
void InstrumentWindow::enableGL( bool on )
{
  m_useOpenGL = on;
  if ( m_surfaceType == FULL3D )
  {
    // always OpenGL in 3D
    selectOpenGLDisplay( true );
  }
  else
  {
    // select the display
    selectOpenGLDisplay( on );
  }
}

/// True if the GL instrument display is currently on
bool InstrumentWindow::isGLEnabled() const
{
    return m_useOpenGL;
}

/**
  * Create and add the tab widgets.
  */
void InstrumentWindow::createTabs(QSettings& settings)
{
    //Render Controls
    InstrumentWindowRenderTab *renderTab = new InstrumentWindowRenderTab(this);
    connect(renderTab,SIGNAL(setAutoscaling(bool)),this,SLOT(setColorMapAutoscaling(bool)));
    connect(renderTab,SIGNAL(rescaleColorMap()),this,SLOT(setupColorMap()));
    mControlsTab->addTab( renderTab, QString("Render"));
    renderTab->loadSettings(settings);

    // Pick controls
    InstrumentWindowPickTab *pickTab = new InstrumentWindowPickTab(this);
    mControlsTab->addTab( pickTab, QString("Pick"));
    pickTab->loadSettings(settings);

    // Mask controls
    InstrumentWindowMaskTab *maskTab = new InstrumentWindowMaskTab(this);
    mControlsTab->addTab( maskTab, QString("Mask"));
    connect(maskTab,SIGNAL(executeAlgorithm(const QString&, const QString&)),this,SLOT(executeAlgorithm(const QString&, const QString&)));
    maskTab->loadSettings(settings);

    // Instrument tree controls
    InstrumentWindowTreeTab *treeTab = new InstrumentWindowTreeTab(this);
    mControlsTab->addTab( treeTab, QString("Instrument Tree"));
    treeTab->loadSettings(settings);

    connect(mControlsTab,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

    m_tabs << renderTab << pickTab << maskTab << treeTab;

}
