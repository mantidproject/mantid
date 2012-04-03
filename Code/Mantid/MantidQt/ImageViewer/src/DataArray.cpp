
#include <iostream>

#include "MantidQtImageViewer/DataArray.h"

namespace MantidQt
{
namespace ImageView
{


DataArray::DataArray( double xmin,     double xmax,
                      double ymin,     double ymax,
                      bool   is_log_x,
                      size_t n_rows,   size_t n_cols,
                      float *data )
{
  this->xmin = xmin;
  this->xmax = xmax;
  this->ymin = ymin;
  this->ymax = ymax;
  this->is_log_x = is_log_x;
  this->n_rows = n_rows;
  this->n_cols = n_cols;
  this->data = data;

  data_min = data[0];
  data_max = data[0];
  double value;
  size_t index = 0;
  for ( size_t row = 0; row < n_rows; row++ )
    for ( size_t col = 0; col < n_cols; col++ )
    {
      value = data[index];
      if ( value < data_min )
        data_min = value;
      else if ( value > data_max )
        data_max = value;
      index++;
    }
/*
   std::cout << "DataArray Constructor, data_min = " << data_min << std::endl;
   std::cout << "DataArray Constructor, data_max = " << data_max << std::endl;
*/
}

double DataArray::GetXMin() const
{
  return xmin;
}

double DataArray::GetXMax() const
{
  return xmax;
}

double DataArray::GetYMin() const
{
  return ymin;
}

double DataArray::GetYMax() const
{
  return ymax;
}

bool DataArray::GetIsLogX() const
{
  return is_log_x;
}

double DataArray::GetDataMin() const
{
  return data_min;
}

double DataArray::GetDataMax() const
{
  return data_max;
}

size_t DataArray::GetNRows() const
{
  return n_rows;
}

size_t DataArray::GetNCols() const
{
  return n_cols;
}

float * DataArray::GetData() const
{
  return data;
}

double DataArray::GetValue( int row, int col ) const
{
  if ( row < 0 )
  {
    row = 0;
  }
  if ( row > (int)n_rows - 1 )
  {
    row = (int)n_rows - 1;
  }
  if ( col < 0 )
  {
    col = 0;
  }
  if ( col > (int)n_cols - 1 )
  {
    col = (int)n_cols - 1;
  }

  return data[ row * n_cols + col ];
}

double DataArray::GetValue( double x, double y ) const
{
  double relative_x = (x - xmin) / (xmax - xmin);
  int col = (int)( relative_x * (double)n_cols );

  double relative_y = (y - ymin) / (ymax - ymin);
  int row = (int)( relative_y * (double)n_rows );

  return GetValue( row, col );
}


} // namespace MantidQt 
} // namespace ImageView 
