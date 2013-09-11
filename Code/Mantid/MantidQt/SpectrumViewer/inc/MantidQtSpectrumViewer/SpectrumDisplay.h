#ifndef  SPECTRUM_DISPLAY_H
#define  SPECTRUM_DISPLAY_H

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QTableWidget>
#include <qwt_plot.h>

#include "MantidQtSpectrumViewer/ImageDataSource.h"
#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/SpectrumPlotItem.h"
#include "MantidQtSpectrumViewer/ISliderHandler.h"
#include "MantidQtSpectrumViewer/IRangeHandler.h"
#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class SpectrumDisplay 
  
    This class provides the image display and coordinates the image and
    graph displays for the SpectrumView data viewer.
 
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


class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumDisplay
{
  public:

     /// Make an SpectrumDisplay to display with the given widgets and controls 
     SpectrumDisplay( QwtPlot*       image_plot, 
                   ISliderHandler* slider_handler,
                   IRangeHandler*  range_handler,
                   GraphDisplay*  h_graph,
                   GraphDisplay*  v_graph,
                   QTableWidget*  table_widget );

     virtual ~SpectrumDisplay();

     /// Set some properties of the SpectrumPlotItem object
     void setupSpectrumPlotItem();

     /// Set the source of the image data and information for the table
     void SetDataSource( ImageDataSource* data_source );

     /// Rebuild the scroll bars and image due to change of xmin, xmax, step
     void UpdateRange();

     /// Rebuild image from data source, due to resize or scroll bar movement
     void UpdateImage();

     /// Change the color tables used to map intensity to color
     void SetColorScales( std::vector<QRgb> & positive_color_table,
                          std::vector<QRgb> & negative_color_table );

     /// Change the control parameter (0...100) used to brighten the image
     void SetIntensity( double control_parameter );
   
     /// Record the point that the user is currently pointing at with the mouse
     virtual QPair<double,double> SetPointedAtPoint( QPoint point, int mouseClick = 2 );

     /// Set horizontal graph wit data from the array at the specified y value
     void SetHGraph( double y );

     /// Set vertical graph with data from the array at the specified x value
     void SetVGraph( double x );

  protected:
     SpectrumPlotItem*       image_plot_item;

  private:
     /// Check if the DataSource has been changed under us
     bool DataSourceRangeChanged();

     /// Get the rectangle currently covered by the image in pixel coordinates
     void GetDisplayRectangle( QRect &rect );

     /// Show information about the point (x, y) on the image in the table
     void ShowInfoList( double x, double y );

     std::vector<QRgb>    positive_color_table;
     std::vector<QRgb>    negative_color_table;
     std::vector<double>  intensity_table;

     ImageDataSource*     data_source;
     DataArray*           data_array;

     QwtPlot*             image_plot;

     ISliderHandler*       slider_handler;
     IRangeHandler*        range_handler;

     GraphDisplay*        h_graph_display;
     GraphDisplay*        v_graph_display;

     double               pointed_at_x;
     double               pointed_at_y;

     QTableWidget*        image_table;
                                           // save current total data range
                                           // so we can reset the data source
                                           // if we detect a change of range
     double               total_y_min;
     double               total_y_max;
     double               total_x_min;
     double               total_x_max;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif   // SPECTRUM_DISPLAY_H
