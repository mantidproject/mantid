#include <iostream>

#include <math.h>

#include "MantidQtImageViewer/ImageDataSource.h"

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
double ImageDataSource::GetXMin() const
{
  return total_xmin;
}


/**
 * Get the largest 'x' value covered by the data.
 */
double ImageDataSource::GetXMax() const
{
  return total_xmax;
}


/**
 * Get the smallest 'y' value covered by the data.
 */
double ImageDataSource::GetYMin() const
{
  return total_ymin;
}


/**
 * Get the largest 'y' value covered by the data.
 */
double ImageDataSource::GetYMax() const
{
  return total_ymax;
}


/**
 * Get the total number of rows the data is divided into
 */
size_t ImageDataSource::GetNRows() const
{
  return total_rows;
}


/**
 * Get the total number of columns the data is divided into
 */
size_t ImageDataSource::GetNCols() const
{
  return total_cols;
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
