#include <iostream>

#include <math.h>

#include "MantidQtSpectrumViewer/ImageDataSource.h"

namespace MantidQt
{
namespace ImageView
{


/**
 *  Construct data source with specified total range and data size.
 *
 *  @param total_xmin  The smallest 'x' value covered by the data
 *  @param total_xmax  The largest 'x' value covered by the data
 *  @param total_ymin  The smallest 'y' value covered by the data
 *  @param total_ymax  The largest 'y' value covered by the data
 *  @param total_rows  The total number of rows the data is divided into
 *  @param total_cols  The total number of columns the data is divided into
 */
ImageDataSource::ImageDataSource( double total_xmin, double total_xmax,
                                  double total_ymin, double total_ymax,
                                  size_t total_rows, size_t total_cols )
{
  this->total_xmin = total_xmin;
  this->total_xmax = total_xmax;
  this->total_ymin = total_ymin;
  this->total_ymax = total_ymax;
  this->total_rows = total_rows;
  this->total_cols = total_cols;
}


ImageDataSource::~ImageDataSource()
{
}


/**
 * Get the smallest 'x' value covered by the data.
 */
double ImageDataSource::GetXMin()
{
  return total_xmin;
}


/**
 * Get the largest 'x' value covered by the data.
 */
double ImageDataSource::GetXMax()
{
  return total_xmax;
}


/**
 * Get the smallest 'y' value covered by the data.
 */
double ImageDataSource::GetYMin()
{
  return total_ymin;
}


/**
 * Get the largest 'y' value covered by the data.
 */
double ImageDataSource::GetYMax()
{
  return total_ymax;
}


/**
 * Get the total number of rows the data is divided into
 */
size_t ImageDataSource::GetNRows()
{
  return total_rows;
}


/**
 * Get the total number of columns the data is divided into
 */
size_t ImageDataSource::GetNCols()
{
  return total_cols;
}


/**
 * Clamp x to the interval of x-values covered by this DataSource
 * @param x   If x is more than xmax it will be set to xmax. If x is less 
 *            than xmin, it will be set to xmin.
 */
void ImageDataSource::RestrictX( double & x )
{
  if ( x > total_xmax )
  {
    x = total_xmax;
  }
  else if ( x < total_xmin )
  {
    x = total_xmin;
  }
}


/**
 * Clamp y to the interval of y-values covered by this DataSource.
 * @param y   If y is more than ymax it will be set to ymax. If y is less 
 *            than ymin, it will be set to ymin.
 */
void ImageDataSource::RestrictY( double & y )
{
  if ( y > total_ymax )
  {
    y = total_ymax;
  }
  else if ( y < total_ymin )
  {
    y = total_ymin;
  }
}


/**
 * Clamp row to a valid row number for this DataSource.
 * @param row  If row is more than n_rows-1, it is set to n_rows-1.  If
 *             row < 0 it is set to zero.
 */
void ImageDataSource::RestrictRow( int & row )
{
  if ( row >= (int)total_rows )
  {
    row = (int)total_rows - 1;
  }
  else if ( row < 0 )
  {
    row = 0;
  }
}


/**
 * Clamp col to a valid column number for this DataSource.
 * @param col  If col is more than n_cols-1, it is set to n_cols-1.  If
 *             col < 0 it is set to zero.
 */
void ImageDataSource::RestrictCol( int & col )
{
  if ( col >= (int)total_cols )
  {
    col = (int)total_cols - 1;
  }
  else if ( col < 0 )
  {
    col = 0;
  }
}


/**
 *  Convenience method to get all the data at the maximum resolution.
 */
DataArray* ImageDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}

} // namespace MantidQt 
} // namespace ImageView 
