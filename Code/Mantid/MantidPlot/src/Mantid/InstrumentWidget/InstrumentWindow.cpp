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
#include "PanelsSurface.h"
#include "SimpleWidget.h"
#include "DetXMLFile.h"
#include "../MantidUI.h"
#include "../AlgorithmMonitor.h"
#include "TSVSerialiser.h"

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
#include <QUrl>
#include <QTemporaryFile>

#include "MantidQtAPI/FileDialogHandler.h"

#include <numeric>
#include <stdexcept>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::API;

// Name of the QSettings group to store the InstrumentWindw settings
const char* InstrumentWindowSettingsGroup = "Mantid/InstrumentWindow";

namespace {
  /**
   * Exception type thrown when an istrument has no sample and cannot be displayed in the instrument view.
   */
  class InstrumentHasNoSampleError: public std::runtime_error
  {
  public:
    InstrumentHasNoSampleError():std::runtime_error("Instrument has no sample.\nSource and sample need to be set in the IDF."){}
  };

}

/**
 * Constructor.
 */
InstrumentWindow::InstrumentWindow(const QString& wsName, const QString& label, ApplicationWindow *app , const QString& name , Qt::WFlags f ):
  MdiSubWindow(app, label, name, f), WorkspaceObserver(),
  m_InstrumentDisplay(NULL),
  m_simpleDisplay(NULL),
  m_workspaceName(wsName),
  m_instrumentActor(NULL),
  m_surfaceType(FULL3D),
  m_savedialog_dir(QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"))),
  mViewChanged(false), 
  m_blocked(false),
  m_instrumentDisplayContextMenuOn(false)
{
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
  m_clearPeakOverlays = new QAction("Clear peaks",this);
  connect(m_clearPeakOverlays,SIGNAL(activated()),this,SLOT(clearPeakOverlays()));

  confirmClose(app->confirmCloseInstrWindow);

  setAttribute(Qt::WA_DeleteOnClose);

  // Watch for the deletion of the associated workspace
  observePreDelete();
  observeAfterReplace();
  observeRename();
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
 * Returns the named tab or the current tab if none supplied
 * @param title Optional title of a tab (default="")
 */
InstrumentWindowTab *InstrumentWindow::getTab(const QString & title)const
{
  QWidget *tab(NULL);
  if(title.isEmpty()) tab = mControlsTab->currentWidget();
  else 
  {
    for(int i = 0; i < mControlsTab->count(); ++i)
    {
      if(mControlsTab->tabText(i) == title)
      {
        tab = mControlsTab->widget(i);
        break;
      }
    }
  }

  if(tab) return qobject_cast<InstrumentWindowTab*>(tab);
  else return NULL;
}

/**
 * @param tab An enumeration for the tab to select
 * @returns A pointer to the requested tab
 */
InstrumentWindowTab * InstrumentWindow::getTab(const Tab tab) const
{
  QWidget *widget = mControlsTab->widget(static_cast<int>(tab));
  if(widget) return qobject_cast<InstrumentWindowTab*>(widget);
  else return NULL;
}

/**
  * Opens Qt file dialog to select the filename.
  * The dialog opens in the directory used last for saving or the default user directory.
  *
  * @param title :: The title of the dialog.
  * @param filters :: The filters
  * @param selectedFilter :: The selected filter.
  */
QString InstrumentWindow::getSaveFileName(const QString& title, const QString& filters, QString *selectedFilter)
{
    QString filename = MantidQt::API::FileDialogHandler::getSaveFileName(this, title, m_savedialog_dir, filters, selectedFilter);

    // If its empty, they cancelled the dialog
    if( !filename.isEmpty() )
    {
        //Save the directory used
        QFileInfo finfo(filename);
        m_savedialog_dir = finfo.dir().path();
    }
    return filename;
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
  // we cannot do 3D without OpenGL
  if ( type == FULL3D && !isGLEnabled() )
  {
    QMessageBox::warning(this,"Mantid - Warning","OpenGL must be enabled to render the instrument in 3D.");
    return;
  }

  if (type < RENDERMODE_SIZE)
  {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    SurfaceType surfaceType = SurfaceType(type);
    if (!m_instrumentActor) return;

    ProjectionSurface* surface = getSurface().get();
    int peakLabelPrecision = 6;
    bool showPeakRow = true;
    bool showPeakLabels = true;
    if ( surface )
    {
      peakLabelPrecision = surface->getPeakLabelPrecision();
      showPeakRow = surface->getShowPeakRowsFlag();
      showPeakLabels = surface->getShowPeakLabelsFlag();
    }
    else
    {
      QSettings settings;
      peakLabelPrecision = settings.value("Mantid/InstrumentWindow/PeakLabelPrecision",2).toInt();
      showPeakRow = settings.value("Mantid/InstrumentWindow/ShowPeakRows",true).toBool();
      showPeakLabels = settings.value("Mantid/InstrumentWindow/ShowPeakLabels",true).toBool();
    }


    // Surface factory
    // If anything throws during surface creation, store error message here
    QString errorMessage;
    try
    {
        Mantid::Geometry::Instrument_const_sptr instr = m_instrumentActor->getInstrument();
        Mantid::Geometry::IComponent_const_sptr sample = instr->getSample();
        if ( !sample )
        {
          throw InstrumentHasNoSampleError();
        }
        Mantid::Kernel::V3D sample_pos = sample->getPos();
        Mantid::Kernel::V3D axis;
        // define the axis
        if (surfaceType == SPHERICAL_Y || surfaceType == CYLINDRICAL_Y)
        {
          axis = Mantid::Kernel::V3D(0,1,0);
        }
        else if (surfaceType == SPHERICAL_Z || surfaceType == CYLINDRICAL_Z)
        {
          axis = Mantid::Kernel::V3D(0,0,1);
        }
        else if (surfaceType == SPHERICAL_X || surfaceType == CYLINDRICAL_X)
        {
          axis = Mantid::Kernel::V3D(1,0,0);
        }
        else // SIDE_BY_SIDE
        {
            axis = Mantid::Kernel::V3D(0,0,1);
        }

        // create the surface
        if (surfaceType == FULL3D)
        {
          surface = new Projection3D(m_instrumentActor,getInstrumentDisplayWidth(),getInstrumentDisplayHeight());
        }
        else if (surfaceType <= CYLINDRICAL_Z)
        {
          surface = new UnwrappedCylinder(m_instrumentActor,sample_pos,axis);
        }
        else if (surfaceType <= SPHERICAL_Z)
        {
          surface = new UnwrappedSphere(m_instrumentActor,sample_pos,axis);
        }
        else // SIDE_BY_SIDE
        {
            surface = new PanelsSurface(m_instrumentActor,sample_pos,axis);
        }
    }
    catch(InstrumentHasNoSampleError&)
    {
      QApplication::restoreOverrideCursor();
      throw;
    }
    catch(std::exception &e)
    {
      errorMessage = e.what();
    }
    catch(...)
    {
      errorMessage = "Unknown exception thrown.";
    }
    if ( !errorMessage.isNull() )
    {
      // if exception was thrown roll back to the current surface type.
      QApplication::restoreOverrideCursor();
      QMessageBox::critical(this,"MantidPlot - Error", 
        "Surface cannot be created because of an exception:\n\n  " + 
        errorMessage + 
        "\n\nPlease select a different surface type.");
      // if suface change was initialized by the GUI this should ensure its consistency
      emit surfaceTypeChanged( m_surfaceType );
      return;
    }
    // end Surface factory

    m_surfaceType = surfaceType;
    surface->setPeakLabelPrecision(peakLabelPrecision);
    surface->setShowPeakRowsFlag(showPeakRow);
    surface->setShowPeakLabelsFlag(showPeakLabels);
    // set new surface
    setSurface(surface);

    // init tabs with new surface
    foreach (InstrumentWindowTab* tab, m_tabs)
    {
        tab->initSurface();
    }

    //connect(surface,SIGNAL(multipleDetectorsSelected(QList<int>&)),this,SLOT(multipleDetectorsSelected(QList<int>&)));
    connect(surface,SIGNAL(executeAlgorithm(Mantid::API::IAlgorithm_sptr)),this,SIGNAL(execMantidAlgorithm(Mantid::API::IAlgorithm_sptr)));
    connect(surface,SIGNAL(updateInfoText()),this,SLOT(updateInfoText()),Qt::QueuedConnection);
    QApplication::restoreOverrideCursor();
  }
  emit surfaceTypeChanged( type );
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
  else if ( upperCaseStr == "SIDE_BY_SIDE" )
  {
    typeIndex = 7;
  }
  setSurfaceType( typeIndex );
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

/**
 * Saves the current image buffer as a png file.
 * @param filename Optional filename. Empty string raises a save dialog
 */
void InstrumentWindow::saveImage(QString filename)
{
  QString defaultExt = ".png";
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  if(filename.isEmpty())
  {
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
    QString selectedFilter = "*" + defaultExt;
    filename = getSaveFileName("Save image ...", filter, &selectedFilter);

    // If its empty, they cancelled the dialog
    if( filename.isEmpty() ) return;
  }
  
  QFileInfo finfo(filename);
  QString ext = finfo.completeSuffix();

  if( ext.isEmpty() )
  {
    filename += defaultExt;
    ext = QFileInfo(filename).completeSuffix();
  }
  else
  {
    if( !formats.contains(ext.toAscii()) )
    {
      QString msg("Unsupported file extension. Choose one of the following: ");
      QListIterator<QByteArray> itr(formats);
      while( itr.hasNext() )
      {
        msg += itr.next() + ", ";
      }
      msg.chop(2);// Remove last space and comma
      QMessageBox::warning(this, "MantidPlot", msg);
      return;
    }
  }
  
  if ( isGLEnabled() )
  {
    m_InstrumentDisplay->saveToFile(filename);
  }
  else
  {
    m_simpleDisplay->saveToFile(filename);
  }
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
  auto surface = getSurface();
  if ( surface )
  {
    // if surface is null istrument view wasn't created and there is nothing to save
    settings.setValue("PeakLabelPrecision",getSurface()->getPeakLabelPrecision());
    settings.setValue("ShowPeakRows",getSurface()->getShowPeakRowsFlag());
    settings.setValue("ShowPeakLabels",getSurface()->getShowPeakLabelsFlag());
    foreach(InstrumentWindowTab* tab, m_tabs)
    {
        tab->saveSettings(settings);
    }
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
    getSurface()->deletePeaksWorkspace(pws);
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
    if (m_instrumentActor)
    {
      // Check if it's still the same workspace underneath (as well as having the same name)
      auto matrixWS = boost::dynamic_pointer_cast<const MatrixWorkspace>( workspace );
      bool sameWS = false;
      try {
        sameWS = ( matrixWS == m_instrumentActor->getWorkspace() );
      } catch (std::runtime_error&) {
        // Carry on, sameWS should stay false
      }

      // try to detect if the instrument changes (unlikely if the workspace hasn't, but theoretically possible)
      bool resetGeometry = matrixWS->getInstrument()->getNumberDetectors() != m_instrumentActor->ndetectors();

      // if workspace and instrument don't change keep the scaling
      if ( sameWS && !resetGeometry )
      {
        m_instrumentActor->updateColors();
      }
      else
      {
        delete m_instrumentActor;
        m_instrumentActor = NULL;
        init( resetGeometry, true, 0.0, 0.0, false );
        updateInstrumentDetectors();
      }
    }

  }
}

void InstrumentWindow::renameHandle(const std::string &oldName, const std::string &newName)
{
  if (oldName == m_workspaceName.toStdString())
  {
    m_workspaceName = QString::fromStdString(newName);
    setWindowTitle(QString("Instrument - ") + m_workspaceName);
  }
}

void InstrumentWindow::clearADSHandle()
{
  confirmClose(false);
  close();
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
      //surface->updateView();
      updateInstrumentView();
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
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace")
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
  QString name = e->mimeData()->objectName();
  if (name == "MantidWorkspace")
  {
    QString text = e->mimeData()->text();
    int endIndex = 0;
    QStringList wsNames;
    while (text.indexOf("[\"",endIndex) > -1)
    {
      int startIndex = text.indexOf("[\"",endIndex) + 2;
      endIndex = text.indexOf("\"]",startIndex);
      wsNames.append(text.mid(startIndex,endIndex-startIndex));
    }

    foreach (const auto& wsName, wsNames)
    {
      if(this->overlay(wsName)) e->accept();  
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
 *  Overlay a workspace with the given name
 * @param wsName The name of a workspace in the ADS
 * @returns True if the overlay was successful, false otherwise
 */
bool InstrumentWindow::overlay(const QString & wsName)
{
  using namespace Mantid::API;
  auto pws = boost::dynamic_pointer_cast<IPeaksWorkspace>(AnalysisDataService::Instance().retrieve(wsName.toStdString()));
  auto surface = boost::dynamic_pointer_cast<UnwrappedSurface>( getSurface() );
  bool success(false);
  if (pws && surface)
  {
    surface->setPeaksWorkspace(pws);
    updateInstrumentView();
    success = true;
  }
  else if (pws && !surface)
  {
    QMessageBox::warning(this,"MantidPlot - Warning","Please change to an unwrapped view to see peak labels.");
  }
  return success;
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
 * @param on :: True to show, false to hide.
 */
void InstrumentWindow::setShowPeakRowFlag(bool on)
{
  getSurface()->setShowPeakRowsFlag(on);
  updateInstrumentView();
}

/**
 * Enable or disable the show peak hkl labels flag
 * @param on :: True to show, false to hide.
 */
void InstrumentWindow::setShowPeakLabelsFlag(bool on)
{
    getSurface()->setShowPeakLabelsFlag(on);
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
  UnwrappedSurface *unwrappedSurface = dynamic_cast<UnwrappedSurface*>( surface );
  if ( unwrappedSurface )
  {
    m_renderTab->flipUnwrappedView(unwrappedSurface->isFlippedView());
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
    selectOpenGLDisplay(isGLEnabled());
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
    m_renderTab = new InstrumentWindowRenderTab(this);
    connect(m_renderTab,SIGNAL(setAutoscaling(bool)),this,SLOT(setColorMapAutoscaling(bool)));
    connect(m_renderTab,SIGNAL(rescaleColorMap()),this,SLOT(setupColorMap()));
    mControlsTab->addTab( m_renderTab, QString("Render"));
    m_renderTab->loadSettings(settings);

    // Pick controls
    InstrumentWindowPickTab *pickTab = new InstrumentWindowPickTab(this);
    mControlsTab->addTab( pickTab, QString("Pick"));
    pickTab->loadSettings(settings);

    // Mask controls
    InstrumentWindowMaskTab *maskTab = new InstrumentWindowMaskTab(this);
    mControlsTab->addTab( maskTab, QString("Mask/Group"));
    connect(maskTab,SIGNAL(executeAlgorithm(const QString&, const QString&)),this,SLOT(executeAlgorithm(const QString&, const QString&)));
    maskTab->loadSettings(settings);

    // Instrument tree controls
    InstrumentWindowTreeTab *treeTab = new InstrumentWindowTreeTab(this);
    mControlsTab->addTab( treeTab, QString("Instrument Tree"));
    treeTab->loadSettings(settings);

    connect(mControlsTab,SIGNAL(currentChanged(int)),this,SLOT(tabChanged(int)));

    m_tabs << m_renderTab << pickTab << maskTab << treeTab;

}

/**
  * Return a name for a group in QSettings to store InstrumentWindow configuration.
  */
QString InstrumentWindow::getSettingsGroupName() const
{
  return QString::fromAscii( InstrumentWindowSettingsGroup );
}

/**
  * Construct a name for a group in QSettings to store instrument-specific configuration.
  */
QString InstrumentWindow::getInstrumentSettingsGroupName() const
{
  return QString::fromAscii( InstrumentWindowSettingsGroup ) + "/" +
      QString::fromStdString( getInstrumentActor()->getInstrument()->getName() );
}

void InstrumentWindow::loadFromProject(const std::string& lines, ApplicationWindow* app, const int fileVersion)
{
  Q_UNUSED(fileVersion);

  TSVSerialiser tsv(lines);
  if(tsv.hasLine("geometry"))
  {
    const QString geometry = QString::fromStdString(tsv.lineAsString("geometry"));
    app->restoreWindowGeometry(app, this, geometry);
  }
}

std::string InstrumentWindow::saveToProject(ApplicationWindow* app)
{
  TSVSerialiser tsv;
  tsv.writeRaw("<instrumentwindow>");
  tsv.writeLine("WorkspaceName") << m_workspaceName.toStdString();
  tsv.writeRaw(app->windowGeometryInfo(this));
  tsv.writeRaw("</instrumentwindow>");
  return tsv.outputLines();
}
