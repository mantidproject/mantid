/**
 *  File: MatrixWSDataSource.cpp
 */

#include <iostream>
#include <math.h>

#include <QThread>

#include "MantidQtImageViewer/MatrixWSDataSource.h"
#include "MantidQtImageViewer/IVUtils.h"
#include "MantidAPI/ISpectrum.h"
// #include "MantidAPI/IEventList.h"

using namespace Mantid;
using namespace Kernel;
using namespace API;

namespace MantidQt
{
namespace ImageView
{

/**
 * Construct a DataSource object around the specifed MatrixWorkspace.
 *
 * @param mat_ws  Shared pointer to the matrix workspace being "wrapped"
 */
MatrixWSDataSource::MatrixWSDataSource( MatrixWorkspace_sptr mat_ws )
                 :ImageDataSource( 0.0, 1.0, 0.0, 1.0, 0, 0 )  // some defaults
{
  this->mat_ws = mat_ws;

  total_xmin = mat_ws->getXMin(); 
  total_xmax = mat_ws->getXMax(); 

  total_ymin = 0;                 // y direction is spectrum index
  total_ymax = (double)mat_ws->getNumberHistograms();

  total_rows = mat_ws->getNumberHistograms();

  total_cols = 1000000;              // Default data resolution
}


MatrixWSDataSource::~MatrixWSDataSource()
{
}


/**
 * Get the smallest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetXMin()
{
  total_xmin = mat_ws->getXMin(); 
  return total_xmin;

}


/**
 * Get the largest 'x' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetXMax()
{
  total_xmax = mat_ws->getXMax(); 
  return total_xmax;
}


/**
 * Get the largest 'y' value covered by the data.  Must override base class
 * method, since the DataSource can be changed!
 */
double MatrixWSDataSource::GetYMax()
{
  total_ymax = (double)mat_ws->getNumberHistograms();
  return total_ymax;
}


/**
 * Get the total number of rows the data is divided into.  Must override base
 * class method, since the DataSource can be changed!
 */
size_t MatrixWSDataSource::GetNRows()
{
  total_ymax = (double)mat_ws->getNumberHistograms();
  return total_rows;
}


/**
 * Get a data array covering the specified range of data, at the specified
 * resolution.  NOTE: The calling code is responsible for deleting the 
 * DataArray that is constructed in and returned by this method.
 *
 * @param xmin      Left edge of region to be covered.
 * @param xmax      Right edge of region to be covered.
 * @param ymin      Bottom edge of region to be covered.
 * @param ymax      Top edge of region to be covered.
 * @param n_rows    Number of rows to return. If the number of rows is less
 *                  than the actual number of data rows in [ymin,ymax], the 
 *                  data will be subsampled, and only the specified number 
 *                  of rows will be returned.
 * @param n_cols    The specrum data will be rebinned using the specified
 *                  number of colums.
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.  (NOT USED YET)
 */
DataArray* MatrixWSDataSource::GetDataArray( double xmin,   double  xmax,
                                             double ymin,   double  ymax,
                                             size_t n_rows, size_t  n_cols,
                                             bool   is_log_x )
{
/*
  std::cout << "Start MatrixWSDataSource::GetDataArray " << std::endl;
  std::cout << "  xmin   = " << xmin 
            << "  xmax   = " << xmax 
            << "  ymin   = " << ymin 
            << "  ymax   = " << ymax 
            << "  n_rows = " << n_rows
            << "  n_cols = " << n_cols << std::endl;
*/
  size_t first_col;
  IVUtils::CalculateInterval( total_xmin, total_xmax, total_cols,
                              first_col, xmin, xmax, n_cols );

  size_t first_row;
  IVUtils::CalculateInterval( total_ymin, total_ymax, total_rows,
                              first_row, ymin, ymax, n_rows );

  float* new_data = new float[n_rows * n_cols];   // this is deleted in the
                                                  // DataArrray destructor
  MantidVec x_scale;
  x_scale.resize(n_cols+1);
  double dx = (xmax - xmin)/((double)n_cols + 1.0);
  for ( size_t i = 0; i < n_cols+1; i++ )
  {
    x_scale[i] = xmin + (double)i * dx;;
  }
                                                   // choose spectra from  
                                                   // required range of 
                                                   // spectrum indexes 
  double y_step = (ymax - ymin) / (double)n_rows;
  double mid_y;
  double d_y_index;
  size_t source_row;

  MantidVec y_vals;
  MantidVec err;
  y_vals.resize(n_cols);
  err.resize(n_cols);
  size_t index = 0;
  for ( size_t i = 0; i < n_rows; i++ )
  {
    mid_y = ymin + ((double)i + 0.5) * y_step;
    IVUtils::Interpolate( total_ymin, total_ymax, mid_y,
                                 0.0, (double)total_rows, d_y_index );
    source_row = (size_t)d_y_index;

    y_vals.clear();

    mat_ws->generateHistogram( source_row, x_scale, y_vals, err, true );
    for ( size_t col = 0; col < n_cols; col++ )
    {
      new_data[index] = (float)y_vals[col];
      index++;
    }
  }
                                // The calling code is responsible for deleting 
                                // the DataArray when it is done with it      
  DataArray* new_data_array = new DataArray( xmin, xmax, ymin, ymax,
                                           is_log_x, n_rows, n_cols, new_data);
  return new_data_array;
}


/**
 * Get a data array covering the full range of data.
 *
 * @param is_log_x  Flag indicating whether or not the data should be
 *                  binned logarithmically.  (NOT USED YET)
 */
DataArray * MatrixWSDataSource::GetDataArray( bool is_log_x )
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
void MatrixWSDataSource::GetInfoList( double x, 
                                      double y,
                                      std::vector<std::string> &list )
{
  list.clear();

  int row = (int)y;
  RestrictRow( row );

  ISpectrum* spec = mat_ws->getSpectrum( row );

  double spec_num = spec->getSpectrumNo();
  IVUtils::PushNameValue( "Spec Num", 8, 0, spec_num, list );

  std::string x_label = mat_ws->getAxis(0)->unit()->label();
  IVUtils::PushNameValue( x_label, 8, 3, x, list );

  std::set<detid_t> ids = spec->getDetectorIDs();
  if ( !ids.empty() )
  {
    std::set<detid_t>::iterator it = ids.begin();
    double d_id = (double)*it;
    IVUtils::PushNameValue( "Det ID", 8, 0, d_id, list );
  }
}


} // namespace MantidQt 
} // namespace ImageView
