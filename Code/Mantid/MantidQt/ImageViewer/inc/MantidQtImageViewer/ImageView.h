#ifndef  IMAGE_VIEW_H
#define  IMAGE_VIEW_H

#include <QMainWindow>
#include <QtGui>

#include "ui_ImageView.h"
#include "IVConnections.h"
#include "GraphDisplay.h"
#include "ImageDisplay.h"
#include "ImageDataSource.h"
#include "SliderHandler.h"

/**
    @class ImageView 
  
      This is the top level class for the ImageView data viewer.  Data is
    displayed in an ImageView, by constructing the ImageView object and
    specifying a particular data source.
 
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


class ImageView 
{
  public:
     ImageView( ImageDataSource* data_source );
    ~ImageView();

  private:
    Ui_MainWindow*   ui;
    QMainWindow*     window;
    
    SliderHandler*   slider_handler;
    GraphDisplay*    h_graph;
    GraphDisplay*    v_graph;

    ImageDisplay*   image_display;

    IVConnections*  iv_connections;
};

} // namespace MantidQt 
} // namespace ImageView 

#endif   // IMAGE_VIEW_H
