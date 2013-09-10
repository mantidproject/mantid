/**
 * File DataArray.cpp
 */

#include <iostream>
#include <math.h>

#include "MantidQtSpectrumViewer/DataArray.h"

namespace MantidQt
{
namespace SpectrumView
{

/**
 * Construct a DataArray "wrapper" around the data and region info.  The 
 * actual data must be provided in a one-dimensional array, with n_rows*n_cols
 * entries.  The data corresponds to the "real" region [xmin,xmax]X[ymin,ymax].
 * Xmin must correspond to the left edge of the first column and xmax must
 * correspond to the right edge of the last column.  Simiarly, ymin must 
 * correspond to the outer edge of the first row and ymax must correspond to 
 * the outer edge of the last row.
 *
 * @param xmin       Left edge of data region
 * @param xmax       Right edge of data region
 * @param ymin       Bottom edge of data region
 * @param ymax       Top edge of data region
 * @param is_log_x   Flag indication whether or not the data is binned
 *                   logarithmically in the 'x' direction.
 * @param n_rows     Number of rows in the data array
 * @param n_cols     Number of columns in the data array
 * @param data       Pointer to start of memory block holding the actual 
 *                   data as a list of floats.
 */
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
}

DataArray::~DataArray()
{
//  std::cout << "DataArray destructor called" << std::endl;

  if ( data )
  {
    delete[] data;
  }
}

/**
 * Get the value corresponding to the left edge of the array.
 */
double DataArray::GetXMin() const
{
  return xmin;
}

/**
 * Get the value corresponding to the right edge of the array.
 */
double DataArray::GetXMax() const
{
  return xmax;
}

/**
 * Get the value corresponding to the bottom edge of the array (outer edge
 * of first row).
 */
double DataArray::GetYMin() const
{
  return ymin;
}

/**
 * Get the value corresponding to the top edge of the array (outer edge
 * of last row).
 */
double DataArray::GetYMax() const
{
  return ymax;
}

/**
 * Check if the returned array is binned logarithmically in 'x'.
 */
bool DataArray::IsLogX() const
{
  return is_log_x;
}

/**
 * Get smallest value recorded in this DataArray
 */
double DataArray::GetDataMin() const
{
  return data_min;
}


/**
 * Get largest value recorded in this DataArray
 */
double DataArray::GetDataMax() const
{
  return data_max;
}


/**
 * Get the actual number of rows in this DataArray
 *
 */
size_t DataArray::GetNRows() const
{
  return n_rows;
}


/**
 * Get the actual number of columns in this DataArray
 */
size_t DataArray::GetNCols() const
{
  return n_cols;
}


/**
 * Get the list of all values, packed in a 1-D array, in row-major order
 */
float * DataArray::GetData() const
{
  return data;
}


/**
 * Get the value at the specified row and column.  If the row or column
 * value is outside of the array, a value from the edge of the array
 * will be returned.  That is, the row and column numbers are "clamped"
 * to always lie in the range of valid values.
 */
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


/**
 * Get the value from the row and column containing the specified point.
 * If the specified point (x,y) is off the edge of the array, a value
 * from the edge of the array will be returned. 
 */
double DataArray::GetValue( double x, double y ) const
{
/*
  int col = 0;
  if ( is_log_x )
  {
    col = (int)((double)n_cols * log(x/xmin)/log(xmax/xmin) );
  }
  else
  {
    double relative_x = (x - xmin) / (xmax - xmin);
    col = (int)( relative_x * (double)n_cols );
  }

  double relative_y = (y - ymin) / (ymax - ymin);
  int row = (int)( relative_y * (double)n_rows );
*/
  size_t col = ColumnOfX( x );
  size_t row = RowOfY( y );

  return GetValue( (int)row, (int)col );
}


/**
 * Clamp x to the interval of x-values covered by this DataArray.
 * @param x   If x is more than xmax it will be set to xmax. If x is less 
 *            than xmin, it will be set to xmin.
 */
void DataArray::RestrictX( double & x ) const
{
  if ( x > xmax )
  {
    x = xmax;
  }
  else if ( x < xmin )
  {
    x = xmin;
  }
}


/**
 * Clamp y to the interval of y-values covered by this DataArray.
 * @param y   If y is more than ymax it will be set to ymax. If y is less 
 *            than ymin, it will be set to ymin.
 */
void DataArray::RestrictY( double & y ) const
{
  if ( y > ymax )
  {
    y = ymax;
  }
  else if ( y < ymin )
  {
    y = ymin;
  }
}


/**
 * Clamp row to a valid row number for this DataArray.
 * @param row  If row is more than n_rows-1, it is set to n_rows-1.  If
 *             row < 0 it is set to zero.
 */
void DataArray::RestrictRow( int & row ) const
{
  if ( row >= (int)n_rows )
  {
    row = (int)n_rows - 1;
  }
  else if ( row < 0 )
  {
    row = 0;
  }
}


/**
 * Clamp col to a valid column number for this DataArray.
 * @param col  If col is more than n_cols-1, it is set to n_cols-1.  If
 *             col < 0 it is set to zero.
 */
void DataArray::RestrictCol( int & col ) const
{
  if ( col >= (int)n_cols )
  {
    col = (int)n_cols - 1;
  }
  else if ( col < 0 )
  {
    col = 0;
  }
}


/**
 * Calculate the column number containing the specified x value.  If the
 * specified value is less than xmin, 0 is returned.  If the specified
 * value is more than or equal to xmax, n_cols-1 is returned.  This
 * method uses the is_log_x flag to determine whether to use a "log"
 * transformation to map the column to x.
 *
 * @param x  The x value to map to a column number
 *
 * @return A valid column number, containing x, if x is in range, or the
 *         first or last column number if x is out of range.
 */
size_t DataArray::ColumnOfX( double x ) const
{
  int col;
  if ( is_log_x )
  {
    col = (int)((double)n_cols * log(x/xmin)/log(xmax/xmin));
  }
  else
  {
    col = (int)((double)n_cols * (x-xmin)/(xmax-xmin));
  }

  RestrictCol( col );
  return (size_t)col;
}


/*
 * Calculate the x-value at the center of the specified column.  If the 
 * column number is too large, xmax is returned.  If the column number is
 * too small, xmin is returned.  This method uses the is_log_x flag to
 * determine whether to use a "log" tranformation to map the column to x.
 *
 * @param col  The column number to map to an x-value.
 *
 * @return A corresponding x value between xmin and xmax.
 */
double DataArray::XOfColumn( size_t col ) const
{
  double xval;
  if ( is_log_x )
  {
    xval = xmin * exp( ((double)col+0.5)/(double)n_cols * log(xmax/xmin));
  }
  else
  {
    xval = ((double)col+0.5)/(double)n_cols * (xmax-xmin) + xmin;
  }
 
  RestrictX( xval );
  return xval;
}


/**
 * Calculate the row number containing the specified y value.  If the
 * specified value is less than ymin, 0 is returned.  If the specified
 * value is more than or equal to ymax, n_rows-1 is returned. 
 *
 * @param y  The y value to map to a row number
 *
 * @return A valid row number, containing y, if y is in range, or the
 *         first or last row number if y is out of range.
 */
size_t DataArray::RowOfY( double y ) const
{
  int row = (int)((double)n_rows * (y-ymin)/(ymax-ymin));

  RestrictRow( row );
  return (size_t)row;
}


/*
 * Calculate the y-value at the center of the specified row.  If the 
 * row number is too large, ymax is returned.  If the row number is
 * too small, ymin is returned.  
 *
 * @param row  The row number to map to an y-value.
 *
 * @return A corresponding y value between ymin and ymax.
 */
double DataArray::YOfRow( size_t row ) const
{
  double yval;
  yval = ((double)row+0.5)/(double)n_rows * (ymax-ymin) + ymin;

  RestrictY( yval );
  return yval;
}


} // namespace SpectrumView
} // namespace MantidQt 
