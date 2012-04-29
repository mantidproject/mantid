#include <iostream>

#include <math.h>

#include "MantidQtImageViewer/TestDataSource.h"
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{


/**
 * Construct a DataSource object to provide fake test data over the
 * specified region using the specified number of rows and columns. 
 *
 * @param total_xmin   The x-coordinate at the left edge of the data region
 * @param total_xmax   The x-coordinate at the right edge of the data region
 * @param total_ymin   The y-coordinate at the bottom edge of the data region
 * @param total_ymax   The y-coordinate at the top edge of the data region
 * @param total_rows   The number of rows the test data should be divided into
 * @param total_cols   The number of columns the test data should be divided 
 *                     into
 */
TestDataSource::TestDataSource( double total_xmin, double total_xmax,
                                double total_ymin, double total_ymax,
                                size_t total_rows, size_t total_cols )
              :ImageDataSource( total_xmin, total_xmax,
                                total_ymin, total_ymax,
                                total_rows, total_cols )
{
                                        // make some test data in array data[]
  double x;
  double y;
  data = new float[total_rows*total_cols];
  for ( size_t row = 0; row < total_rows; row++ )
    for ( size_t col = 0; col < total_cols; col++ )
    {
       x = ((double)col - (double)total_cols/2.0)/(double)total_cols; 
       y = ((double)row - (double)total_rows/2.0)/(double)total_rows; 
       data[ row * total_cols + col ] = (float)
                ((double)(row + col) + 1000.0*(1.0+cos( (x*x + y*y)*10.0 )));
    }
                                                // mark a row 1/4 way up
  double point = (total_ymax - total_ymin)/4 + total_ymin;
  double mark_row = 0;
  IVUtils::Interpolate( total_ymin, total_ymax, point,
                               0.0, (double)total_rows, mark_row );

  size_t row_offset = (int)(mark_row) * total_cols;
  for ( size_t col = 0; col < total_cols; col++ )
  {
     data[ row_offset + col ] = 0;
  }
                                                 // mark a col 1/10 way over
  point = (total_xmax - total_xmin)/10 + total_xmin;
  double mark_col = 0;
  IVUtils::Interpolate( total_xmin, total_xmax, point,
                               0.0, (double)total_cols, mark_col );

  size_t col_offset = (int)( mark_col );
  for ( size_t row = 0; row < total_rows; row++ )
  {
     data[ row * total_cols + col_offset ] = (float)(total_rows + total_cols);
  }
}


TestDataSource::~TestDataSource()
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
 *                  binned logarithmically.  (NOT USED)
 */
DataArray * TestDataSource::GetDataArray( double xmin,   double  xmax,
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
                                           // for deleting the DataArray
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
 *                  binned logarithmically.  (NOT USED)
 */
DataArray * TestDataSource::GetDataArray( bool is_log_x )
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
void TestDataSource::GetInfoList( double x, 
                                  double y,
                                  std::vector<std::string> &list )
{
  list.clear();

  IVUtils::PushNameValue( "Test X", 8, 3, x, list );
  IVUtils::PushNameValue( "Test Y", 8, 3, y, list );
}

} // namespace MantidQt 
} // namespace ImageView
