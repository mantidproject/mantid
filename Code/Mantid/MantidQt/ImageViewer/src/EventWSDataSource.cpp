#include <iostream>

#include <math.h>

#include "MantidQtImageViewer/EventWSDataSource.h"
#include "MantidQtImageViewer/IVUtils.h"

using namespace Mantid;
using namespace DataObjects;
using namespace Kernel;

namespace MantidQt
{
namespace ImageView
{

EventWSDataSource::EventWSDataSource( EventWorkspace* ev_ws )
                 :ImageDataSource( 0.0, 1.0, 0.0, 1.0, 0, 0 )  // some defaults
{
  this->ev_ws = ev_ws;
  total_xmin = ev_ws->getXMin(); 
  total_xmax = ev_ws->getXMax(); 
  total_ymin = 0;                 // y direction is spectrum index
  total_ymax = (double)ev_ws->getNumberHistograms();
  total_rows = ev_ws->getNumberHistograms();
  total_cols = 1000;              // initially use 1000 bins for event data

  x_scale = new MantidVec();
  x_scale->resize(1000);
  double dx = (total_xmax - total_xmin)/((double)total_cols + 1.0);
  for ( size_t i = 0; i < total_cols+1; i++ )
  {
    (*x_scale)[i] = total_xmin + (double)i * dx;;
  }

  for ( size_t i = 0; i < total_rows; i++ )
  {
    EventList & list = ev_ws->getEventList(i);
    list.setX( *x_scale );
    list.setTofs( *x_scale );
  }

  new_data = 0;                   // no data loaded yet
  new_data_array = 0;             // no DataArray object created yet
}


EventWSDataSource::~EventWSDataSource()
{
  delete x_scale;

  if ( new_data != 0 )
  {
    delete[] new_data;
  }

  if ( new_data_array != 0 )
  {
    delete new_data_array;
  }
}


DataArray * EventWSDataSource::GetDataArray( double xmin,   double  xmax,
                                             double ymin,   double  ymax,
                                             size_t n_rows, size_t  n_cols,
                                             bool   is_log_x )
{
  size_t first_col;
  IVUtils::CalculateInterval( total_xmin, total_xmax, total_cols,
                              first_col, xmin, xmax, n_cols );

  size_t first_row;
  IVUtils::CalculateInterval( total_ymin, total_ymax, total_rows,
                              first_row, ymin, ymax, n_rows );

  if ( new_data )
  {
    delete[] new_data;
  }
  new_data = new float[n_rows * n_cols];

  delete x_scale;
  x_scale = new MantidVec();
  x_scale->resize(n_cols);
  double dx = (xmax - xmin)/((double)total_cols + 1.0);
  for ( size_t i = 0; i < total_cols+1; i++ )
  {
    (*x_scale)[i] = total_xmin + (double)i * dx;;
  }

  size_t index = 0;
  for ( size_t i = first_row; i < first_row + n_rows; i++ )
  {
    EventList & list = ev_ws->getEventList(i);
    list.setX( *x_scale );
    list.setTofs( *x_scale );
    MantidVec & y_vals = ev_ws->dataY(i);
    for ( size_t col = 0; col < n_cols; col++ )
    {
      new_data[index++] = (float)y_vals[col];
    }
  }

  if ( new_data_array )
  {
    delete new_data_array;
  }
  new_data_array = new DataArray( xmin, xmax, ymin, ymax,
                                  is_log_x, n_rows, n_cols, new_data);

  return new_data_array;
}


DataArray * EventWSDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}


void EventWSDataSource::GetInfoList( double x, 
                                     double y,
                                     std::vector<std::string> &list )
{
  list.clear();

  list.push_back("Test X:");
  std::string x_str;
  IVUtils::Format(8,3,x,x_str);
  list.push_back(x_str);

  list.push_back("Test Y:");
  std::string y_str;
  IVUtils::Format(8,3,y,y_str);
  list.push_back(y_str);
}

} // namespace MantidQt 
} // namespace ImageView
