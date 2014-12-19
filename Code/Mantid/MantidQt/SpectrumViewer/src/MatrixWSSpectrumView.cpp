#include <iostream>
#include "MantidQtSpectrumViewer/MatrixWSSpectrumView.h"
#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"

using Mantid::API::MatrixWorkspace_const_sptr;
using namespace MantidQt;
using namespace SpectrumView;


/**
 * Construct an SpectrumView for the specified matrix workspace
 */
MatrixWSSpectrumView::MatrixWSSpectrumView( MatrixWorkspace_const_sptr mat_ws )
{
  /* This is the QMainWindow for the viewer. */
  /* It is deleted when the window is closed. */
  spectrum_view = new SpectrumView();

  std::string title = "SpectrumView (" + mat_ws->getTitle() + ")";
  QString qtitle = QString::fromStdString(title);
  spectrum_view->setCaption( qtitle );
  spectrum_view->renderWorkspace(mat_ws);
}


MatrixWSSpectrumView::~MatrixWSSpectrumView()
{
  // nothing to do here, since spectrum_view is deleted when the window closes
}

