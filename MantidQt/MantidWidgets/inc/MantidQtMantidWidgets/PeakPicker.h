#ifndef MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_
#define MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_

#include "MantidKernel/System.h"

#include "MantidAPI/IPeakFunction.h"
#include "MantidQtMantidWidgets/WidgetDllOption.h"

#include <qwt_plot.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_picker.h>

namespace MantidQt
{
namespace MantidWidgets
{

  /** PeakPicker : A simplified version of PeakPickerTool, available for use on general QwtPlots.
    
    Copyright &copy; 2014 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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

    File change history is stored at: <https://github.com/mantidproject/mantid>
    Code Documentation is available at: <http://doxygen.mantidproject.org>
  */
  class EXPORT_OPT_MANTIDQT_MANTIDWIDGETS PeakPicker : public QwtPlotPicker, public QwtPlotItem
  {
    Q_OBJECT

  public:
    /// Constructor
    PeakPicker(QwtPlot* plot, QColor color);

    /// Correct QwtPlotItem type info
    virtual int rtti() const { return QwtPlotItem::Rtti_PlotMarker; }

    /// Draw the peak picker
    /// @see QwtPlotItem::draw
    virtual void draw(QPainter* painter, const QwtScaleMap& xMap, const QwtScaleMap& yMap,
                      const QRect& canvasRect) const;

    /// @return Currently represented peak
    Mantid::API::IPeakFunction_const_sptr peak() const;

    /// @param peak :: New peak to represent
    void setPeak(const Mantid::API::IPeakFunction_const_sptr& peak);

  signals:
    /// Emitted when the peak picker is moved or resized in any way
    void changed();

  private:
    /// Size of the dragging region. Bigger value means it's easier to hit the dragging area, though
    /// harder to get to the right element if there are lots of them.
    static const double DRAG_SENSITIVITY;

    /// Default cursor to use when not dragging
    static const QCursorShape DEFAULT_CURSOR;

    /// Event filter installed for the plot
    bool eventFilter(QObject* object, QEvent* event);

    /// The plot peak picker operates on
    QwtPlot* m_plot;

    /// Pens used for drawing
    QPen m_basePen, m_widthPen;

    /// Dragging status flags
    bool m_isMoving, m_isResizing;

    /// Currently represented peak
    Mantid::API::IPeakFunction_sptr m_peak;
  };




} // namespace MantidWidgets
} // namespace MantidQt

#endif  /* MANTIDQT_MANTIDWIDGETS_PEAKPICKER_H_ */
