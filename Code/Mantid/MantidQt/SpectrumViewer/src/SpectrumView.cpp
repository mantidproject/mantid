#include <iostream>
#include "MantidQtSpectrumViewer/SpectrumView.h"
#include "MantidQtSpectrumViewer/ColorMaps.h"

#include "MantidQtSpectrumViewer/SVConnections.h"
#include "MantidQtSpectrumViewer/SpectrumDisplay.h"
#include "MantidQtSpectrumViewer/SliderHandler.h"
#include "MantidQtSpectrumViewer/RangeHandler.h"
#include "MantidQtSpectrumViewer/EModeHandler.h"
#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"

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
  m_hGraph(NULL), m_vGraph(NULL),
  m_ui(new Ui::SpectrumViewer()),
  m_sliderHandler(NULL),
  m_rangeHandler(NULL),
  m_spectrumDisplay(NULL),
  m_svConnections(NULL),
  m_emodeHandler(NULL)
{
  m_ui->setupUi(this);
}


SpectrumView::~SpectrumView()
{
  delete m_spectrumDisplay;
  delete m_svConnections;

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

  if(m_svConnections)
    m_svConnections->imageSplitterMoved();
}


/**
 * Renders a new workspace on the spectrum viewer.
 *
 * @param wksp The matrix workspace to render
 */
void SpectrumView::renderWorkspace(Mantid::API::MatrixWorkspace_const_sptr wksp)
{
  m_dataSource = MatrixWSDataSource_sptr(new MatrixWSDataSource(wksp));

  updateHandlers(m_dataSource);

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  // Connect WorkspaceObserver signals
  connect(this, SIGNAL(needToClose()), this, SLOT(closeWindow()));
  connect(this, SIGNAL(needToUpdate()), this, SLOT(updateWorkspace()));

  // Set the window title
  std::string windowTitle = "SpectrumView (" + wksp->getTitle() + ")";
  this->setWindowTitle(QString::fromStdString(windowTitle).simplified());

  // Remove the old graph plots
  if(m_hGraph) delete m_hGraph;
  if(m_vGraph) delete m_vGraph;
  if(m_spectrumDisplay) delete m_spectrumDisplay;
  if(m_svConnections) delete m_svConnections;

  m_hGraph = new GraphDisplay( m_ui->h_graphPlot, m_ui->h_graph_table, false );
  m_vGraph = new GraphDisplay( m_ui->v_graphPlot, m_ui->v_graph_table, true );

  m_spectrumDisplay = new SpectrumDisplay( m_ui->spectrumPlot,
                                           m_sliderHandler,
                                           m_rangeHandler,
                                           m_hGraph, m_vGraph,
                                           m_ui->image_table);

  m_svConnections = new SVConnections( m_ui, this, m_spectrumDisplay,
                                       m_hGraph, m_vGraph );

  m_spectrumDisplay->setDataSource( m_dataSource );
}


/**
 * Setup the various handlers (energy-mode, slider, range) for UI controls.
 *
 * @param dataSource The data source for the current workspace
 */
void SpectrumView::updateHandlers(SpectrumDataSource_sptr dataSource)
{
  // If we have a MatrixWSDataSource give it the handler for the
  // EMode, so the user can set EMode and EFixed.  NOTE: we could avoid
  // this type checking if we made the ui in the calling code and passed
  // it in.  We would need a common base class for this class and
  // the ref-viewer UI.
  MatrixWSDataSource_sptr matrixWsDataSource = boost::dynamic_pointer_cast<MatrixWSDataSource>( dataSource );

  if ( matrixWsDataSource != NULL )
  {
    m_emodeHandler = new EModeHandler( m_ui );
    matrixWsDataSource -> setEModeHandler( m_emodeHandler );
  }
  else
  {
    m_emodeHandler = NULL;
  }

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
  if (m_spectrumDisplay->hasData(wsName, ws))
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
  if (m_spectrumDisplay->hasData(wsName, ws))
  {
    renderWorkspace(boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(ws));
  }
}

} // namespace SpectrumView
} // namespace MantidQt
