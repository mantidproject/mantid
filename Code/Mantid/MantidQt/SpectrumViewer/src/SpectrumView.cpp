#include <iostream>
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
  updateHandlers();
}


SpectrumView::~SpectrumView()
{
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

  for(auto svConnection = m_svConnections.begin(); svConnection != m_svConnections.end(); ++svConnection) {
    (**svConnection).imageSplitterMoved();
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

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  // Connect WorkspaceObserver signals
  connect(this, SIGNAL(needToClose()), this, SLOT(closeWindow()));
  //connect(this, SIGNAL(needToUpdate()), this, SLOT(updateWorkspace()));

  // Set the window title
  std::string windowTitle = "SpectrumView (" + wksp->getTitle() + ")";
  this->setWindowTitle(QString::fromStdString(windowTitle).simplified());

  auto hGraph = boost::make_shared<GraphDisplay>(m_ui->h_graphPlot,
                                                   m_ui->h_graph_table, false);
  m_hGraph.append(hGraph);
  auto vGraph = boost::make_shared<GraphDisplay>(m_ui->v_graphPlot,
                                                   m_ui->v_graph_table, true);
  m_vGraph.append(vGraph);

  auto spectrumDisplay = boost::make_shared<SpectrumDisplay>( m_ui->spectrumPlot,
                                           m_sliderHandler,
                                           m_rangeHandler,
                                           hGraph.get(), vGraph.get(),
                                           m_ui->image_table);

  m_svConnections.append(boost::make_shared<SVConnections>( m_ui, this, spectrumDisplay.get(),
                                       hGraph.get(), vGraph.get() ));
  spectrumDisplay->setDataSource( dataSource );
  m_spectrumDisplay.append(spectrumDisplay);
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

} // namespace SpectrumView
} // namespace MantidQt
