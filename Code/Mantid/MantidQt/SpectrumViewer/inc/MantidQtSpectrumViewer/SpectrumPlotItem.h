#ifndef SPECTRUM_PLOT_ITEM_H
#define SPECTRUM_PLOT_ITEM_H

#include <QPainter>
#include <QRect>

#include <qwt_plot_item.h>
#include <qwt_scale_map.h>

#include "MantidQtSpectrumViewer/DataArray.h"
#include "MantidQtSpectrumViewer/DllOptionSV.h"

/**
    @class SpectrumPlotItem

    This class is responsible for actually drawing the image data onto
    a QwtPlot for the SpectrumView data viewer.

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
namespace SpectrumView
{

class EXPORT_OPT_MANTIDQT_SPECTRUMVIEWER SpectrumPlotItem : public QwtPlotItem
{

public:

  /// Construct basic plot item with NO data to plot.
  SpectrumPlotItem();

  virtual ~SpectrumPlotItem();

  /// Specify the data to be plotted and the color table to use
  void setData( DataArray_const_sptr dataArray,
                std::vector<QRgb>* positiveColorTable,
                std::vector<QRgb>* negativeColorTable );

  /// Set a non-linear lookup table to scale data values before mapping to color
  void setIntensityTable( std::vector<double>*  intensityTable );

  /// Draw the image (this is called by QWT and must not be called directly.)
  virtual void draw(      QPainter    * painter,
                    const QwtScaleMap & xMap,
                    const QwtScaleMap & yMap,
                    const QRect       & canvasRect) const;

protected:
  int m_bufferID;       // set to 0 or 1 to select buffer
  DataArray_const_sptr m_dataArray0;     // these provide double buffers
  DataArray_const_sptr m_dataArray1;     // for the float data.

private:
  /* This class just uses the following */
  /* but they are created and deleted */
  /* in the upper level classes */
  std::vector<QRgb>   * m_positiveColorTable;
  std::vector<QRgb>   * m_negativeColorTable;
  std::vector<double> * m_intensityTable;

};

} // namespace SpectrumView
} // namespace MantidQt

#endif  // SPECTRUM_PLOT_ITEM_H
