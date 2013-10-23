
#include <iostream>
#include  "MantidQtSpectrumViewer/SpectrumView.h"
#include  "MantidQtSpectrumViewer/ColorMaps.h"

#include "ui_SpectrumView.h"
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
 *  @param data_source  The source of the data that will be displayed. 
 */
SpectrumView::SpectrumView(QWidget *parent) :
  QMainWindow(parent, 0),
  WorkspaceObserver(),
  m_ui(new Ui::SpectrumViewer())
{
  m_ui->setupUi(this);
}

SpectrumView::~SpectrumView()
{
//  std::cout << "SpectrumView destructor called" << std::endl;

  delete  h_graph;
  delete  v_graph;

  delete  m_ui;
  delete  m_slider_handler;
  delete  m_range_handler;
  delete  m_spectrum_display;
  delete  m_sv_connections;
  if ( m_emode_handler)
  {
    delete m_emode_handler;
  }
}
void SpectrumView::renderWorkspace(Mantid::API::MatrixWorkspace_const_sptr wksp)
{
  MatrixWSDataSource* data_source = new MatrixWSDataSource(wksp);
  this->updateHandlers(data_source);

  // Watch for the deletion of the associated workspace
  observeAfterReplace();
  observePreDelete();
  observeADSClear();

  // connect WorkspaceObserver signals
  connect(this, SIGNAL(needToClose()), this, SLOT(closeWindow()));
  connect(this, SIGNAL(needToUpdate()), this, SLOT(updateWorkspace()));

  // set the window title
  std::string title = std::string("SpectrumView (") +
      wksp->getTitle() +
      std::string(")");
  this->setWindowTitle(QString::fromStdString(title));

  h_graph = new GraphDisplay( m_ui->h_graphPlot, m_ui->h_graph_table, false );
  v_graph = new GraphDisplay( m_ui->v_graphPlot, m_ui->v_graph_table, true );

  m_spectrum_display = new SpectrumDisplay( m_ui->spectrumPlot,
                 m_slider_handler,
                 m_range_handler,
                 h_graph, v_graph,
                 m_ui->image_table );

  m_sv_connections = new SVConnections( m_ui, this, m_spectrum_display,
                                        h_graph, v_graph );

  m_spectrum_display->SetDataSource( data_source );
}

/// Setup the various handlers (energy-mode, slider, range)
void SpectrumView::updateHandlers(SpectrumDataSource* data_source)
{
  // IF we have a MatrixWSDataSource give it the handler for the
  // EMode, so the user can set EMode and EFixed.  NOTE: we could avoid
  // this type checking if we made the ui in the calling code and passed
  // it in.  We would need a common base class for this class and
  // the ref-viewer UI.
  MatrixWSDataSource* matrix_ws_data_source =
                      dynamic_cast<MatrixWSDataSource*>( data_source );
  if ( matrix_ws_data_source != 0 )
  {
    m_emode_handler = new EModeHandler( m_ui );
    matrix_ws_data_source -> SetEModeHandler( m_emode_handler );
  }
  else
  {
    m_emode_handler = 0;
  }

  m_slider_handler = new SliderHandler( m_ui );
  m_range_handler = new RangeHandler( m_ui );

}

/** Slot to close the window */
void SpectrumView::closeWindow()
{
  close();
}

/** Slot to replace the workspace being looked at. */
void SpectrumView::updateWorkspace()
{
  close(); // TODO the right thing
}

/** Signal to close this window if the workspace has just been deleted */
void SpectrumView::preDeleteHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  if (m_spectrum_display->hasData(wsName, ws))
  {
    emit needToClose();
  }
}

/** Signal that the workspace being looked at was just replaced with a different one */
void SpectrumView::afterReplaceHandle(const std::string& wsName,const boost::shared_ptr<Mantid::API::Workspace> ws)
{
  std::cout << "afterReplaceHandle" << std::endl;
  if (m_spectrum_display->hasData(wsName, ws))
  {
//    MatrixWSDataSource* matrix_ws_data_source = new Matrix
//                        dynamic_cast<MatrixWSDataSource>( ws );
//    saved_spectrum_display->SetDataSource(ws); // TODO implement the right thing
    emit needToUpdate();
  }
}

} // namespace SpectrumView
} // namespace MantidQt 
