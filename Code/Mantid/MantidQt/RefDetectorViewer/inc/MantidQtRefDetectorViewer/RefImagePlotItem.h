#ifndef REF_IMAGE_PLOT_ITEM_H
#define REF_IMAGE_PLOT_ITEM_H

#include "MantidQtSpectrumViewer/SpectrumPlotItem.h"
#include "DllOption.h"
#include "MantidQtRefDetectorViewer/RefLimitsHandler.h"

namespace MantidQt
{
namespace RefDetectorViewer
{
/** This class is responsible for actually drawing the image data onto
    a QwtPlot for the SpectrumView data viewer.

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

class EXPORT_OPT_MANTIDQT_REFDETECTORVIEWER RefImagePlotItem : public SpectrumView::SpectrumPlotItem
{

public:
  /// Construct basic plot item with NO data to plot.
  RefImagePlotItem(const RefLimitsHandler * const limitsHandler);

  ~RefImagePlotItem();

  /// Draw the image (this is called by QWT and must not be called directly.)
  virtual void draw(      QPainter    * painter,
                    const QwtScaleMap & xMap,
                    const QwtScaleMap & yMap,
                    const QRect       & canvasRect) const;

private:
  const RefLimitsHandler * const m_limitsHandler;

};

} // namespace RefDetectorViewer
} // namespace MantidQt


#endif  // REF_IMAGE_PLOT_ITEM_H
