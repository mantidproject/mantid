#include "InstrumentWindow.h"
#include "InstrumentWindowRenderTab.h"
#include "InstrumentWindowPickTab.h"
#include "InstrumentWindowMaskTab.h"
#include "XIntegrationControl.h"
#include "InstrumentActor.h"
#include "UnwrappedCylinder.h"
#include "UnwrappedSphere.h"
#include "Projection3D.h"
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

#include <numeric>
#include <fstream>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::API;

/**
 * A simple widget for drawing unwrapped instrument images.
 */
class SimpleWidget : public QWidget
{
public:
  /// Constructor
  SimpleWidget(QWidget* parent=0):QWidget(parent),m_surface(NULL)
  {
    // Receive mouse move events
    setMouseTracking( true );
  }
  /// Assign a surface to draw on
  void setSurface(ProjectionSurface* surface){m_surface = surface;}
  /// Return the surface 
  ProjectionSurface* getSurface(){return m_surface;}
  /// Refreshes the view
  void refreshView()
  {
    if(m_surface)
    {
      m_surface->updateView();
      update();
    }
  }
protected:
  void paintEvent(QPaintEvent*)
  {
    if(m_surface)
    {
      m_surface->drawSimple(this);
    }
  }
  /**
  * Mouse press callback method, It implements mouse button press initialize methods.
  * @param event :: This is the event variable which has the position and button states
  */
  void mousePressEvent(QMouseEvent* event)
  {
    if (m_surface)
    {
      m_surface->mousePressEvent(event);
    }
    update();
  }

  /**
  * This is mouse move callback method. It implements the actions to be taken when the mouse is
  * moved with a particular button is pressed.
  * @param event :: This is the event variable which has the position and button states
  */
  void mouseMoveEvent(QMouseEvent* event)
  {
    if (m_surface)
    {
      m_surface->mouseMoveEvent(event);
    }
    repaint();
  }

  /**
  * This is mouse button release callback method. This resets the cursor to pointing hand cursor
  * @param event :: This is the event variable which has the position and button states
  */
  void mouseReleaseEvent(QMouseEvent* event)
  {
    if (m_surface)
    {
      m_surface->mouseReleaseEvent(event);
    }
    repaint();
  }

  /**
  * Mouse wheel event to set the zooming in and out
  * @param event :: This is the event variable which has the status of the wheel
  */
  void wheelEvent(QWheelEvent* event)
  {
    if (m_surface)
    {
      m_surface->wheelEvent(event);
    }
    update();
  }
  //-------------------------------------
  //    Class data
  //-------------------------------------
  ProjectionSurface* m_surface; ///< The projection surface
};

/**
 * Constructor.
 */
InstrumentWindow::InstrumentWindow(const QString& wsName, const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ):
  MdiSubWindow(app, label, name, f), WorkspaceObserver(),
  m_workspaceName(wsName),
  m_instrumentActor(NULL),
  mViewChanged(false), 
  m_blocked(false),
  m_instrumentDisplayContextMenuOn(false)
{
  m_surfaceType = FULL3D;
  m_savedialog_dir = QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  setFocusPolicy(Qt::StrongFocus);
  //QWidget *frame = new QWidget();
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  QSplitter* controlPanelLayout = new QSplitter(Qt::Horizontal);

  //Add Tab control panel and Render window
  mControlsTab = new QTabWidget(this,0);
  controlPanelLayout->addWidget(mControlsTab);
  controlPanelLayout->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);

  // Create the display widget
  m_InstrumentDisplay = new MantidGLWidget(this);
  m_InstrumentDisplay->installEventFilter(this);
  connect(m_InstrumentDisplay,SIGNAL(mouseOut()),this,SLOT(mouseLeftInstrumentDisplay()));

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

  //Set the mouse/keyboard operation info
  mInteractionInfo = new QLabel();
  mainLayout->addWidget(mInteractionInfo);

  QSettings settings;
  settings.beginGroup("Mantid/InstrumentWindow");
  
  // Background colour
  setBackgroundColor(settings.value("BackgroundColor",QColor(0,0,0,1.0)).value<QColor>());
  
  //Render Controls
  m_renderTab = new InstrumentWindowRenderTab(this);
  connect(m_renderTab,SIGNAL(setAutoscaling(bool)),this,SLOT(setColorMapAutoscaling(bool)));
  connect(m_renderTab,SIGNAL(rescaleColorMap()),this,SLOT(setupColorMap()));
  mControlsTab->addTab( m_renderTab, QString("Render"));
  
  // Pick controls
  m_pickTab = new InstrumentWindowPickTab(this);
  mControlsTab->addTab( m_pickTab, QString("Pick"));
  // Miniplot tube x units
  m_pickTab->setTubeXUnits(settings.value("TubeXUnits",0).toInt());

  settings.endGroup();

  // Mask controls
  m_maskTab = new InstrumentWindowMaskTab(this);
  mControlsTab->addTab( m_maskTab, QString("Mask"));
  connect(m_maskTab,SIGNAL(executeAlgorithm(const QString&, const QString&)),this,SLOT(executeAlgorithm(const QString&, const QString&)));

  // Instrument tree controls
  QWidget* instrumentTree=createInstrumentTreeTab(mControlsTab);
  mControlsTab->addTab( instrumentTree, QString("Instrument Tree"));

  connect(mControlsTab,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

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

  const int windowWidth = 600;
  const int tabsSize = windowWidth / 3;
  QList<int> sizes;
  sizes << tabsSize << windowWidth - tabsSize;
  controlPanelLayout->setSizes(sizes);
  controlPanelLayout->setStretchFactor(0,0);
  controlPanelLayout->setStretchFactor(1,1);
  
  resize(windowWidth,650);

  tabChanged(0);

  connect(this,SIGNAL(needSetIntegrationRange(double,double)),this,SLOT(setIntegrationRange(double,double)));
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
  delete m_InstrumentDisplay;
}

/**
 * Init the geometry and colour map outside constructor to prevent creating a broken MdiSubwindow.
 * Must be called straight after constructor.
 * @param updateGeometry :: Set true for resetting the view's geometry: the bounding box and rotation. Default is true.
 */
void InstrumentWindow::init(bool resetGeometry)
{
  // Previously in (now removed) setWorkspaceName method
  m_InstrumentDisplay->makeCurrent(); // ?
  m_instrumentActor = new InstrumentActor(m_workspaceName);
  m_xIntegration->setTotalRange(m_instrumentActor->minBinValue(),m_instrumentActor->maxBinValue());
  m_xIntegration->setUnits(QString::fromStdString(m_instrumentActor->getWorkspace()->getAxis(0)->unit()->caption()));
  if ( resetGeometry )
  {
    setSurfaceType(m_surfaceType); // This call must come after the InstrumentActor is created
  }
  setupColorMap();
  mInstrumentTree->setInstrumentActor(m_instrumentActor);
  setInfoText( getSurfaceInfoText() );
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
InstrumentWindow::Tab InstrumentWindow::getTab()const
{
  return (Tab)mControlsTab->currentIndex();
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

    ProjectionSurface* surface = getSurface();
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

    if (m_surfaceType == FULL3D)
    {
      Projection3D* p3d = new Projection3D(m_instrumentActor,getInstrumentDisplayWidth(),getInstrumentDisplayHeight());
      p3d->set3DAxesState(m_renderTab->areAxesOn());
      surface = p3d;
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
    setSurface(surface);
    m_renderTab->init();
    m_pickTab->init();
    m_maskTab->init();

    connect(surface,SIGNAL(singleDetectorTouched(int)),this,SLOT(singleDetectorTouched(int)));
    connect(surface,SIGNAL(singleDetectorPicked(int)),this,SLOT(singleDetectorPicked(int)));
    connect(surface,SIGNAL(multipleDetectorsSelected(QList<int>&)),this,SLOT(multipleDetectorsSelected(QList<int>&)));
    QApplication::restoreOverrideCursor();
  }
  update();
}

/**
 * Update the colormap on the render tab.
 */
void InstrumentWindow::setupColorMap()
{
  m_renderTab->setupColorBar(
    m_instrumentActor->getColorMap(),
    m_instrumentActor->minValue(),
    m_instrumentActor->maxValue(),
    m_instrumentActor->minPositiveValue(),
    m_instrumentActor->autoscaling()
  );
}

/**
  * Connected to QTabWidget::currentChanged signal
  */
void InstrumentWindow::tabChanged(int i)
{
  ProjectionSurface* surface = getSurface();
  QString text;
  if(i != 1) // no picking
  {
    if (surface)
    {
      if (i == 0)
      {
        surface->componentSelected();
        m_instrumentActor->accept(SetAllVisibleVisitor());
      }
    }
  }
  if (surface)
  {
    setInfoText(surface->getInfoText());
    refreshInstrumentDisplay();
  }
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
    refreshInstrumentDisplay();
  }
}

void InstrumentWindow::showPickOptions()
{
  if (m_pickTab->canUpdateTouchedDetector() && !m_selectedDetectors.empty())
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
  m_renderTab->setMinValue(minValue);
  m_renderTab->setMaxValue(maxValue);
  update();
}

/// Set the minimum value of the colour map
void InstrumentWindow::setColorMapMinValue(double minValue)
{
  m_renderTab->setMinValue(minValue);
  update();
}

/// Set the maximumu value of the colour map
void InstrumentWindow::setColorMapMaxValue(double maxValue)
{
  m_renderTab->setMaxValue(maxValue);
  update();
}

/**
 * This is the callback for the combo box that selects the view direction
 */
void InstrumentWindow::setViewDirection(const QString& input)
{
  Projection3D* p3d = dynamic_cast<Projection3D*>( getSurface() );
  if (p3d)
  {
    p3d->setViewDirection(input);
  }
  //mViewChanged = true;
  //m_InstrumentDisplay->repaint();
  refreshInstrumentDisplay();
  repaint();
}

/** For the scripting API. Selects a omponent in the tree and zooms to it.
 *  @param name The name of the component
 *  @throw std::invalid_argument If the component name given does not exist in the tree
 */
void InstrumentWindow::selectComponent(const QString & name)
{
  QModelIndex component = mInstrumentTree->findComponentByName(name);
  if( !component.isValid() )
  {
    throw std::invalid_argument("No component named \'"+name.toStdString()+"\' found");
  }

  mInstrumentTree->clearSelection();
  mInstrumentTree->scrollTo(component, QAbstractItemView::EnsureVisible );
  mInstrumentTree->selectionModel()->select(component, QItemSelectionModel::Select);
  mInstrumentTree->sendComponentSelectedSignal(component);
}

/**
 * Set the scale type programmatically
 * @param type :: The scale choice
 */
void InstrumentWindow::setScaleType(GraphOptions::ScaleType type)
{
  m_renderTab->setScaleType(type);
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
  
  m_InstrumentDisplay->saveToFile(filename);
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
  settings.setValue("BackgroundColor", m_InstrumentDisplay->currentBackgroundColor());
  settings.setValue("TubeXUnits",m_pickTab->getTubeXUnits());
  settings.setValue("PeakLabelPrecision",getSurface()->getPeakLabelPrecision());
  settings.setValue("ShowPeakRows",getSurface()->getShowPeakRowFlag());
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
    m_InstrumentDisplay->repaint();
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
    if (m_instrumentActor)
    {
      auto matrixWS = boost::dynamic_pointer_cast<const MatrixWorkspace>( workspace );
      resetGeometry = matrixWS->getInstrument()->getNumberDetectors() != m_instrumentActor->ndetectors();
      delete m_instrumentActor;
    }
    init( resetGeometry );
    updateWindow();
  }
}

void InstrumentWindow::clearADSHandle()
{
  confirmClose(false);
  close();
}

void InstrumentWindow::updateWindow()
{
  if ( isGLEnabled() )
  {
    m_InstrumentDisplay->refreshView();
  }
  else
  {
    m_simpleDisplay->refreshView();
  }
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



QWidget * InstrumentWindow::createInstrumentTreeTab(QTabWidget* ControlsTab)
{
  QWidget* instrumentTree=new QWidget(ControlsTab);
  QVBoxLayout* instrumentTreeLayout=new QVBoxLayout(instrumentTree);
  //Tree Controls
  mInstrumentTree = new InstrumentTreeWidget(0);
  instrumentTreeLayout->addWidget(mInstrumentTree);
  connect(mInstrumentTree,SIGNAL(componentSelected(Mantid::Geometry::ComponentID)),
          m_InstrumentDisplay,SLOT(componentSelected(Mantid::Geometry::ComponentID)));
  return instrumentTree;
}

void InstrumentWindow::block()
{
  m_blocked = true;
}

void InstrumentWindow::unblock()
{
  m_blocked = false;
}

void InstrumentWindow::set3DAxesState(bool on)
{
  Projection3D* p3d = dynamic_cast<Projection3D*>( getSurface() );
  if (p3d)
  {
    p3d->set3DAxesState(on);
    refreshInstrumentDisplay();
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
  refreshInstrumentDisplay();
}

void InstrumentWindow::changeColorMapMinValue(double minValue)
{
  m_instrumentActor->setAutoscaling(false);
  m_instrumentActor->setMinValue(minValue);
  setupColorMap();
  refreshInstrumentDisplay();
}

/// Set the maximumu value of the colour map
void InstrumentWindow::changeColorMapMaxValue(double maxValue)
{
  m_instrumentActor->setAutoscaling(false);
  m_instrumentActor->setMaxValue(maxValue);
  setupColorMap();
  refreshInstrumentDisplay();
}

void InstrumentWindow::changeColorMapRange(double minValue, double maxValue)
{
  m_instrumentActor->setMinMaxRange(minValue,maxValue);
  setupColorMap();
  refreshInstrumentDisplay();
}

void InstrumentWindow::setWireframe(bool on)
{
  Projection3D* p3d = dynamic_cast<Projection3D*>( getSurface() );
  if (p3d)
  {
    p3d->setWireframe(on);
  }
  refreshInstrumentDisplay();
}

/**
 * Set new integration range but don't update XIntegrationControl (because the control calls this slot)
 */
void InstrumentWindow::setIntegrationRange(double xmin,double xmax)
{
  m_instrumentActor->setIntegrationRange(xmin,xmax);
  setupColorMap();
  refreshInstrumentDisplay();
  if (getTab() == PICK)
  {
    m_pickTab->changedIntegrationRange(xmin,xmax);
  }
}

/**
 * Set new integration range and update XIntegrationControl. To be called from python.
 */
void InstrumentWindow::setBinRange(double xmin,double xmax)
{
  m_xIntegration->setRange(xmin,xmax);
}

void InstrumentWindow::singleDetectorTouched(int detid)
{
  if (m_pickTab->canUpdateTouchedDetector())
  {
    //mInteractionInfo->setText(cursorPos.display());
    m_pickTab->updatePick(detid);
  }
}

void InstrumentWindow::singleDetectorPicked(int detid)
{
  m_pickTab->updatePick(detid);
}

void InstrumentWindow::multipleDetectorsSelected(QList<int>& detlist)
{
  m_selectedDetectors = detlist;
  showPickOptions();
}

/** A class for creating grouping xml files
  */
class DetXMLFile
{
public:
  enum Option {List,Sum};
  /// Create a grouping file to extract all detectors in detector_list excluding those in dets
  DetXMLFile(const std::vector<int>& detector_list, const QList<int>& dets, const QString& fname)
  {
    m_fileName = fname;
    m_delete = false;
    std::ofstream out(m_fileName.toStdString().c_str());
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    std::vector<int>::const_iterator idet = detector_list.begin();
    for(; idet != detector_list.end(); ++idet)
    {
      if (!dets.contains(*idet))
      {
        out <<  *idet << ',';
      }
    }
    out << "\"/> </group> \n</detector-grouping>\n";
  }

  /// Create a grouping file to extract detectors in dets. Option List - one group - one detector,
  /// Option Sum - one group which is a sum of the detectors
  /// If fname is empty create a temporary file
  DetXMLFile(const QList<int>& dets, Option opt = List, const QString& fname = "")
  {
    if (dets.empty())
    {
      m_fileName = "";
      return;
    }

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
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
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
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?> \n<detector-grouping> \n";
    out << "<group name=\"sum\"> <detids val=\"";
    foreach(int det,dets)
    {
      out << det << ',';
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
  QString m_fileName; ///< holds the grouping file name
  bool m_delete;      ///< if true delete the file on destruction
};

/**
  * Extract selected detectors to a new workspace
  */
void InstrumentWindow::extractDetsToWorkspace()
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  DetXMLFile mapFile(m_selectedDetectors);
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
  DetXMLFile mapFile(m_selectedDetectors,DetXMLFile::Sum);
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
  QString fname = QFileDialog::getSaveFileName(this,"Save grouping file");
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(m_selectedDetectors,DetXMLFile::Sum,fname);
  }

}

void InstrumentWindow::createExcludeGroupingFile()
{
  QString fname = QFileDialog::getSaveFileName(this,"Save grouping file");
  if (!fname.isEmpty())
  {
    DetXMLFile mapFile(m_instrumentActor->getAllDetIDs(),m_selectedDetectors,fname);
  }
}

void InstrumentWindow::executeAlgorithm(const QString& alg_name, const QString& param_list)
{
  emit execMantidAlgorithm(alg_name,param_list,this);
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
  m_renderTab->updateSurfaceTypeControl(itype);
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
    UnwrappedSurface* surface = dynamic_cast<UnwrappedSurface*>( getSurface() );
    if (pws && surface)
    {
      surface->setPeaksWorkspace(pws);
      refreshInstrumentDisplay();
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
  if ((dynamic_cast<MantidGLWidget*>(obj) == m_InstrumentDisplay ||
       dynamic_cast<SimpleWidget*>(obj) == m_simpleDisplay) && 
       ev->type() == QEvent::ContextMenu)
  {
    // an ugly way of preventing the curve in the pick tab's miniplot disappearing when 
    // cursor enters the context menu
    m_instrumentDisplayContextMenuOn = true; 
    QMenu context(this);
    // add tab specific actions
    switch(getTab())
    {
    case PICK:  m_pickTab->setInstrumentDisplayContextMenu(context); 
                if ( getSurface()->hasPeakOverlays() )
                {
                  context.addSeparator();
                  context.addAction(m_clearPeakOverlays);
                }
                break;
    default:
      break;
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
  refreshInstrumentDisplay();
}

/**
 * Respond to mouse leaving the instrument display area.
 */
void InstrumentWindow::mouseLeftInstrumentDisplay()
{
  if (getTab() == PICK && !m_instrumentDisplayContextMenuOn)
  {
    // remove the curve from the miniplot
    m_pickTab->mouseLeftInstrmentDisplay();
  }
}

/**
 * Remove all peak overlays from the instrument display.
 */
void InstrumentWindow::clearPeakOverlays()
{
  getSurface()->clearPeakOverlays();
  m_InstrumentDisplay->repaint();
}

/**
 * Set the precision (significant digits) with which the HKL peak labels are displayed.
 * @param n :: Precision, > 0
 */
void InstrumentWindow::setPeakLabelPrecision(int n)
{
  getSurface()->setPeakLabelPrecision(n);
  m_InstrumentDisplay->repaint();
}

/**
 * Enable or disable the show peak row flag
 */
void InstrumentWindow::setShowPeakRowFlag(bool on)
{
  getSurface()->setShowPeakRowFlag(on);
  m_InstrumentDisplay->repaint();
}

/**
 * Set background color of the instrument display
 * @param color :: New background colour.
 */
void InstrumentWindow::setBackgroundColor(const QColor& color)
{
  m_InstrumentDisplay->setBackgroundColor(color);
}

/**
 * Get the surface info string
 */
QString InstrumentWindow::getSurfaceInfoText() const
{
  return m_InstrumentDisplay->getSurface()->getInfoText();
}

/**
 * Get pointer to the projection surface
 */
ProjectionSurface* InstrumentWindow::getSurface() const
{
  return m_InstrumentDisplay->getSurface();
}

/**
 * Set newly created projection surface
 * @param surface :: Pointer to the new surace.
 */
void InstrumentWindow::setSurface(ProjectionSurface* surface)
{
  m_InstrumentDisplay->setSurface(surface);
  m_InstrumentDisplay->update();
  m_simpleDisplay->setSurface(surface);
  m_simpleDisplay->update();
}

/// Return the width of the instrunemt display
int InstrumentWindow::getInstrumentDisplayWidth() const
{
  return m_InstrumentDisplay->width();
}

/// Return the height of the instrunemt display
int InstrumentWindow::getInstrumentDisplayHeight() const
{
  return m_InstrumentDisplay->height();
}

/// Refresh the instrument display
void InstrumentWindow::refreshInstrumentDisplay()
{
  m_InstrumentDisplay->refreshView();
}

/// Toggle between the GL and simple instrument display widgets
void InstrumentWindow::enableGL( bool on )
{
  if ( on )
  {
    m_instrumentDisplayLayout->setCurrentIndex( 0 );
  }
  else
  {
    m_instrumentDisplayLayout->setCurrentIndex( 1 );
  }
  getSurface()->updateView();
}

/// True if the GL instrument display is currently on
bool InstrumentWindow::isGLEnabled() const
{
  return m_instrumentDisplayLayout->currentIndex() == 0;
}
