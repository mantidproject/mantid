#ifndef ISLIDER_HANDLER_H
#define ISLIDER_HANDLER_H

#include "MantidQtSpectrumViewer/DllOptionSV.h"
#include <QRect>
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"

namespace MantidQt
{
namespace SpectrumView
{

/** An interface to the SliderHandler, which manages the horizontal and vertical
    scroll bars for the SpectrumView data viewer.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER ISliderHandler
{

public:
  /// Construct object to manage image scrollbars from the specified UI
  ISliderHandler() {}
  virtual ~ISliderHandler() {}

  /// Configure the image scrollbars for the specified data and drawing area
  virtual void configureSliders( QRect drawArea,
                                 SpectrumDataSource_sptr dataSource ) = 0;

  /// Configure the horizontal scrollbar to cover the specified range
  virtual void configureHSlider( int         nDataSteps,
                                 int         nPixels ) = 0;

  /// Return true if the image horizontal scrollbar is enabled.
  virtual bool hSliderOn() = 0;

  /// Return true if the image vertical scrollbar is enabled.
  virtual bool vSliderOn() = 0;

  /// Get the range of columns to display in the image.
  virtual void getHSliderInterval( int &xMin, int &xMax ) = 0;

  /// Get the range of rows to display in the image.
  virtual void getVSliderInterval( int &yMin, int &yMax ) = 0;
};

} // namespace SpectrumView
} // namespace MantidQt

#endif // ISLIDER_HANDLER_H
