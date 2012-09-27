#include <iostream>
#include "MantidQtRefDetectorViewer/RefMatrixWSImageView.h"
#include "MantidQtRefDetectorViewer/RefMatrixWSDataSource.h"

using Mantid::API::MatrixWorkspace_sptr;
using namespace MantidQt;
using namespace RefDetectorViewer;

/**
 * Construct an ImageView for the specified matrix workspace
 */
RefMatrixWSImageView::RefMatrixWSImageView( MatrixWorkspace_sptr mat_ws )
{
  RefMatrixWSDataSource* source = new RefMatrixWSDataSource( mat_ws );

  image_view = new RefImageView( source );  // this is the QMainWindow
                                         // for the viewer.  It is
                                         // deleted when the window
                                         // is closed
}

RefMatrixWSImageView::~RefMatrixWSImageView()
{
  // nothing to do here, since image_view is deleted when the window closes
}

