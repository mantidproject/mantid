
#include <iostream>

#include <qapplication.h>                                                        
#include <QMainWindow>
#include <QtGui>

#include "MantidQtSpectrumViewer/IVUtils.h"
#include "MantidQtSpectrumViewer/SpectrumView.h"
#include "MantidQtSpectrumViewer/ArrayDataSource.h"

using namespace MantidQt;
using namespace SpectrumView;

/**
 * Construct an array of test data over the specified region with the
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
float * MakeTestData( double total_xmin, double total_xmax,
                      double total_ymin, double total_ymax,
                      size_t total_rows, size_t total_cols )
{
                                        // make some test data in array data[]
  double x;
  double y;
  float* data = new float[total_rows*total_cols];
  for ( size_t row = 0; row < total_rows; row++ )
    for ( size_t col = 0; col < total_cols; col++ )
    {
       x = ((double)col - (double)total_cols/2.0)/(double)total_cols;
       y = ((double)row - (double)total_rows/2.0)/(double)total_rows;
       data[ row * total_cols + col ] = 
                                     (float)(1000.0 * cos( (x*x + y*y)*20.0 ));
    }
                                                // mark a row 1/4 way up
  double point = (total_ymax - total_ymin)/4 + total_ymin;
  double mark_row = 0;
  IVUtils::Interpolate( total_ymin, total_ymax, point,
                               0.0, (double)total_rows, mark_row );

  size_t row_offset = (int)(mark_row) * total_cols;
  for ( size_t col = 0; col < total_cols; col++ )
  {
     data[ row_offset-total_cols + col ] = 0;
     data[ row_offset            + col ] = 0;
     data[ row_offset+total_cols + col ] = 0;
  }
                                                 // mark a col 1/10 way over
  point = (total_xmax - total_xmin)/10 + total_xmin;
  double mark_col = 0;
  IVUtils::Interpolate( total_xmin, total_xmax, point,
                               0.0, (double)total_cols, mark_col );

  size_t col_offset = (int)( mark_col );
  for ( size_t row = 0; row < total_rows; row++ )
  {
     data[ row * total_cols + col_offset-1 ] = 0;
     data[ row * total_cols + col_offset   ] = 0;
     data[ row * total_cols + col_offset+1 ] = 0;
  }

  return data;
}


int main( int argc, char **argv )
{
  QApplication a( argc, argv );

  float * data = MakeTestData( 10, 110, 220, 320, 2000, 2000 );

  ArrayDataSource* source = 
                   new ArrayDataSource( 10, 110, 220, 320, 2000, 2000, data );

  MantidQt::SpectrumView::SpectrumView spectrum_view( source );

                       // Don't delete on close in this case, since image_view
                       // will be deleted when the application ends
  spectrum_view.setAttribute(Qt::WA_DeleteOnClose,false);

  return a.exec();
}

