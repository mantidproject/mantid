#include <iostream>
#include "MantidQtSpectrumViewer/MatrixWSImageView.h"
#include "MantidQtSpectrumViewer/MatrixWSDataSource.h"

using Mantid::API::MatrixWorkspace_const_sptr;
using namespace MantidQt;
using namespace SpectrumView;

/**
 * Construct an ImageView for the specified matrix workspace
 */
MatrixWSImageView::MatrixWSImageView( MatrixWorkspace_const_sptr mat_ws )
{
  MatrixWSDataSource* source = new MatrixWSDataSource( mat_ws );

  image_view = new ImageView( source );  // this is the QMainWindow
                                         // for the viewer.  It is
                                         // deleted when the window
                                         // is closed

  std::string title = std::string("ImageView ( ") + 
                                   mat_ws->getTitle() + 
                      std::string(" )");

  QString qtitle = QString::fromStdString(title);

  image_view->setCaption( qtitle );
}

MatrixWSImageView::~MatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}

