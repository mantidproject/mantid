#ifndef REF_SLIDER_HANDLER_H
#define REF_SLIDER_HANDLER_H

#include "MantidQtSpectrumViewer/ISliderHandler.h"
#include <QRect>

#include "ui_RefImageView.h"
#include "MantidQtSpectrumViewer/SpectrumDataSource.h"
#include "MantidQtRefDetectorViewer/DllOption.h"

/**
    @class SliderHandler

    This manages the horizontal and vertical scroll bars for the
    SpectrumView data viewer.

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


class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefSliderHandler : public SpectrumView::ISliderHandler
{
  public:
    /// Construct object to manage image scrollbars from the specified UI
    RefSliderHandler( Ui_RefImageViewer* ivUI );

    /// Configure the image scrollbars for the specified data and drawing area
    void configureSliders( QRect drawArea,
                           SpectrumView::SpectrumDataSource_sptr dataSource );

    /// Configure the horizontal scrollbar to cover the specified range
    void configureHSlider( int nDataSteps,
                           int nPixels );

    /// Return true if the image horizontal scrollbar is enabled.
    bool hSliderOn();

    /// Return true if the image vertical scrollbar is enabled.
    bool vSliderOn();

    /// Get the range of columns to display in the image.
    void getHSliderInterval( int &xMin, int &xMax );

    /// Get the range of rows to display in the image.
    void getVSliderInterval( int &yMin, int &yMax );

  private:
    /// Configure the specified scrollbar to cover the specified range
    void configureSlider( QScrollBar* scrollBar,
                          int         nDataSteps,
                          int         nPixels,
                          int         val );

    Ui_RefImageViewer*   m_ivUI;

};

} // namespace RefDetectorViewer
} // namespace MantidQt

#endif // REF_SLIDER_HANDLER_H
