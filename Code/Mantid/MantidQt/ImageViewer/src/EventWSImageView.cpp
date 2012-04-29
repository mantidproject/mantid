#include <iostream>
#include "MantidQtImageViewer/EventWSImageView.h"
#include "MantidQtImageViewer/EventWSDataSource.h"

using Mantid::API::IEventWorkspace_sptr;
using namespace MantidQt;
using namespace ImageView;

EventWSImageView::EventWSImageView( IEventWorkspace_sptr ev_ws )
{
  EventWSDataSource* source = new EventWSDataSource( ev_ws );

  image_view = new ImageView( source );  // this is the QMainWindow
                                         // for the viewer.  It is
                                         // deleted when the window
                                         // is closed
}

EventWSImageView::~EventWSImageView()
{
}

