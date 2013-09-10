#include <iostream>

#include <math.h>

#include "MantidQtSpectrumViewer/ArrayDataSource.h"
#include "MantidQtSpectrumViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{

/**
 * Construct a DataSource object to to display data from the specified
 * array.  This class takes ownership of the data and will delete it
 * when this class is deleted.  The calling code should not change the
 * values in the array or delete it!
 *
 * @param total_xmin   The x-coordinate at the left edge of the first column
 *                     of data. 
 * @param total_xmax   The x-coordinate at the right edge of the last column
 *                     of data. 
 * @param total_ymin   The y-coordinate at the bottom edge of bottom row of
 *                     the data region.
 * @param total_ymax   The y-coordinate at the top edge of the top row of
 *                     the data region
 * @param total_rows   The number of rows the data values are divided into.
 * @param total_cols   The number of columns the test values are divided 
 *                     into.
 * @param data         The list of floats holding the data to be displayed,
 *                     stored in row major order.                    
 */
ArrayDataSource::ArrayDataSource( double total_xmin, double total_xmax,
                                  double total_ymin, double total_ymax,
                                  size_t total_rows, size_t total_cols,
                                  float* data )
                :ImageDataSource( total_xmin, total_xmax,
                                  total_ymin, total_ymax,
                                  total_rows, total_cols )
{
  this->data       = data;
}


ArrayDataSource::~ArrayDataSource()
{
  delete[] data;
}


/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.  NOTE: The calling code is responsible for deleting the 
 * DataArray that is returned, when it is no longer needed.
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
 *                  binned logarithmically in the X-direction.  This
 *                  DataSource does not support rebinning to a log axis, so
 *                  the DataArray is always returned with is_log_x = false. 
 */
DataArray * ArrayDataSource::GetDataArray( double xmin,   double  xmax,
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

  float* new_data = new float[n_rows * n_cols];   // This is deleted in the
                                                  // DataArray destructor

  double x_step = (xmax - xmin) / (double)n_cols;
  double y_step = (ymax - ymin) / (double)n_rows;
  double mid_y,
         mid_x;
  double d_x_index,
         d_y_index;
  size_t source_row,
         source_col;
  size_t index = 0;                               // get data for middle of
  for ( size_t row = 0; row < n_rows; row++ )     // each destination position
  {
    mid_y = ymin + ((double)row + 0.5) * y_step;
    IVUtils::Interpolate( total_ymin, total_ymax, mid_y,
                                 0.0, (double)total_rows, d_y_index );
    source_row = (size_t)d_y_index;
    for ( size_t col = 0; col < n_cols; col++ )
    {
      mid_x = xmin + ((double)col + 0.5) * x_step;
      IVUtils::Interpolate( total_xmin, total_xmax, mid_x,
                                   0.0, (double)total_cols, d_x_index );
      source_col = (size_t)d_x_index;             
      new_data[index] = data[source_row * total_cols + source_col];
      index++;
    } 
  }
                                           // The calling code is responsible
  is_log_x = false;                        // for deleting the DataArray
  DataArray* new_data_array = new DataArray( xmin, xmax, ymin, ymax, 
                                           is_log_x, n_rows, n_cols, new_data);
  return new_data_array;
}


/**
 * Get a data array covering the full range of data.
 * NOTE: The calling code is responsible for deleting the DataArray that is
 * returned, when it is no longer needed.
 *
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.  This DataSource does not 
 *                  support rebinning to a log axis, so the DataArray is 
 *                  always returned with is_log_x = false.
 */
DataArray * ArrayDataSource::GetDataArray( bool is_log_x )
{
  is_log_x = false;
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
void ArrayDataSource::GetInfoList( double x, 
                                   double y,
                                   std::vector<std::string> &list )
{
  list.clear();

  IVUtils::PushNameValue( "X", 8, 3, x, list );
  IVUtils::PushNameValue( "Y", 8, 3, y, list );
}

} // namespace MantidQt 
} // namespace ImageView
