#ifndef MATRIX_WS_DATA_SOURCE_H
#define MATRIX_WS_DATA_SOURCE_H

#include <cstddef>

#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

#include "MantidAPI/MatrixWorkspace.h"

/**
    @class MatrixWSDataSource

    This class provides a concrete implementation of an SpectrumDataSource
    that gets it's data from a matrix workspace.

    @author Dennis Mikkelson
    @date   2012-05-08

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
class EModeHandler;

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER MatrixWSDataSource: public SpectrumDataSource
{
  public:

    /// Construct a DataSource object around the specifed MatrixWorkspace
    MatrixWSDataSource( Mantid::API::MatrixWorkspace_const_sptr matWs );

    ~MatrixWSDataSource();

    virtual bool hasData(const std::string& wsName, const boost::shared_ptr<Mantid::API::Workspace> ws);

    /// OVERRIDES: Get the smallest 'x' value covered by the data
    virtual double getXMin();

    /// OVERRIDES: Get the largest 'x' value covered by the data
    virtual double getXMax();

    /// OVERRIDES: Get the largest 'y' value covered by the data
    virtual double getYMax();

    /// OVERRIDES: Get the total number of rows of data
    virtual size_t getNRows();

    /// Get DataArray covering full range of data in x, and y directions
    DataArray_const_sptr getDataArray( bool isLogX );

    /// Get DataArray covering restricted range of data
    DataArray_const_sptr getDataArray( double  xMin,
                                       double  xMax,
                                       double  yMin,
                                       double  yMax,
                                       size_t  nRows,
                                       size_t  nCols,
                                       bool    isLogX );

    /// Set the class that gets the emode & efixed info from the user.
    void setEModeHandler( EModeHandler* emodeHandler );

    /// Get a list containing pairs of strings with information about x,y
    void getInfoList( double x,
                      double y,
                      std::vector<std::string> &list );


  private:
    Mantid::API::MatrixWorkspace_const_sptr m_matWs;
    EModeHandler* m_emodeHandler;

};

typedef boost::shared_ptr<MatrixWSDataSource> MatrixWSDataSource_sptr;
typedef boost::shared_ptr<const MatrixWSDataSource> MatrixWSDataSource_const_sptr;

} // namespace SpectrumView
} // namespace MantidQt

#endif // MATRIX_WS_DATA_SOURCE_H
