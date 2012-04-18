#include "MantidDataObjects/EventWorkspace.h"

#include "MantidQtImageViewer/EventWSImageView.h"
#include "MantidQtImageViewer/EventWSDataSource.h"

using namespace MantidQt;
using namespace ImageView;

EventWSImageView::EventWSImageView( EventWorkspace_sptr ev_ws )
{
  EventWSDataSource* source = new EventWSDataSource( ev_ws );
  image_view = new ImageView( source );
}

EventWSImageView::~EventWSImageView()
{
  delete image_view;
}

