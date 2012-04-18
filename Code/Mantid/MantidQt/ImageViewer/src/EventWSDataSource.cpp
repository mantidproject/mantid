/**
 *  File: EventWSDataSource.cpp
 */

#include <iostream>
#include <math.h>

#include "MantidQtImageViewer/EventWSDataSource.h"
#include "MantidQtImageViewer/IVUtils.h"
#include "MantidAPI/ISpectrum.h"

using namespace Mantid;
using namespace DataObjects;
using namespace Kernel;
using namespace API;

namespace MantidQt
{
namespace ImageView
{

/**
 * Construct a DataSource object around the specifed EventWorkspace.
 *
 * @param ev_ws  Shared pointer to the event workspace being "wrapped"
 */
EventWSDataSource::EventWSDataSource( EventWorkspace_sptr ev_ws )
                 :ImageDataSource( 0.0, 1.0, 0.0, 1.0, 0, 0 )  // some defaults
{
  this->ev_ws = ev_ws;
  total_xmin = ev_ws->getTofMin(); 
  total_xmax = ev_ws->getTofMax(); 
  total_ymin = 0;                 // y direction is spectrum index
  total_ymax = (double)ev_ws->getNumberHistograms();
  total_rows = ev_ws->getNumberHistograms();
  total_cols = 500;              // initially use 500 bins for event data

//  std::cout << "total_xmin = " << total_xmin << std::endl;
//  std::cout << "total_xmax = " << total_xmax << std::endl;

  if ( total_xmax > 120000 )   
  {
    total_xmax = 120000;          // hack for now
    std::cout << "WARNING: max tof too large, set to " 
              << total_xmax << std::endl;
  }

  x_scale = new MantidVec();
  x_scale->resize(total_cols+1);
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


/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.
 *
 * @param xmin      Left edge of region to be covered.
 * @param xmax      Right edge of region to be covered.
 * @param ymin      Bottom edge of region to be covered.
 * @param ymax      Top edge of region to be covered.
 * @param n_rows    Number of rows to return. If the number of rows is less
 *                  than the actual number of data rows in [ymin,ymax], the 
 *                  data will be subsampled, and only the specified number 
 *                  of rows will be returned.
 * @param n_cols    The event data will be rebinned using the specified
 *                  number of colums.
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.  (NOT USED YET)
 */
DataArray* EventWSDataSource::GetDataArray( double xmin,   double  xmax,
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
  x_scale->resize(n_cols+1);
  double dx = (xmax - xmin)/((double)n_cols + 1.0);
  for ( size_t i = 0; i < n_cols+1; i++ )
  {
    (*x_scale)[i] = xmin + (double)i * dx;;
  }

  size_t index = 0;
  for ( size_t i = first_row; i < first_row + n_rows; i++ )
  {
    EventList & list = ev_ws->getEventList(i);
    list.setX( *x_scale );
    list.setTofs( *x_scale );
    const MantidVec & y_vals = ev_ws->readY(i);
    for ( size_t col = 0; col < n_cols; col++ )
    {
      new_data[index] = (float)y_vals[col];
      index++;
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


/**
 * Get a data array covering the full range of data.
 *
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.  (NOT USED YET)
 */
DataArray * EventWSDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}

/**
 * Clear the vector of strings and then add pairs of strings giving information
 * about the specified point, x, y.  The first string in a pair should 
 * generally be a string describing the value being presented and the second
 * string should contain the value.
 *  
 * @param x    The x-coordinate of the point of interest in the data.
 * @param y    The y-coordinate of the point of interest in the data.
 * @param list Vector that will be filled out with the information strings.
 */
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

  ISpectrum* spec = ev_ws->getSpectrum( (int)y );
  double spec_id = spec->getSpectrumNo();

  list.push_back("Spec ID");
  std::string spec_id_str;
  IVUtils::Format(8,0,spec_id,spec_id_str);
  list.push_back( spec_id_str );
}

} // namespace MantidQt 
} // namespace ImageView
