#ifndef  REF_IMAGE_DISPLAY_H
#define  REF_IMAGE_DISPLAY_H

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QTableWidget>
#include <qwt_plot.h>

#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtRefDetectorViewer/GraphDisplay.h"
#include "MantidQtRefDetectorViewer/RefImagePlotItem.h"
#include "MantidQtRefDetectorViewer/SliderHandler.h"
#include "MantidQtRefDetectorViewer/RangeHandler.h"
#include "MantidQtImageViewer/DllOptionIV.h"

/**
    @class RefImageDisplay 
  
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
namespace RefDetectorViewer
{


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER RefImageDisplay
{
  public:

     /// Make an ImageDisplay to display with the given widgets and controls 
     RefImageDisplay( QwtPlot*       image_plot, 
                   SliderHandler* slider_handler,
                   RangeHandler*  range_handler,
                   GraphDisplay*  h_graph,
                   GraphDisplay*  v_graph,
                   QTableWidget*  table_widget,
                   QRadioButton*  radioButtonPeakLeft,
                   QRadioButton* radioButtonPeakRight,
                   QRadioButton* radioButtonBackLeft,
                   QRadioButton* radioButtonBackRight,
                   QRadioButton* radioButtonTOFmin,
                   QRadioButton* radioButtonTOFmax,
                   QLineEdit* lineEditPeakLeft,
                   QLineEdit* lineEditPeakRight,
                   QLineEdit* lineEditBackLeft,
                   QLineEdit* lineEditBackRight,
                   QLineEdit* lineEditTOFmin,
                   QLineEdit* lineEditTOFmax);

     ~RefImageDisplay();

     /// Set the source of the image data and information for the table
     void SetDataSource( ImageView::ImageDataSource* data_source );

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
     /// default right click (mouseClick = 2)
     void SetPointedAtPoint( QPoint point, int mouseClick = 2 );

     /// Set horizontal graph wit data from the array at the specified y value
     void SetHGraph( double y );

     /// Set vertical graph with data from the array at the specified x value
     void SetVGraph( double x );
     
    /// get peak, back and tof values
    static int getPeakLeft();
    static int getPeakRight();
    static int getBackLeft();
    static int getBackRight();
    static int getTOFmin();
    static int getTOFmax();
    
    static void setPeakLeft(int value);
    static void setPeakRight(int value);
    static void setBackLeft(int value);
    static void setBackRight(int value);
    static void setTOFmin(int value);
    static void setTOFmax(int value);
    
private:
     /// Check if the DataSource has been changed under us
     bool DataSourceRangeChanged();

     /// Get the rectangle currently covered by the image in pixel coordinates
     void GetDisplayRectangle( QRect &rect );

     /// Show information about the point (x, y) on the image in the table
     void ShowInfoList( double x, double y );

     // Show information about the x and y values selected in peak/back/tof/left/right boxes
     void ShowPeakBackSelectionValue(double x, double y );

     std::vector<QRgb>    positive_color_table;
     std::vector<QRgb>    negative_color_table;
     std::vector<double>  intensity_table;

     ImageView::ImageDataSource*     data_source;
     ImageView::DataArray*           data_array;

     QwtPlot*             image_plot;
     RefImagePlotItem*       image_plot_item;

     SliderHandler*       slider_handler;
     RangeHandler*        range_handler;

     GraphDisplay*        h_graph_display;
     GraphDisplay*        v_graph_display;

     double               pointed_at_x;
     double               pointed_at_y;

     //to update peak and back left and right infos
     QRadioButton*        radioButtonPeakLeft;
     QRadioButton*        radioButtonPeakRight;
     QRadioButton*        radioButtonBackLeft;
     QRadioButton*        radioButtonBackRight;
    QRadioButton*        radioButtonTOFmin;
    QRadioButton*        radioButtonTOFmax;
     QLineEdit*           lineEditPeakLeft;
     QLineEdit*           lineEditPeakRight;
     QLineEdit*           lineEditBackLeft;
     QLineEdit*           lineEditBackRight;
    QLineEdit*           lineEditTOFmin;
    QLineEdit*           lineEditTOFmax;
    
    static int peakLeft;
    static int peakRight;
    static int backLeft;
    static int backRight;
    static int TOFmin;
    static int TOFmax;
    
    QTableWidget*        image_table;
                                           // save current total data range
                                           // so we can reset the data source
                                           // if we detect a change of range
     double               total_y_min;
     double               total_y_max;
     double               total_x_min;
     double               total_x_max;
};

} // namespace MantidQt 
} // namespace ImageView 

#endif   // REF_IMAGE_DISPLAY_H
