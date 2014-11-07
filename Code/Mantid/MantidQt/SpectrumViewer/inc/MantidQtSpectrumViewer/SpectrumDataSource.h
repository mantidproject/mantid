#ifndef SPECTRUM_DATA_SOURCE_H
#define SPECTRUM_DATA_SOURCE_H

#include <boost/shared_ptr.hpp>
#include <cstddef>
#include <vector>
#include <string>
#include "MantidAPI/Workspace.h"
#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class SpectrumDataSource

    This class is an abstract base class for classes that can provide
    data to be displayed in an SpectrumView data viewer.

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

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumDataSource
{
  public:

    /// Construct data source with specified total range and data size
    SpectrumDataSource( double totalXmin, double totalXmax,
                        double totalYmin, double totalYmax,
                        size_t totalRows, size_t totalCols );

    virtual ~SpectrumDataSource();

    virtual bool hasData(const std::string& wsName,
                         const boost::shared_ptr<Mantid::API::Workspace> ws) = 0;

    /// Get the smallest 'x' value covered by the data
    virtual double getXMin();

    /// Get the largest 'x' value covered by the data
    virtual double getXMax();

    /// Get the smallest 'y' value covered by the data
    virtual double getYMin();

    /// Get the largest 'y' value covered by the data
    virtual double getYMax();

    /// Get the total number of rows of data
    virtual size_t getNRows();

    /// Get the total number of columns of data
    virtual size_t getNCols();

    /// Clamp x to the interval of x-values covered by this DataSource
    virtual void restrictX( double & x );

    /// Clamp y to the interval of y-values covered by this DataSource
    virtual void restrictY( double & y );

    /// Clamp row to a valid row number for this DataSource
    virtual void restrictRow( int & row );

    /// Clamp col to a valid column number for this dataSource
    virtual void restrictCol( int & col );

    /// Get a DataArray roughly spaning the specified rectangle.  NOTE: The
    /// actual size and number of steps returned in the DataArray will be
    /// adjusted to match the underlying data.
    virtual DataArray_const_sptr getDataArray( double  xMin,
                                               double  xMax,
                                               double  yMin,
                                               double  yMax,
                                               size_t  nRows,
                                               size_t  nCols,
                                               bool    isLogX ) = 0;

    /// Convenience method to get data covering the full range at max resolution
    virtual DataArray_const_sptr getDataArray( bool is_log_x );

    /// Get list of pairs of strings with info about the data at location x, y
    virtual void getInfoList( double x,
                              double y,
                              std::vector<std::string> &list ) = 0;

  protected:
    double m_totalXMin;
    double m_totalXMax;
    double m_totalYMin;
    double m_totalYMax;
    size_t m_totalRows;
    size_t m_totalCols;

};

typedef boost::shared_ptr<SpectrumDataSource> SpectrumDataSource_sptr;
typedef boost::shared_ptr<const SpectrumDataSource> SpectrumDataSource_const_sptr;

} // namespace SpectrumView
} // namespace MantidQt

#endif // SPECTRUM_DATA_SOURCE_H
