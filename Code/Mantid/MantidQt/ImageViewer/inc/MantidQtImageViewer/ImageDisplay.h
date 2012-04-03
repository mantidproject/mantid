#ifndef  IMAGE_DISPLAY_H
#define  IMAGE_DISPLAY_H

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QTableWidget>
#include <qwt_plot.h>

#include "ImageDataSource.h"
#include "GraphDisplay.h"
#include "ImagePlotItem.h"
#include "SliderHandler.h"

/**
    @class ImageDisplay 
  
       This class provides the image display and coordinates the image and
    graph displays for the ImageView data viewer.
 
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


class ImageDisplay
{
  public:
     ImageDisplay( QwtPlot*       image_plot, 
                   SliderHandler* slider_handler,
                   GraphDisplay*  h_graph,
                   GraphDisplay*  v_graph,
                   QTableWidget*  table_widget );

     ~ImageDisplay();

     void SetDataSource( ImageDataSource* data_source );

     /// Rebuild image from data source, due to resize or scroll bar
     void UpdateImage();
   
     void SetPointedAtPoint( QPoint point );

     void GetDisplayRectangle( QRect &rect );

  private:
     void ShowInfoList( double x, double y );

     std::vector<QRgb> color_table;

     ImageDataSource* data_source;
     DataArray*       data_array;

     QwtPlot*         image_plot;
     ImagePlotItem*   image_plot_item;

     SliderHandler*   slider_handler;

     GraphDisplay*    h_graph_display;
     GraphDisplay*    v_graph_display;

     QTableWidget*    image_table;
};

} // namespace MantidQt 
} // namespace ImageView 


#endif   // IMAGE_DISPLAY_H
