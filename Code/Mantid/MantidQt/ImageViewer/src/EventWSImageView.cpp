#include <iostream>
#include "MantidQtImageViewer/EventWSImageView.h"
#include "MantidQtImageViewer/EventWSDataSource.h"

using Mantid::API::IEventWorkspace_sptr;
using namespace MantidQt;
using namespace ImageView;

EventWSImageView::EventWSImageView( IEventWorkspace_sptr ev_ws )
{
    std::cout<< "EventWSImageView constructor on Thread " <<
              QThread::currentThreadId() << std::endl;
  EventWSDataSource* source = new EventWSDataSource( ev_ws );
  image_view = new ImageView( source );
}

EventWSImageView::~EventWSImageView()
{
  delete image_view;
}

