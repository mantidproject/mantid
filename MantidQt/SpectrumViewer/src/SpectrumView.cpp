#include "MantidQtSpectrumViewer/SpectrumView.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"
#include "MantidQtSpectrumViewer/SVConnections.h"
#include "MantidQtSpectrumViewer/SpectrumDisplay.h"
#include "MantidQtSpectrumViewer/SliderHandler.h"
#include "MantidQtSpectrumViewer/RangeHandler.h"
#include "MantidQtSpectrumViewer/EModeHandler.h"
#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"

#include <boost/make_shared.hpp>

namespace MantidQt
{
namespace SpectrumView
{

/**
 *  Construct an SpectrumView to display data from the specified data source.
 *  The specified SpectrumDataSource must be constructed elsewhere and passed
 *  into this SpectrumView constructor.  Most other components of the SpectrumView
 *  are managed by this class.  That is the graphs, image display and other
 *  parts of the SpectrumView are constructed here and are deleted when the
 *  SpectrumView destructor is called.
 *
 *  @param parent Top-level widget for object.
 */
SpectrumView::SpectrumView(QWidget *parent) :
  QMainWindow(parent, 0),
  WorkspaceObserver(),
  m_ui(new Ui::SpectrumViewer()),
  m_sliderHandler(NULL),
  m_rangeHandler(NULL),
  m_emodeHandler(NULL)
{
  m_ui->setupUi(this);
  //m_ui->x_min_input->setValidator(new QDoubleValidator(this));
  connect(m_ui->imageTabs,SIGNAL(currentChanged(int)),this,SLOT(changeSpectrumDisplay(int)));
  connect(m_ui->imageTabs,SIGNAL(tabCloseRequested(int)),this,SLOT(respondToTabCloseReqest(int)));
  connect(m_ui->tracking_always_on, SIGNAL(toggled(bool)), this, SLOT(changeTracking(bool)));
  updateHandlers();
  setAcceptDrops(true);
  loadSettings();

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observePreDelete();
  observeADSClear();
}


SpectrumView::~SpectrumView()
{
  saveSettings();
  if(m_emodeHandler)
    delete m_emodeHandler;
}


/**
 * Handles the resize event fo rthe window.
 *
 * Used to keep the image splitters in the correct position and
 * in alighment with each other.
 *
 * @param event The resize event
 */
void SpectrumView::resizeEvent(QResizeEvent * event)
{
  QMainWindow::resizeEvent(event);
  if (m_svConnections)
  {
    m_svConnections->imageSplitterMoved();
  }
}


/**
 * Renders a new workspace on the spectrum viewer.
 *
 * @param wksp The matrix workspace to render
 */
void SpectrumView::renderWorkspace(Mantid::API::MatrixWorkspace_const_sptr wksp)
{
  auto dataSource = MatrixWSDataSource_sptr(new MatrixWSDataSource(wksp));
  m_dataSource.append(dataSource);

  // If we have a MatrixWSDataSource give it the handler for the
  // EMode, so the user can set EMode and EFixed.  NOTE: we could avoid
  // this type checking if we made the ui in the calling code and passed
  // it in.  We would need a common base class for this class and
  // the ref-viewer UI.
  dataSource -> setEModeHandler( m_emodeHandler );

  // Connect WorkspaceObserver signals
  connect(this, SIGNAL(needToClose()), this, SLOT(closeWindow()));
  //connect(this, SIGNAL(needToUpdate()), this, SLOT(updateWorkspace()));

  // Set the window title
  std::string windowTitle = "SpectrumView (" + wksp->getTitle() + ")";
  this->setWindowTitle(QString::fromStdString(windowTitle).simplified());

  auto spectrumPlot = m_ui->spectrumPlot;
  bool isFirstPlot = m_spectrumDisplay.isEmpty();

  int tab = 0;
  if (isFirstPlot)
  {
    m_ui->imageTabs->setTabText(
        m_ui->imageTabs->indexOf(m_ui->imageTabs->currentWidget()),
        QString::fromStdString(wksp->name()));
    m_hGraph = boost::make_shared<GraphDisplay>(m_ui->h_graphPlot,
                                                     m_ui->h_graph_table, false);
    m_vGraph = boost::make_shared<GraphDisplay>(m_ui->v_graphPlot,
                                                     m_ui->v_graph_table, true);
  }
  else
  {
    spectrumPlot = new QwtPlot(this);
    auto widget = new QWidget();
    auto layout = new QHBoxLayout();
    layout->addWidget(spectrumPlot);
    widget->setLayout(layout);
    tab = m_ui->imageTabs->addTab(widget, QString::fromStdString(wksp->name()));
    m_ui->imageTabs->setTabsClosable(true);
  }

  auto spectrumDisplay = boost::make_shared<SpectrumDisplay>( spectrumPlot,
                                           m_sliderHandler,
                                           m_rangeHandler,
                                           m_hGraph.get(), m_vGraph.get(),
                                           m_ui->image_table,
                                           isTrackingOn());
  spectrumDisplay->setDataSource( dataSource );

  if (isFirstPlot)
  {
    m_svConnections = boost::make_shared<SVConnections>( m_ui, this, spectrumDisplay.get(),
                                         m_hGraph.get(), m_vGraph.get() );
    connect(this,SIGNAL(spectrumDisplayChanged(SpectrumDisplay*)),m_svConnections.get(),SLOT(setSpectrumDisplay(SpectrumDisplay*)));
    m_svConnections->imageSplitterMoved();
  }
  else
  {
    foreach(boost::shared_ptr<SpectrumDisplay> sd, m_spectrumDisplay) {
      sd->addOther(spectrumDisplay);
    }
    spectrumDisplay->addOthers(m_spectrumDisplay);
  }

  m_spectrumDisplay.append(spectrumDisplay);
  m_ui->imageTabs->setCurrentIndex(tab);
}


/**
 * Setup the various handlers (energy-mode, slider, range) for UI controls.
 */
void SpectrumView::updateHandlers()
{
  m_emodeHandler = new EModeHandler( m_ui );
  m_sliderHandler = new SliderHandler( m_ui );
  m_rangeHandler = new RangeHandler( m_ui );
}


/**
 * Slot to close the window.
 */
void SpectrumView::closeWindow()
{
  close();
}


/**
 * Signal to close this window if the workspace has just been deleted.
 *
 * @param wsName Name of workspace
 * @param ws Pointer to workspace
 */
void SpectrumView::preDeleteHandle(const std::string& wsName, const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (m_spectrumDisplay.front()->hasData(wsName, ws))
  {
    emit needToClose();
  }
}


/**
 * Signal that the workspace being looked at was just replaced with a different one.
 *
 * @param wsName Name of workspace
 * @param ws Pointer to workspace
 */
void SpectrumView::afterReplaceHandle(const std::string& wsName, const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (m_spectrumDisplay.front()->hasData(wsName, ws))
  {
    renderWorkspace(boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws));
  }
}

void SpectrumView::dropEvent(QDropEvent *de)
{
  auto words = de->mimeData()->text().split('"');
  auto ws = Mantid::API::AnalysisDataService::Instance().retrieveWS<Mantid::API::MatrixWorkspace>(words[1].toStdString());
  renderWorkspace(ws);
}

void SpectrumView::dragMoveEvent(QDragMoveEvent *de)
{
  auto pos = m_ui->imageTabs->mapFrom(this,de->pos());
  if ( m_ui->imageTabs->rect().contains(pos) )
  {
    de->accept();
  }
  else
  {
    de->ignore();
  }
}

void SpectrumView::dragEnterEvent(QDragEnterEvent *de)
{
  if (de->mimeData()->objectName() == "MantidWorkspace") {
    de->acceptProposedAction();
  }
}

void SpectrumView::changeSpectrumDisplay(int tab)
{
  auto spectrumDisplay = m_spectrumDisplay[tab].get();
  m_svConnections->setSpectrumDisplay(spectrumDisplay);
  m_hGraph->clear();
  m_vGraph->clear();
}

void SpectrumView::respondToTabCloseReqest(int tab)
{
  if (m_spectrumDisplay.size() > 1) {
    auto displayToRemove = m_spectrumDisplay[tab];
    for (auto disp = m_spectrumDisplay.begin(); disp != m_spectrumDisplay.end();
         ++disp) {
      if (disp->get() != displayToRemove.get()) {
        (**disp).removeOther(displayToRemove);
      }
    }
    m_svConnections->removeSpectrumDisplay(displayToRemove.get());
    m_spectrumDisplay.remove(displayToRemove);
    m_ui->imageTabs->removeTab(tab);
    m_hGraph->clear();
    m_vGraph->clear();
  }
  if (m_spectrumDisplay.size() == 1) {
    m_ui->imageTabs->setTabsClosable(false);
  }
}

/**
 * Check if mouse tracking should be "always on".
 */
bool SpectrumView::isTrackingOn() const {
  return m_ui->tracking_always_on->isChecked();
}

void SpectrumView::changeTracking(bool on) {
  if (m_spectrumDisplay.isEmpty()) {
    return;
  }
  auto tab = m_ui->imageTabs->currentIndex();
  m_spectrumDisplay[tab]->setTrackingOn(on);
}

/// Load settings
void SpectrumView::loadSettings() {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  m_ui->tracking_always_on->setChecked(settings.value("CursorTracking", true).toBool());
}

/// Save settings
void SpectrumView::saveSettings() const {
  QSettings settings;
  settings.beginGroup("Mantid/MultiDatasetFit");
  settings.setValue("CursorTracking",m_ui->tracking_always_on->isChecked());
}

} // namespace SpectrumView
} // namespace MantidQt
