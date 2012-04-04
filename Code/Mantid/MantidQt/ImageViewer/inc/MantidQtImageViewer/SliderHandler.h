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

    SliderHandler( Ui_MainWindow* iv_ui );

    void ConfigureSliders( QRect            draw_area, 
                           ImageDataSource* data_source );

    bool VSliderOn();

    bool HSliderOn();

    // NOTE: x_min will be the smaller column number in the array, corresponding
    //       to lower values on the calibrated x-scale
    void GetHSliderInterval( int &x_min, int &x_max );

    // NOTE: y_min will be the smaller row number in the array, corresponding
    //       to lower values on the calibrated y-scale
    void GetVSliderInterval( int &y_min, int &y_max );

  private:
    Ui_MainWindow*   iv_ui;
};

} // namespace MantidQt 
} // namespace ImageView

#endif // SLIDER_HANDLER_H
