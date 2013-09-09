#ifndef IV_CONNECTIONS_H
#define IV_CONNECTIONS_H

#include <QtCore/QtCore>
#include <QtGui/QWidget>
#include <QActionGroup>

#include "ui_ImageView.h"
#include "MantidQtImageViewer/ImageView.h"
#include "MantidQtImageViewer/TrackingPicker.h"
#include "MantidQtImageViewer/ImageDisplay.h"
#include "MantidQtImageViewer/GraphDisplay.h"
#include "MantidQtImageViewer/DllOptionIV.h"


/**
    @class IVConnections 
  
       This class provides the connections between the ImageView GUI components
    made using QtDesigner and the classes that do the actual work for the
    ImageView.  It basically provides SLOTS that are called by the GUI 
   components' SIGNALS and in turn call methods on the ImageView 
   implementation objects.
 
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


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER IVConnections: public QWidget
{
  Q_OBJECT

public:

  /// Construct the object that links the GUI components to the other specifed
  /// higher level objects.
  IVConnections( Ui_ImageViewer* ui, 
                 ImageView*     image_view,
                 ImageDisplay*  image_display,
                 GraphDisplay*  h_graph_display,
                 GraphDisplay*  v_graph_display );

  ~IVConnections();

  /// Set the pix map that shows the color scale from the specified color maps
  void ShowColorScale( std::vector<QRgb> & positive_color_table,
                       std::vector<QRgb> & negative_color_table );

public slots:
  void close_viewer();
  void toggle_Hscroll();
  void toggle_Vscroll();
  void image_horizontal_range_changed();
  void graph_range_changed();
  void v_scroll_bar_moved();
  void h_scroll_bar_moved();
  void imageSplitter_moved();
  void imagePicker_moved();
  void h_graphPicker_moved();
  void v_graphPicker_moved();
  void intensity_slider_moved();
  void heat_color_scale();
  void gray_color_scale();
  void negative_gray_color_scale();
  void green_yellow_color_scale();
  void rainbow_color_scale();
  void optimal_color_scale();
  void multi_color_scale();
  void spectrum_color_scale();
  void load_color_map();
  void online_help_slot();
 
private:

  Ui_ImageViewer*  iv_ui;
  ImageView*       iv_main_window;
  ImageDisplay*    image_display;
  GraphDisplay*    h_graph_display;
  GraphDisplay*    v_graph_display;
  TrackingPicker*  image_picker;
  TrackingPicker*  h_graph_picker;
  TrackingPicker*  v_graph_picker;
  QActionGroup*    color_group;

};

} // namespace MantidQt 
} // namespace ImageView 


#endif  // IV_CONNECTIONS_H
