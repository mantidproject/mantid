#ifndef DATA_ARRAY_H
#define DATA_ARRAY_H

#include <cstddef>
#include <vector>

#include <boost/shared_ptr.hpp>

#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class DataArray

       This class provides a simple immutable wrapper around a block of data
    returned from an SpectrumDataSource.

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

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER DataArray
{
  public:

    /// Construct a DataArray "wrapper" around the data and region info
    DataArray( double xMin,    double xMax,
               double yMin,    double yMax,
               bool   isLogX,
               size_t nRows,   size_t nCols,
               std::vector<float> data );

    ~DataArray();

    /// Get the smallest 'x' value actually covered by this DataArray
    double getXMin() const;

    /// Get the largest 'x' value actually covered by this DataArray
    double getXMax() const;

    /// Get the smallest 'y' value actually covered by this DataArray
    double getYMin() const;

    /// Get the largest 'y' value actually covered by this DataArray
    double getYMax() const;

    /// Check if the returned array is binned logarithmically in 'x'
    bool isLogX() const;

    /// Get smallest value recorded in this DataArray
    double getDataMin() const;

    /// Get largest value recorded in this DataArray
    double getDataMax() const;

    // Get the actual number of rows in this DataArray
    size_t getNRows() const;

    /// Get the actual number of columns in this DataArray
    size_t getNCols() const;

    /// Get vector containing all values, packed in a 1-D array
    std::vector<float> getData() const;

    /// Get the value at the specified row and column
    double getValue( int row, int col ) const;

    /// Get the value from the row and column containing the specified point
    double getValue( double x, double y ) const;

    /// Clamp x to the interval of x-values covered by this DataArray
    void restrictX( double & x ) const;

    /// Clamp y to the interval of y-values covered by this DataArray
    void restrictY( double & y ) const;

    /// Clamp row to a valid row number for this DataArray
    void restrictRow( int & row ) const;

    /// Clamp col to a valid column number for this DataArray
    void restrictCol( int & col ) const;

    /// Calculate the column number containing the specified x
    size_t columnOfX( double x ) const;

    /// Calculate the x-value at the center of the specified column
    double xOfColumn( size_t col ) const;

    /// Calculate the row number containing the specified y
    size_t rowOfY( double y ) const;

    /// Calculate the y-value at the center of the specified row
    double yOfRow( size_t row ) const;


  private:
    double m_xMin;
    double m_xMax;
    double m_yMin;
    double m_yMax;
    bool   m_isLogX;
    size_t m_nRows;
    size_t m_nCols;

    double m_dataMin;
    double m_dataMax;

    std::vector<float> m_data;

};

typedef boost::shared_ptr<DataArray> DataArray_sptr;
typedef boost::shared_ptr<const DataArray> DataArray_const_sptr;

} // namespace SpectrumView
} // namespace MantidQt

#endif // DATA_ARRAY_H
