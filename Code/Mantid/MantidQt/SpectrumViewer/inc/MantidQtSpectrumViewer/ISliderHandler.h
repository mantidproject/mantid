#ifndef ISLIDER_HANDLER_H
#define ISLIDER_HANDLER_H

#include "MantidQtSpectrumViewer/DllOptionIV.h"
#include <QRect>
#include "MantidQtSpectrumViewer/ImageDataSource.h"

namespace MantidQt
{
namespace SpectrumView
{
/** An interface to the SliderHandler, which manages the horizontal and vertical
    scroll bars for the ImageView data viewer.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory & NScD Oak Ridge National Laboratory

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

    Code Documentation is available at <http://doxygen.mantidproject.org>
 */
class EXPORT_OPT_MANTIDQT_IMAGEVIEWER ISliderHandler
{
public:
  /// Construct object to manage image scrollbars from the specified UI
  ISliderHandler() {}
  virtual ~ISliderHandler() {}

  /// Configure the image scrollbars for the specified data and drawing area
  virtual void ConfigureSliders( QRect            draw_area,
                                 ImageDataSource* data_source ) = 0;
  /// Configure the horizontal scrollbar to cover the specified range
  virtual void ConfigureHSlider( int         n_data_steps,
                         int         n_pixels ) = 0;
  /// Return true if the image horizontal scrollbar is enabled.
  virtual bool HSliderOn() = 0;
  /// Return true if the image vertical scrollbar is enabled.
  virtual bool VSliderOn() = 0;
  /// Get the range of columns to display in the image.
  virtual void GetHSliderInterval( int &x_min, int &x_max ) = 0;
  /// Get the range of rows to display in the image.
  virtual void GetVSliderInterval( int &y_min, int &y_max ) = 0;
};

} // namespace SpectrumView
} // namespace MantidQt 

#endif // ISLIDER_HANDLER_H
