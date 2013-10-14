#ifndef MATRIX_WS_DATA_SOURCE_H
#define MATRIX_WS_DATA_SOURCE_H

#include <cstddef>

#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/EModeHandler.h"
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

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER MatrixWSDataSource: public SpectrumDataSource
{
  public:

    /// Construct a DataSource object around the specifed MatrixWorkspace
    MatrixWSDataSource( Mantid::API::MatrixWorkspace_const_sptr mat_ws );

   ~MatrixWSDataSource();

    virtual bool hasData(const std::string& wsName, const boost::shared_ptr<Mantid::API::Workspace> ws);

    /// OVERRIDES: Get the smallest 'x' value covered by the data
    virtual double GetXMin();

    /// OVERRIDES: Get the largest 'x' value covered by the data
    virtual double GetXMax();

    /// OVERRIDES: Get the largest 'y' value covered by the data
    virtual double GetYMax();

    /// OVERRIDES: Get the total number of rows of data
    virtual size_t GetNRows();

    /// Get DataArray covering full range of data in x, and y directions
    DataArray * GetDataArray( bool is_log_x );

    /// Get DataArray covering restricted range of data 
    DataArray * GetDataArray( double  xmin,
                              double  xmax,
                              double  ymin,
                              double  ymax,
                              size_t  n_rows,
                              size_t  n_cols,
                              bool    is_log_x );

    /// Set the class that gets the emode & efixed info from the user.
    void SetEModeHandler( EModeHandler* emode_handler );

    /// Get a list containing pairs of strings with information about x,y
    void GetInfoList( double x,
                      double y,
                      std::vector<std::string> &list );


  private:
    Mantid::API::MatrixWorkspace_const_sptr  mat_ws;
    EModeHandler* saved_emode_handler;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif // MATRIX_WS_DATA_SOURCE_H
