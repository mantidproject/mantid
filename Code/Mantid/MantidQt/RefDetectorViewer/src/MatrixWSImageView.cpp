#include <iostream>
#include "MantidQtRefDetectorViewer/MatrixWSImageView.h"
#include "MantidQtRefDetectorViewer/MatrixWSDataSource.h"

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt;
using namespace RefDetectorViewer;

/**
 * Construct an ImageView for the specified matrix workspace
 */
MatrixWSImageView::MatrixWSImageView( MatrixWorkspace_sptr mat_ws )
{
  MatrixWSDataSource* source = new MatrixWSDataSource( mat_ws );

  image_view = new ImageView( source );  // this is the QMainWindow
                                         // for the viewer.  It is
                                         // deleted when the window
                                         // is closed
}

MatrixWSImageView::~MatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}

