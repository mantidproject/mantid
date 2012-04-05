#ifndef EVENT_WS_DATA_SOURCE_H
#define EVENT_WS_DATA_SOURCE_H

#include <cstddef>

#include "MantidQtImageViewer/DataArray.h"
#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtImageViewer/DllOptionIV.h"

#include "MantidDataObjects/EventWorkspace.h"

/**
    @class EventWSDataSource 
  
       This class provides a concrete implementation of an ImageDataSource
    that gets it's data from an event workspace.
 
    @author Dennis Mikkelson 
    @date   2012-04-04 
     
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

using namespace Mantid;
using namespace DataObjects;
using namespace Kernel;

namespace MantidQt
{
namespace ImageView
{


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER EventWSDataSource: public ImageDataSource
{
  public:

    EventWSDataSource( EventWorkspace_sptr ev_ws );

   ~EventWSDataSource();

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

    void GetInfoList( double x,
                      double y,
                      std::vector<std::string> &list );
  private:
    EventWorkspace_sptr  ev_ws;
    float*               new_data;
    DataArray*           new_data_array;
    MantidVec*           x_scale;
};

} // namespace MantidQt 
} // namespace ImageView 

#endif // EVENT_WS_DATA_SOURCE_H
