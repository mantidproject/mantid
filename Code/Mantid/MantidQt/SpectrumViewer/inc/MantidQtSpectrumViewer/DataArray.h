#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include <cstddef>

#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class DataArray 
  
       This class provides a simple immutable wrapper around a block of data
    returned from an ImageDataSource.
 
    @author Dennis Mikkelson 
    @date   2012-04-03 
     
    Copyright © 2012 ORNL, STFC Rutherford Appleton Laboratories
  
    This file is part of Mantid.

    Mantid is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Mantid is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Code Documentation is available at 
                 <http://doxygen.mantidproject.org>
 */

namespace MantidQt
{
namespace ImageView
{

class EXPORT_OPT_MANTIDQT_IMAGEVIEWER DataArray
{
  public:

    /// Construct a DataArray "wrapper" around the data and region info
    DataArray( double xmin,     double xmax,
               double ymin,     double ymax,
               bool   is_log_x,
               size_t n_rows,   size_t n_cols,
               float *data );

    ~DataArray();

    /// Get the smallest 'x' value actually covered by this DataArray
    double GetXMin()    const;

    /// Get the largest 'x' value actually covered by this DataArray
    double GetXMax()    const;

    /// Get the smallest 'y' value actually covered by this DataArray
    double GetYMin()    const;

    /// Get the largest 'y' value actually covered by this DataArray
    double GetYMax()    const;

    /// Check if the returned array is binned logarithmically in 'x'
    bool IsLogX()  const;

    /// Get smallest value recorded in this DataArray
    double GetDataMin() const;

    /// Get largest value recorded in this DataArray
    double GetDataMax() const;

    // Get the actual number of rows in this DataArray
    size_t GetNRows()   const;

    /// Get the actual number of columns in this DataArray
    size_t GetNCols()   const;

    /// Get simple array containing all values, packed in a 1-D array
    float* GetData()    const;

    /// Get the value at the specified row and column
    double GetValue( int row, int col ) const;

    /// Get the value from the row and column containing the specified point
    double GetValue( double x, double y ) const;

    /// Clamp x to the interval of x-values covered by this DataArray
    void RestrictX( double & x ) const;
  
    /// Clamp y to the interval of y-values covered by this DataArray
    void RestrictY( double & y ) const;

    /// Clamp row to a valid row number for this DataArray
    void RestrictRow( int & row ) const;

    /// Clamp col to a valid column number for this DataArray
    void RestrictCol( int & col ) const;

    /// Calculate the column number containing the specified x
    size_t ColumnOfX( double x ) const;

    /// Calculate the x-value at the center of the specified column
    double XOfColumn( size_t col ) const;

    /// Calculate the row number containing the specified y
    size_t RowOfY( double y ) const;

    /// Calculate the y-value at the center of the specified row 
    double YOfRow( size_t row ) const;


  private:
    double xmin; 
    double xmax;
    double ymin;
    double ymax;
    bool   is_log_x;
    double data_min; 
    double data_max;
    size_t n_rows;   
    size_t n_cols;
    float *data;        // This is given a reference to the data block,
                        // which is allocated in the ImageDataSource,
                        // but will be deleted in this object's Destructor
};

} // namespace MantidQt 
} // namespace ImageView 

#endif // DATA_ARRAY_H
