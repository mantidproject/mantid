#include <iostream>

#include <math.h>

#include "MantidQtImageViewer/TestDataSource.h"
#include "MantidQtImageViewer/IVUtils.h"

namespace MantidQt
{
namespace ImageView
{


TestDataSource::TestDataSource( double total_xmin, double total_xmax,
                                double total_ymin, double total_ymax,
                                size_t total_rows, size_t total_cols )
              :ImageDataSource( total_xmin, total_xmax,
                                total_ymin, total_ymax,
                                total_rows, total_cols )
{
                                                // initialize pointers to null
  new_data       = 0;
  new_data_array = 0;
                                                // make some test data
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
  if ( new_data )
  {
    delete[] new_data;
  }
  if ( new_data_array )
  {
    delete new_data_array;
  }
}


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

  if ( new_data )
  {
    delete[] new_data;
  }
  new_data = new float[n_rows * n_cols];

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

  if ( new_data_array )
  {
    delete new_data_array;
  }

  new_data_array = new DataArray( xmin, xmax, ymin, ymax, 
                                  is_log_x, n_rows, n_cols, new_data);

  return new_data_array;
}


DataArray * TestDataSource::GetDataArray( bool is_log_x )
{
  return GetDataArray( total_xmin, total_xmax, total_ymin, total_ymax,
                       total_rows, total_cols, is_log_x );
}


void TestDataSource::GetInfoList( double x, 
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
