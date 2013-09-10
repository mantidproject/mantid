#ifndef  IMAGE_VIEW_H
#define  IMAGE_VIEW_H

#include <QMainWindow>
#include <QtGui>

#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/ImageDataSource.h"
#include "MantidQtSpectrumViewer/DllOptionIV.h"

/**
    @class ImageView 
  
      This is the QMainWindow for the ImageView data viewer.  Data is
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


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER ImageView : public QMainWindow
{
  public:

     /// Construct an ImageView to display data from the specified data source 
     ImageView( ImageDataSource* data_source );

    ~ImageView();

  private:
    GraphDisplay*    h_graph;
    GraphDisplay*    v_graph;
                                 
    // keep void pointers to the following objects, to avoid having to 
    // include ui_ImageView.h, which disappears by the time MantidPlot is
    // being built.  We need the pointers so we can delete them in the 
    // destructor.  
    void*            saved_ui;               // Ui_ImageViewer*
    void*            saved_slider_handler;   // SliderHandler*
    void*            saved_range_handler;    // RangeHandler*
    void*            saved_image_display;    // ImageDisplay*
    void*            saved_iv_connections;   // IVConnections*
    void*            saved_emode_handler;    // EModeHandler*
};

} // namespace MantidQt 
} // namespace ImageView 

#endif   // IMAGE_VIEW_H
