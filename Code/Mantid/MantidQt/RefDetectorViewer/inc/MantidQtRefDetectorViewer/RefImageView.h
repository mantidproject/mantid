#ifndef  REF_IMAGE_VIEW_H
#define  REF_IMAGE_VIEW_H

#include <QMainWindow>
#include <QtGui>

#include "MantidQtSpectrumViewer/GraphDisplay.h"
#include "MantidQtSpectrumViewer/ImageDataSource.h"
#include "DllOption.h"

/**
    @class RefImageView 
  
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
namespace RefDetectorViewer
{

  class RefIVConnections;
class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImageView : public QMainWindow
{
  public:

     /// Construct an ImageView to display data from the specified data source 
     RefImageView( ImageView::ImageDataSource* data_source, int peak_min, int peak_max, int back_min, int back_max, int tof_min, int tof_max);

    ~RefImageView();    
    
    RefIVConnections* getIVConnections();

  private:
    ImageView::GraphDisplay*    h_graph;
    ImageView::GraphDisplay*    v_graph;
                                 
    // keep void pointers to the following objects, to avoid having to 
    // include ui_ImageView.h, which disappears by the time MantidPlot is
    // being built.  We need the pointers so we can delete them in the 
    // destructor.  
    void*            saved_ui;               // Ui_RefImageViewer*
    void*            saved_slider_handler;   // SliderHandler*
    void*            saved_range_handler;    // RangeHandler*
    void*            saved_image_display;    // RefImageDisplay*
    //    void*            saved_iv_connections;   // IVConnections*
    RefIVConnections*            saved_iv_connections;   // IVConnections*
};

} // namespace MantidQt 
} // namespace ImageView 

#endif   // REF_IMAGE_VIEW_H
