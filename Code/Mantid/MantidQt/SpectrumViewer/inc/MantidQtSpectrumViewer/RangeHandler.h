#ifndef RANGE_HANDLER_H
#define RANGE_HANDLER_H

#include "MantidQtSpectrumViewer/IRangeHandler.h"
#include "ui_SpectrumView.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class RangeHandler 
  
      This manages the min, max and step range controls for the SpectrumView 
    data viewer. 
 
    @author Dennis Mikkelson 
    @date   2012-04-25 
     
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


class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER RangeHandler : public IRangeHandler
{
  public:

    /// Construct object to manage min, max and step controls in the UI
    RangeHandler( Ui_SpectrumViewer* iv_ui );

    /// Configure min, max and step controls for the specified data source
    void ConfigureRangeControls( SpectrumDataSource* data_source );

    /// Get the range of data to display in the image, from GUI controls
    void GetRange( double &min, double &max, double &step );

    /// Set the values displayed in the GUI controls
    void SetRange( double min, double max, double step );

  private:
    Ui_SpectrumViewer* iv_ui;
    double         total_min_x;
    double         total_max_x;
    size_t         total_n_steps;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif // RANGE_HANDLER_H
