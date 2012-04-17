#ifndef SLIDER_HANDLER_H
#define SLIDER_HANDLER_H

#include <QRect>

#include "ui_ImageView.h"
#include "MantidQtImageViewer/ImageDataSource.h"
#include "MantidQtImageViewer/DllOptionIV.h"

/**
    @class SliderHandler 
  
      This manages the horizontal and vertical scroll bars for the
    ImageView data viewer. 
 
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


class EXPORT_OPT_MANTIDQT_IMAGEVIEWER SliderHandler 
{
  public:

    /// Construct object to manage image sliders from the specified UI
    SliderHandler( Ui_MainWindow* iv_ui );

    /// Configure the image sliders for the specified data and drawing area
    void ConfigureSliders( QRect            draw_area, 
                           ImageDataSource* data_source );

    /// Return true if the image horizontal slider is enabled.
    bool HSliderOn();

    /// Return true if the image vertical slider is enabled.
    bool VSliderOn();

    /// Get the range of columns to display in the image.
    void GetHSliderInterval( int &x_min, int &x_max );

    /// Get the range of rows to display in the image.
    void GetVSliderInterval( int &y_min, int &y_max );

  private:
    Ui_MainWindow*   iv_ui;
};

} // namespace MantidQt 
} // namespace ImageView

#endif // SLIDER_HANDLER_H
