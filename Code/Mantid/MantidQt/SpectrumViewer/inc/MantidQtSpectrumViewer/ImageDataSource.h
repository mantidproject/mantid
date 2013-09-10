#ifndef IMAGE_DATA_SOURCE_H
#define IMAGE_DATA_SOURCE_H

#include <cstddef>
#include <vector>
#include <string>
#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class ImageDataSource 
  
       This class is an abstract base class for classes that can provide 
    data to be displayed in an ImageView data viewer.
 
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
namespace SpectrumView
{


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER ImageDataSource
{
  public:

    /// construct data source with specified total range and data size
    ImageDataSource( double total_xmin, double total_xmax,
                     double total_ymin, double total_ymax,
                     size_t total_rows, size_t total_cols );

    virtual ~ImageDataSource();

    /// Get the smallest 'x' value covered by the data
    virtual double GetXMin();

    /// Get the largest 'x' value covered by the data
    virtual double GetXMax();

    /// Get the smallest 'y' value covered by the data
    virtual double GetYMin();

    /// Get the largest 'y' value covered by the data
    virtual double GetYMax();

    /// Get the total number of rows of data
    virtual size_t GetNRows();

    /// Get the total number of columns of data
    virtual size_t GetNCols();

    /// Clamp x to the interval of x-values covered by this DataSource
    virtual void RestrictX( double & x );

    /// Clamp y to the interval of y-values covered by this DataSource
    virtual void RestrictY( double & y );

    /// Clamp row to a valid row number for this DataSource
    virtual void RestrictRow( int & row );

    /// Clamp col to a valid column number for this dataSource
    virtual void RestrictCol( int & col );

    /// Get a DataArray roughly spaning the specified rectangle.  NOTE: The
    /// actual size and number of steps returned in the DataArray will be  
    /// adjusted to match the underlying data.
    virtual DataArray* GetDataArray( double  xmin,
                                     double  xmax,
                                     double  ymin,
                                     double  ymax,
                                     size_t  n_rows,
                                     size_t  n_cols,
                                     bool    is_log_x ) = 0;

    /// Convenience method to get data covering the full range at max resolution
    virtual DataArray* GetDataArray( bool is_log_x );

    /// Get list of pairs of strings with info about the data at location x, y
    virtual void GetInfoList( double x, 
                              double y,
                              std::vector<std::string> &list ) = 0;
  protected:
    double total_xmin;
    double total_xmax;
    double total_ymin;
    double total_ymax;
    size_t total_rows;
    size_t total_cols;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif // IMAGE_DATA_SOURCE_H
