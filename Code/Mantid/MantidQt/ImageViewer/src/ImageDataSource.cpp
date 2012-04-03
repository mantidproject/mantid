#include <iostream>

#include <math.h>

#include "MantidQtImageViewer/ImageDataSource.h"

namespace MantidQt
{
namespace ImageView
{


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


double ImageDataSource::GetXMin() const
{
  return total_xmin;
}

double ImageDataSource::GetXMax() const
{
  return total_xmax;
}

double ImageDataSource::GetYMin() const
{
  return total_ymin;
}

double ImageDataSource::GetYMax() const
{
  return total_ymax;
}

size_t ImageDataSource::GetNRows() const
{
  return total_rows;
}

size_t ImageDataSource::GetNCols() const
{
  return total_cols;
}


DataArray* ImageDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}

} // namespace MantidQt 
} // namespace ImageView 
